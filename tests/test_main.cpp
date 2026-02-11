#include <catch2/catch_all.hpp>

#include "ecs/component.hpp"
#include "ecs/registry.hpp"
#include "game_constants.hpp"
#include "level.hpp"
#include "score_manager.hpp"
#include "sound_manager.hpp"
#include "systems/ball_physics_system.hpp"
#include "systems/movement_system.hpp"
#include "systems/particle_system.hpp"

#include <algorithm>
#include <cmath>
#include <vector>

using namespace breakout;
using namespace breakout::ecs;

// ============================================================================
// Helper: SDL initialization for tests requiring audio (BallPhysicsSystem)
// ============================================================================
namespace
{

bool sdl_audio_available()
{
    static bool available = []()
    {
        SDL_SetHint(SDL_HINT_AUDIO_DRIVER, "dummy");
        return SDL_Init(SDL_INIT_AUDIO);
    }();
    return available;
}

} // namespace

// ============================================================================
// ComponentPoolImpl Tests
// ============================================================================

TEST_CASE("ComponentPoolImpl add and retrieve", "[ecs][pool]")
{
    ComponentPoolImpl<TransformComponent> pool;

    pool.add(0, TransformComponent{{100.0f, 200.0f}, 50.0f, 30.0f});

    REQUIRE(pool.has(0));
    auto *comp = pool.get(0);
    REQUIRE(comp != nullptr);
    REQUIRE(comp->position.x == Catch::Approx(100.0f));
    REQUIRE(comp->position.y == Catch::Approx(200.0f));
    REQUIRE(comp->width == Catch::Approx(50.0f));
    REQUIRE(comp->height == Catch::Approx(30.0f));
}

TEST_CASE("ComponentPoolImpl has returns false for missing entity", "[ecs][pool]")
{
    ComponentPoolImpl<TransformComponent> pool;

    REQUIRE_FALSE(pool.has(0));
    REQUIRE_FALSE(pool.has(99));
}

TEST_CASE("ComponentPoolImpl get returns nullptr for missing entity", "[ecs][pool]")
{
    ComponentPoolImpl<TransformComponent> pool;

    REQUIRE(pool.get(0) == nullptr);
    REQUIRE(pool.get(42) == nullptr);
}

TEST_CASE("ComponentPoolImpl remove clears component", "[ecs][pool]")
{
    ComponentPoolImpl<VelocityComponent> pool;

    pool.add(0, VelocityComponent{{5.0f, 10.0f}});
    REQUIRE(pool.has(0));

    pool.remove(0);

    REQUIRE_FALSE(pool.has(0));
    REQUIRE(pool.get(0) == nullptr);
}

TEST_CASE("ComponentPoolImpl handles multiple entities", "[ecs][pool]")
{
    ComponentPoolImpl<TransformComponent> pool;

    pool.add(0, TransformComponent{{1.0f, 2.0f}, 10.0f, 10.0f});
    pool.add(5, TransformComponent{{3.0f, 4.0f}, 20.0f, 20.0f});
    pool.add(10, TransformComponent{{5.0f, 6.0f}, 30.0f, 30.0f});

    REQUIRE(pool.has(0));
    REQUIRE(pool.has(5));
    REQUIRE(pool.has(10));
    REQUIRE_FALSE(pool.has(1));
    REQUIRE_FALSE(pool.has(7));

    REQUIRE(pool.get(0)->position.x == Catch::Approx(1.0f));
    REQUIRE(pool.get(5)->position.x == Catch::Approx(3.0f));
    REQUIRE(pool.get(10)->position.x == Catch::Approx(5.0f));
}

TEST_CASE("ComponentPoolImpl remove only affects target entity", "[ecs][pool]")
{
    ComponentPoolImpl<VelocityComponent> pool;

    pool.add(0, VelocityComponent{{1.0f, 2.0f}});
    pool.add(1, VelocityComponent{{3.0f, 4.0f}});

    pool.remove(0);

    REQUIRE_FALSE(pool.has(0));
    REQUIRE(pool.has(1));
    REQUIRE(pool.get(1)->velocity.x == Catch::Approx(3.0f));
}

TEST_CASE("ComponentPoolImpl remove on non-existent entity is safe", "[ecs][pool]")
{
    ComponentPoolImpl<TransformComponent> pool;

    // Should not crash
    pool.remove(0);
    pool.remove(999);
    REQUIRE_FALSE(pool.has(0));
}

TEST_CASE("ComponentPoolImpl const get", "[ecs][pool]")
{
    ComponentPoolImpl<TransformComponent> pool;
    pool.add(0, TransformComponent{{10.0f, 20.0f}, 5.0f, 5.0f});

    const auto &const_pool = pool;
    const auto *comp = const_pool.get(0);

    REQUIRE(comp != nullptr);
    REQUIRE(comp->position.x == Catch::Approx(10.0f));
}

TEST_CASE("ComponentPoolImpl const get returns nullptr for missing", "[ecs][pool]")
{
    ComponentPoolImpl<TransformComponent> pool;

    const auto &const_pool = pool;
    REQUIRE(const_pool.get(0) == nullptr);
}

TEST_CASE("ComponentPoolImpl overwrite existing component", "[ecs][pool]")
{
    ComponentPoolImpl<VelocityComponent> pool;

    pool.add(0, VelocityComponent{{1.0f, 2.0f}});
    pool.add(0, VelocityComponent{{99.0f, 88.0f}});

    REQUIRE(pool.has(0));
    REQUIRE(pool.get(0)->velocity.x == Catch::Approx(99.0f));
    REQUIRE(pool.get(0)->velocity.y == Catch::Approx(88.0f));
}

// ============================================================================
// Registry Entity Lifecycle Tests
// ============================================================================

TEST_CASE("Registry create entity returns valid ID", "[ecs][registry]")
{
    Registry reg;

    Entity e = reg.create_entity();

    REQUIRE(e != INVALID_ENTITY);
    REQUIRE(reg.entity_count() == 1);
}

TEST_CASE("Registry create multiple entities returns unique IDs", "[ecs][registry]")
{
    Registry reg;

    Entity e1 = reg.create_entity();
    Entity e2 = reg.create_entity();
    Entity e3 = reg.create_entity();

    REQUIRE(e1 != e2);
    REQUIRE(e2 != e3);
    REQUIRE(e1 != e3);
    REQUIRE(reg.entity_count() == 3);
}

TEST_CASE("Registry destroy entity decreases count", "[ecs][registry]")
{
    Registry reg;

    Entity e1 = reg.create_entity();
    static_cast<void>(reg.create_entity());
    REQUIRE(reg.entity_count() == 2);

    reg.destroy_entity(e1);

    REQUIRE(reg.entity_count() == 1);
}

TEST_CASE("Registry destroyed entity ID is recycled", "[ecs][registry]")
{
    Registry reg;

    Entity e1 = reg.create_entity();
    reg.destroy_entity(e1);

    Entity e2 = reg.create_entity();

    REQUIRE(e2 != INVALID_ENTITY);
    REQUIRE(reg.entity_count() == 1);
    // Recycled ID should be the same as destroyed one
    REQUIRE(e2 == e1);
}

TEST_CASE("Registry living_entities reflects current state", "[ecs][registry]")
{
    Registry reg;

    Entity e1 = reg.create_entity();
    Entity e2 = reg.create_entity();
    Entity e3 = reg.create_entity();

    REQUIRE(reg.living_entities().size() == 3);

    reg.destroy_entity(e2);

    const auto &living = reg.living_entities();
    REQUIRE(living.size() == 2);
    REQUIRE(std::find(living.begin(), living.end(), e1) != living.end());
    REQUIRE(std::find(living.begin(), living.end(), e3) != living.end());
    REQUIRE(std::find(living.begin(), living.end(), e2) == living.end());
}

TEST_CASE("Registry entity_count starts at zero", "[ecs][registry]")
{
    Registry reg;

    REQUIRE(reg.entity_count() == 0);
}

TEST_CASE("Registry create and destroy many entities", "[ecs][registry]")
{
    Registry reg;
    std::vector<Entity> entities;

    for (int i = 0; i < 100; ++i)
    {
        entities.push_back(reg.create_entity());
    }
    REQUIRE(reg.entity_count() == 100);

    for (int i = 0; i < 50; ++i)
    {
        reg.destroy_entity(entities[static_cast<std::size_t>(i)]);
    }
    REQUIRE(reg.entity_count() == 50);
}

// ============================================================================
// Registry Component Operations Tests
// ============================================================================

TEST_CASE("Registry add and get component", "[ecs][registry]")
{
    Registry reg;
    Entity e = reg.create_entity();

    reg.add_component<TransformComponent>(e, {{100.0f, 200.0f}, 50.0f, 30.0f});

    REQUIRE(reg.has_component<TransformComponent>(e));
    auto *comp = reg.get_component<TransformComponent>(e);
    REQUIRE(comp != nullptr);
    REQUIRE(comp->position.x == Catch::Approx(100.0f));
    REQUIRE(comp->position.y == Catch::Approx(200.0f));
}

TEST_CASE("Registry has_component returns false when not added", "[ecs][registry]")
{
    Registry reg;
    Entity e = reg.create_entity();

    REQUIRE_FALSE(reg.has_component<TransformComponent>(e));
    REQUIRE_FALSE(reg.has_component<VelocityComponent>(e));
    REQUIRE_FALSE(reg.has_component<BallComponent>(e));
}

TEST_CASE("Registry get_component returns nullptr when not added", "[ecs][registry]")
{
    Registry reg;
    Entity e = reg.create_entity();

    REQUIRE(reg.get_component<TransformComponent>(e) == nullptr);
}

TEST_CASE("Registry remove_component", "[ecs][registry]")
{
    Registry reg;
    Entity e = reg.create_entity();

    reg.add_component<VelocityComponent>(e, {{5.0f, 10.0f}});
    REQUIRE(reg.has_component<VelocityComponent>(e));

    reg.remove_component<VelocityComponent>(e);

    REQUIRE_FALSE(reg.has_component<VelocityComponent>(e));
}

TEST_CASE("Registry multiple component types on single entity", "[ecs][registry]")
{
    Registry reg;
    Entity e = reg.create_entity();

    reg.add_component<TransformComponent>(e, {{1.0f, 2.0f}, 3.0f, 4.0f});
    reg.add_component<VelocityComponent>(e, {{5.0f, 6.0f}});
    reg.add_component<BallComponent>(e, {});

    REQUIRE(reg.has_component<TransformComponent>(e));
    REQUIRE(reg.has_component<VelocityComponent>(e));
    REQUIRE(reg.has_component<BallComponent>(e));
    REQUIRE_FALSE(reg.has_component<PaddleComponent>(e));
}

TEST_CASE("Registry destroy_entity removes all components", "[ecs][registry]")
{
    Registry reg;

    Entity e = reg.create_entity();
    reg.add_component<TransformComponent>(e, {{0.0f, 0.0f}, 1.0f, 1.0f});
    reg.add_component<VelocityComponent>(e, {{0.0f, 0.0f}});

    reg.destroy_entity(e);

    // Recreate entity (recycled ID)
    Entity e2 = reg.create_entity();
    REQUIRE_FALSE(reg.has_component<TransformComponent>(e2));
    REQUIRE_FALSE(reg.has_component<VelocityComponent>(e2));
}

TEST_CASE("Registry const get_component", "[ecs][registry]")
{
    Registry reg;
    Entity e = reg.create_entity();
    reg.add_component<TransformComponent>(e, {{42.0f, 84.0f}, 10.0f, 10.0f});

    const auto &const_reg = reg;
    const auto *comp = const_reg.get_component<TransformComponent>(e);

    REQUIRE(comp != nullptr);
    REQUIRE(comp->position.x == Catch::Approx(42.0f));
}

TEST_CASE("Registry component independence between entities", "[ecs][registry]")
{
    Registry reg;
    Entity e1 = reg.create_entity();
    Entity e2 = reg.create_entity();

    reg.add_component<TransformComponent>(e1, {{10.0f, 20.0f}, 5.0f, 5.0f});
    reg.add_component<TransformComponent>(e2, {{30.0f, 40.0f}, 7.0f, 7.0f});

    auto *c1 = reg.get_component<TransformComponent>(e1);
    auto *c2 = reg.get_component<TransformComponent>(e2);

    REQUIRE(c1->position.x == Catch::Approx(10.0f));
    REQUIRE(c2->position.x == Catch::Approx(30.0f));

    // Modify one, ensure the other is unchanged
    c1->position.x = 999.0f;
    REQUIRE(c2->position.x == Catch::Approx(30.0f));
}

// ============================================================================
// Registry View Tests
// ============================================================================

TEST_CASE("Registry view returns entities with matching components", "[ecs][registry]")
{
    Registry reg;

    Entity ball = reg.create_entity();
    reg.add_component<TransformComponent>(ball, {{0.0f, 0.0f}, 16.0f, 16.0f});
    reg.add_component<VelocityComponent>(ball, {{100.0f, 200.0f}});
    reg.add_component<BallComponent>(ball, {});

    Entity paddle = reg.create_entity();
    reg.add_component<TransformComponent>(paddle, {{400.0f, 550.0f}, 100.0f, 20.0f});
    reg.add_component<VelocityComponent>(paddle, {{0.0f, 0.0f}});
    reg.add_component<PaddleComponent>(paddle, {});

    auto balls = reg.view<TransformComponent, VelocityComponent, BallComponent>();
    auto paddles = reg.view<TransformComponent, PaddleComponent>();

    REQUIRE(balls.size() == 1);
    REQUIRE(balls[0] == ball);
    REQUIRE(paddles.size() == 1);
    REQUIRE(paddles[0] == paddle);
}

TEST_CASE("Registry view returns empty when no matches", "[ecs][registry]")
{
    Registry reg;

    Entity e = reg.create_entity();
    reg.add_component<TransformComponent>(e, {{0.0f, 0.0f}, 1.0f, 1.0f});

    auto result = reg.view<TransformComponent, BallComponent>();

    REQUIRE(result.empty());
}

TEST_CASE("Registry view with single component type", "[ecs][registry]")
{
    Registry reg;

    Entity e1 = reg.create_entity();
    reg.add_component<BrickComponent>(e1, {3, BrickColor::RED});

    Entity e2 = reg.create_entity();
    reg.add_component<BrickComponent>(e2, {1, BrickColor::BLUE});

    Entity e3 = reg.create_entity();
    reg.add_component<TransformComponent>(e3, {{0.0f, 0.0f}, 1.0f, 1.0f});

    auto bricks = reg.view<BrickComponent>();

    REQUIRE(bricks.size() == 2);
}

TEST_CASE("Registry view reflects entity destruction", "[ecs][registry]")
{
    Registry reg;

    Entity e1 = reg.create_entity();
    reg.add_component<BrickComponent>(e1, {1, BrickColor::BLUE});

    Entity e2 = reg.create_entity();
    reg.add_component<BrickComponent>(e2, {2, BrickColor::YELLOW});

    REQUIRE(reg.view<BrickComponent>().size() == 2);

    reg.destroy_entity(e1);

    REQUIRE(reg.view<BrickComponent>().size() == 1);
    REQUIRE(reg.view<BrickComponent>()[0] == e2);
}

// ============================================================================
// Registry each() Tests
// ============================================================================

TEST_CASE("Registry each iterates matching entities", "[ecs][registry]")
{
    Registry reg;

    Entity e1 = reg.create_entity();
    reg.add_component<TransformComponent>(e1, {{10.0f, 20.0f}, 5.0f, 5.0f});

    Entity e2 = reg.create_entity();
    reg.add_component<TransformComponent>(e2, {{30.0f, 40.0f}, 5.0f, 5.0f});

    Entity e3 = reg.create_entity();
    reg.add_component<VelocityComponent>(e3, {{1.0f, 2.0f}});

    int count = 0;
    reg.each<TransformComponent>(
        [&count](Entity /*entity*/, TransformComponent & /*transform*/)
        {
            ++count;
        });

    REQUIRE(count == 2);
}

TEST_CASE("Registry each can modify components", "[ecs][registry]")
{
    Registry reg;

    Entity e = reg.create_entity();
    reg.add_component<TransformComponent>(e, {{0.0f, 0.0f}, 10.0f, 10.0f});

    reg.each<TransformComponent>(
        [](Entity /*entity*/, TransformComponent &transform)
        {
            transform.position.x += 50.0f;
        });

    auto *comp = reg.get_component<TransformComponent>(e);
    REQUIRE(comp->position.x == Catch::Approx(50.0f));
}

TEST_CASE("Registry each with multiple component types", "[ecs][registry]")
{
    Registry reg;

    Entity e1 = reg.create_entity();
    reg.add_component<TransformComponent>(e1, {{0.0f, 0.0f}, 16.0f, 16.0f});
    reg.add_component<VelocityComponent>(e1, {{10.0f, 20.0f}});

    Entity e2 = reg.create_entity();
    reg.add_component<TransformComponent>(e2, {{0.0f, 0.0f}, 16.0f, 16.0f});
    // No velocity on e2

    int count = 0;
    reg.each<TransformComponent, VelocityComponent>(
        [&count](Entity /*entity*/, TransformComponent & /*t*/, VelocityComponent & /*v*/)
        {
            ++count;
        });

    REQUIRE(count == 1);
}

TEST_CASE("Registry const each", "[ecs][registry]")
{
    Registry reg;

    Entity e = reg.create_entity();
    reg.add_component<TransformComponent>(e, {{5.0f, 10.0f}, 1.0f, 1.0f});

    const auto &const_reg = reg;
    float sum_x = 0.0f;
    const_reg.each<TransformComponent>(
        [&sum_x](Entity /*entity*/, const TransformComponent &t)
        {
            sum_x += t.position.x;
        });

    REQUIRE(sum_x == Catch::Approx(5.0f));
}

// ============================================================================
// ScoreManager Tests
// ============================================================================

TEST_CASE("ScoreManager initial state is zero", "[score]")
{
    ScoreManager sm;

    REQUIRE(sm.current_score() == 0);
    REQUIRE(sm.high_score() == 0);
}

TEST_CASE("ScoreManager add_score multiplies by 10", "[score]")
{
    ScoreManager sm;

    sm.add_score(1);
    REQUIRE(sm.current_score() == 10);

    sm.add_score(3);
    REQUIRE(sm.current_score() == 40);
}

TEST_CASE("ScoreManager high_score tracks maximum", "[score]")
{
    ScoreManager sm;

    sm.add_score(5);
    REQUIRE(sm.high_score() == 50);

    sm.reset();
    REQUIRE(sm.current_score() == 0);
    REQUIRE(sm.high_score() == 50);

    sm.add_score(3);
    REQUIRE(sm.high_score() == 50);
}

TEST_CASE("ScoreManager high_score updates when surpassed", "[score]")
{
    ScoreManager sm;

    sm.add_score(5);
    sm.reset();
    sm.add_score(10);

    REQUIRE(sm.current_score() == 100);
    REQUIRE(sm.high_score() == 100);
}

TEST_CASE("ScoreManager reset clears current score only", "[score]")
{
    ScoreManager sm;

    sm.add_score(10);
    REQUIRE(sm.current_score() == 100);

    sm.reset();
    REQUIRE(sm.current_score() == 0);
    REQUIRE(sm.high_score() == 100);
}

TEST_CASE("ScoreManager multiple resets", "[score]")
{
    ScoreManager sm;

    sm.add_score(5);
    sm.reset();
    sm.add_score(3);
    sm.reset();
    sm.add_score(1);

    REQUIRE(sm.current_score() == 10);
    REQUIRE(sm.high_score() == 50);
}

TEST_CASE("ScoreManager is_level_complete with no bricks", "[score]")
{
    Registry reg;
    ScoreManager sm;

    REQUIRE(sm.is_level_complete(reg));
}

TEST_CASE("ScoreManager is_level_complete with bricks", "[score]")
{
    Registry reg;
    ScoreManager sm;

    Entity brick = reg.create_entity();
    reg.add_component<BrickComponent>(brick, {3, BrickColor::RED});

    REQUIRE_FALSE(sm.is_level_complete(reg));
}

TEST_CASE("ScoreManager is_level_complete after all bricks destroyed", "[score]")
{
    Registry reg;
    ScoreManager sm;

    Entity brick = reg.create_entity();
    reg.add_component<BrickComponent>(brick, {1, BrickColor::BLUE});

    REQUIRE_FALSE(sm.is_level_complete(reg));

    reg.destroy_entity(brick);

    REQUIRE(sm.is_level_complete(reg));
}

TEST_CASE("ScoreManager is_level_complete with multiple bricks", "[score]")
{
    Registry reg;
    ScoreManager sm;

    Entity b1 = reg.create_entity();
    reg.add_component<BrickComponent>(b1, {1, BrickColor::BLUE});

    Entity b2 = reg.create_entity();
    reg.add_component<BrickComponent>(b2, {2, BrickColor::YELLOW});

    REQUIRE_FALSE(sm.is_level_complete(reg));

    reg.destroy_entity(b1);
    REQUIRE_FALSE(sm.is_level_complete(reg));

    reg.destroy_entity(b2);
    REQUIRE(sm.is_level_complete(reg));
}

// ============================================================================
// MovementSystem Tests
// ============================================================================

TEST_CASE("MovementSystem moves paddle based on velocity", "[movement]")
{
    Registry reg;
    systems::MovementSystem movement;

    Entity paddle = reg.create_entity();
    reg.add_component<TransformComponent>(paddle, {{400.0f, 550.0f}, 100.0f, 20.0f});
    reg.add_component<VelocityComponent>(paddle, {{200.0f, 0.0f}});
    reg.add_component<PaddleComponent>(paddle, {});

    constexpr float dt = 1.0f / 60.0f;
    movement.update(reg, dt, 800.0f);

    auto *transform = reg.get_component<TransformComponent>(paddle);
    REQUIRE(transform->position.x == Catch::Approx(400.0f + 200.0f * dt));
}

TEST_CASE("MovementSystem moves paddle left with negative velocity", "[movement]")
{
    Registry reg;
    systems::MovementSystem movement;

    Entity paddle = reg.create_entity();
    reg.add_component<TransformComponent>(paddle, {{400.0f, 550.0f}, 100.0f, 20.0f});
    reg.add_component<VelocityComponent>(paddle, {{-200.0f, 0.0f}});
    reg.add_component<PaddleComponent>(paddle, {});

    constexpr float dt = 1.0f / 60.0f;
    movement.update(reg, dt, 800.0f);

    auto *transform = reg.get_component<TransformComponent>(paddle);
    REQUIRE(transform->position.x == Catch::Approx(400.0f - 200.0f * dt));
}

TEST_CASE("MovementSystem clamps paddle to left boundary", "[movement]")
{
    Registry reg;
    systems::MovementSystem movement;

    Entity paddle = reg.create_entity();
    reg.add_component<TransformComponent>(paddle, {{10.0f, 550.0f}, 100.0f, 20.0f});
    reg.add_component<VelocityComponent>(paddle, {{-500.0f, 0.0f}});
    reg.add_component<PaddleComponent>(paddle, {});

    movement.update(reg, 1.0f, 800.0f);

    auto *transform = reg.get_component<TransformComponent>(paddle);
    // Minimum position is half paddle width = 50.0
    REQUIRE(transform->position.x == Catch::Approx(50.0f));
}

TEST_CASE("MovementSystem clamps paddle to right boundary", "[movement]")
{
    Registry reg;
    systems::MovementSystem movement;

    Entity paddle = reg.create_entity();
    reg.add_component<TransformComponent>(paddle, {{790.0f, 550.0f}, 100.0f, 20.0f});
    reg.add_component<VelocityComponent>(paddle, {{500.0f, 0.0f}});
    reg.add_component<PaddleComponent>(paddle, {});

    movement.update(reg, 1.0f, 800.0f);

    auto *transform = reg.get_component<TransformComponent>(paddle);
    // Maximum position is screen_width - half paddle width = 800 - 50 = 750
    REQUIRE(transform->position.x == Catch::Approx(750.0f));
}

TEST_CASE("MovementSystem zero velocity keeps paddle position", "[movement]")
{
    Registry reg;
    systems::MovementSystem movement;

    Entity paddle = reg.create_entity();
    reg.add_component<TransformComponent>(paddle, {{400.0f, 550.0f}, 100.0f, 20.0f});
    reg.add_component<VelocityComponent>(paddle, {{0.0f, 0.0f}});
    reg.add_component<PaddleComponent>(paddle, {});

    movement.update(reg, 1.0f, 800.0f);

    auto *transform = reg.get_component<TransformComponent>(paddle);
    REQUIRE(transform->position.x == Catch::Approx(400.0f));
}

TEST_CASE("MovementSystem ignores non-paddle entities", "[movement]")
{
    Registry reg;
    systems::MovementSystem movement;

    Entity ball = reg.create_entity();
    reg.add_component<TransformComponent>(ball, {{100.0f, 100.0f}, 16.0f, 16.0f});
    reg.add_component<VelocityComponent>(ball, {{500.0f, 500.0f}});
    reg.add_component<BallComponent>(ball, {});

    movement.update(reg, 1.0f, 800.0f);

    auto *transform = reg.get_component<TransformComponent>(ball);
    // Ball should not have moved - no PaddleComponent
    REQUIRE(transform->position.x == Catch::Approx(100.0f));
}

TEST_CASE("MovementSystem handles multiple paddles independently", "[movement]")
{
    Registry reg;
    systems::MovementSystem movement;

    Entity p1 = reg.create_entity();
    reg.add_component<TransformComponent>(p1, {{200.0f, 550.0f}, 100.0f, 20.0f});
    reg.add_component<VelocityComponent>(p1, {{100.0f, 0.0f}});
    reg.add_component<PaddleComponent>(p1, {});

    Entity p2 = reg.create_entity();
    reg.add_component<TransformComponent>(p2, {{600.0f, 550.0f}, 100.0f, 20.0f});
    reg.add_component<VelocityComponent>(p2, {{-100.0f, 0.0f}});
    reg.add_component<PaddleComponent>(p2, {});

    constexpr float dt = 0.1f;
    movement.update(reg, dt, 800.0f);

    auto *t1 = reg.get_component<TransformComponent>(p1);
    auto *t2 = reg.get_component<TransformComponent>(p2);

    REQUIRE(t1->position.x == Catch::Approx(210.0f));
    REQUIRE(t2->position.x == Catch::Approx(590.0f));
}

// ============================================================================
// ParticleSystem Tests
// ============================================================================

TEST_CASE("ParticleSystem spawn_destruction_effects creates entities", "[particle]")
{
    Registry reg;
    systems::ParticleSystem ps;

    TransformComponent brick_transform{{400.0f, 200.0f}, BRICK_WIDTH, BRICK_HEIGHT};

    std::size_t before = reg.entity_count();
    ps.spawn_destruction_effects(reg, brick_transform, BrickColor::RED, 400.0f, 210.0f);
    std::size_t after = reg.entity_count();

    // Should have created shard + particle entities
    REQUIRE(after > before);
}

TEST_CASE("ParticleSystem spawned shards have required components", "[particle]")
{
    Registry reg;
    systems::ParticleSystem ps;

    TransformComponent brick_transform{{400.0f, 200.0f}, BRICK_WIDTH, BRICK_HEIGHT};
    ps.spawn_destruction_effects(reg, brick_transform, BrickColor::BLUE, 400.0f, 210.0f);

    auto shards = reg.view<TransformComponent, VelocityComponent, ShardComponent, SpriteComponent>();
    REQUIRE_FALSE(shards.empty());

    for (Entity shard : shards)
    {
        auto *sc = reg.get_component<ShardComponent>(shard);
        REQUIRE(sc != nullptr);
        REQUIRE(sc->lifetime > 0.0f);
        REQUIRE(sc->max_lifetime > 0.0f);
        REQUIRE(sc->lifetime == Catch::Approx(sc->max_lifetime));
    }
}

TEST_CASE("ParticleSystem spawned particles have required components", "[particle]")
{
    Registry reg;
    systems::ParticleSystem ps;

    TransformComponent brick_transform{{400.0f, 200.0f}, BRICK_WIDTH, BRICK_HEIGHT};
    ps.spawn_destruction_effects(reg, brick_transform, BrickColor::YELLOW, 400.0f, 210.0f);

    auto particles =
        reg.view<TransformComponent, VelocityComponent, ParticleComponent, SpriteComponent>();
    REQUIRE_FALSE(particles.empty());

    for (Entity particle : particles)
    {
        auto *pc = reg.get_component<ParticleComponent>(particle);
        REQUIRE(pc != nullptr);
        REQUIRE(pc->lifetime > 0.0f);
        REQUIRE(pc->max_lifetime > 0.0f);
        REQUIRE(pc->size > 0.0f);
    }
}

TEST_CASE("ParticleSystem update reduces shard lifetime", "[particle]")
{
    Registry reg;
    systems::ParticleSystem ps;

    TransformComponent brick_transform{{400.0f, 200.0f}, BRICK_WIDTH, BRICK_HEIGHT};
    ps.spawn_destruction_effects(reg, brick_transform, BrickColor::RED, 400.0f, 210.0f);

    auto shards = reg.view<ShardComponent>();
    REQUIRE_FALSE(shards.empty());

    float initial_lifetime = reg.get_component<ShardComponent>(shards[0])->lifetime;

    constexpr float dt = 0.05f;
    ps.update(reg, dt);

    auto remaining_shards = reg.view<ShardComponent>();
    if (!remaining_shards.empty())
    {
        float updated_lifetime = reg.get_component<ShardComponent>(remaining_shards[0])->lifetime;
        REQUIRE(updated_lifetime < initial_lifetime);
    }
}

TEST_CASE("ParticleSystem expired entities are destroyed", "[particle]")
{
    Registry reg;
    systems::ParticleSystem ps;

    TransformComponent brick_transform{{400.0f, 200.0f}, BRICK_WIDTH, BRICK_HEIGHT};
    ps.spawn_destruction_effects(reg, brick_transform, BrickColor::BLUE, 400.0f, 210.0f);

    std::size_t initial_count = reg.entity_count();
    REQUIRE(initial_count > 0);

    // Large delta_time expires all particles and shards
    ps.update(reg, 10.0f);

    REQUIRE(reg.entity_count() == 0);
}

TEST_CASE("ParticleSystem shard gravity increases downward velocity", "[particle]")
{
    Registry reg;
    systems::ParticleSystem ps;

    TransformComponent brick_transform{{400.0f, 200.0f}, BRICK_WIDTH, BRICK_HEIGHT};
    ps.spawn_destruction_effects(reg, brick_transform, BrickColor::RED, 400.0f, 210.0f);

    auto shards = reg.view<TransformComponent, VelocityComponent, ShardComponent>();
    REQUIRE_FALSE(shards.empty());

    // Record initial Y velocities of all shards
    std::vector<float> initial_vys;
    for (Entity shard : shards)
    {
        initial_vys.push_back(reg.get_component<VelocityComponent>(shard)->velocity.y);
    }

    constexpr float dt = 0.01f;
    ps.update(reg, dt);

    // After update, each shard's Y velocity should have increased (gravity = +200)
    auto updated_shards = reg.view<TransformComponent, VelocityComponent, ShardComponent>();
    for (std::size_t i = 0; i < updated_shards.size() && i < initial_vys.size(); ++i)
    {
        float updated_vy =
            reg.get_component<VelocityComponent>(updated_shards[i])->velocity.y;
        REQUIRE(updated_vy > initial_vys[i]);
    }
}

TEST_CASE("ParticleSystem particle drag reduces velocity", "[particle]")
{
    Registry reg;
    systems::ParticleSystem ps;

    TransformComponent brick_transform{{400.0f, 200.0f}, BRICK_WIDTH, BRICK_HEIGHT};
    ps.spawn_destruction_effects(reg, brick_transform, BrickColor::YELLOW, 400.0f, 210.0f);

    auto particles =
        reg.view<TransformComponent, VelocityComponent, ParticleComponent>();
    REQUIRE_FALSE(particles.empty());

    // Record initial speed of first particle
    auto *vel = reg.get_component<VelocityComponent>(particles[0]);
    float initial_speed = std::sqrt(vel->velocity.x * vel->velocity.x +
                                    vel->velocity.y * vel->velocity.y);

    constexpr float dt = 0.01f;
    ps.update(reg, dt);

    auto remaining = reg.view<TransformComponent, VelocityComponent, ParticleComponent>();
    if (!remaining.empty())
    {
        auto *updated_vel = reg.get_component<VelocityComponent>(remaining[0]);
        float updated_speed = std::sqrt(updated_vel->velocity.x * updated_vel->velocity.x +
                                        updated_vel->velocity.y * updated_vel->velocity.y);
        // Drag (0.98x per update) should reduce speed
        REQUIRE(updated_speed < initial_speed);
    }
}

TEST_CASE("ParticleSystem shard alpha fades with lifetime", "[particle]")
{
    Registry reg;
    systems::ParticleSystem ps;

    TransformComponent brick_transform{{400.0f, 200.0f}, BRICK_WIDTH, BRICK_HEIGHT};
    ps.spawn_destruction_effects(reg, brick_transform, BrickColor::BLUE, 400.0f, 210.0f);

    auto shards = reg.view<ShardComponent, SpriteComponent>();
    REQUIRE_FALSE(shards.empty());

    // Initially alpha should be 1.0
    auto *sprite = reg.get_component<SpriteComponent>(shards[0]);
    REQUIRE(sprite->tint.a == Catch::Approx(1.0f));

    constexpr float dt = 0.1f;
    ps.update(reg, dt);

    auto remaining = reg.view<ShardComponent, SpriteComponent>();
    if (!remaining.empty())
    {
        auto *updated_sprite = reg.get_component<SpriteComponent>(remaining[0]);
        REQUIRE(updated_sprite->tint.a < 1.0f);
        REQUIRE(updated_sprite->tint.a >= 0.0f);
    }
}

// ============================================================================
// BallPhysicsSystem Tests (require SDL dummy audio for SoundManager)
// ============================================================================

TEST_CASE("BallPhysicsSystem ball moves according to velocity", "[ball_physics]")
{
    if (!sdl_audio_available())
    {
        SKIP("SDL audio not available");
    }

    Registry reg;
    SoundManager sound;
    systems::ParticleSystem particles;
    ScoreManager score;
    systems::BallPhysicsSystem physics(sound, particles, score);

    Entity ball = reg.create_entity();
    reg.add_component<TransformComponent>(ball, {{400.0f, 300.0f}, 16.0f, 16.0f});
    reg.add_component<VelocityComponent>(ball, {{200.0f, -300.0f}});
    reg.add_component<BallComponent>(ball, {});

    constexpr float dt = 1.0f / 60.0f;
    physics.update(reg, dt, 800.0f, 600.0f);

    auto *transform = reg.get_component<TransformComponent>(ball);
    REQUIRE(transform->position.x == Catch::Approx(400.0f + 200.0f * dt).margin(1.0f));
    REQUIRE(transform->position.y == Catch::Approx(300.0f - 300.0f * dt).margin(1.0f));
}

TEST_CASE("BallPhysicsSystem ball bounces off left wall", "[ball_physics]")
{
    if (!sdl_audio_available())
    {
        SKIP("SDL audio not available");
    }

    Registry reg;
    SoundManager sound;
    systems::ParticleSystem particles;
    ScoreManager score;
    systems::BallPhysicsSystem physics(sound, particles, score);

    Entity ball = reg.create_entity();
    // Ball near left wall, moving left
    reg.add_component<TransformComponent>(ball, {{5.0f, 300.0f}, 16.0f, 16.0f});
    reg.add_component<VelocityComponent>(ball, {{-200.0f, 100.0f}});
    reg.add_component<BallComponent>(ball, {});

    physics.update(reg, 0.1f, 800.0f, 600.0f);

    auto *velocity = reg.get_component<VelocityComponent>(ball);
    REQUIRE(velocity->velocity.x > 0.0f);
}

TEST_CASE("BallPhysicsSystem ball bounces off right wall", "[ball_physics]")
{
    if (!sdl_audio_available())
    {
        SKIP("SDL audio not available");
    }

    Registry reg;
    SoundManager sound;
    systems::ParticleSystem particles;
    ScoreManager score;
    systems::BallPhysicsSystem physics(sound, particles, score);

    Entity ball = reg.create_entity();
    // Ball near right wall, moving right
    reg.add_component<TransformComponent>(ball, {{795.0f, 300.0f}, 16.0f, 16.0f});
    reg.add_component<VelocityComponent>(ball, {{200.0f, 100.0f}});
    reg.add_component<BallComponent>(ball, {});

    physics.update(reg, 0.1f, 800.0f, 600.0f);

    auto *velocity = reg.get_component<VelocityComponent>(ball);
    REQUIRE(velocity->velocity.x < 0.0f);
}

TEST_CASE("BallPhysicsSystem ball bounces off top wall", "[ball_physics]")
{
    if (!sdl_audio_available())
    {
        SKIP("SDL audio not available");
    }

    Registry reg;
    SoundManager sound;
    systems::ParticleSystem particles;
    ScoreManager score;
    systems::BallPhysicsSystem physics(sound, particles, score);

    Entity ball = reg.create_entity();
    // Ball near top, moving up
    reg.add_component<TransformComponent>(ball, {{400.0f, 5.0f}, 16.0f, 16.0f});
    reg.add_component<VelocityComponent>(ball, {{100.0f, -200.0f}});
    reg.add_component<BallComponent>(ball, {});

    physics.update(reg, 0.1f, 800.0f, 600.0f);

    auto *velocity = reg.get_component<VelocityComponent>(ball);
    REQUIRE(velocity->velocity.y > 0.0f);
}

TEST_CASE("BallPhysicsSystem ball resets when falling below screen", "[ball_physics]")
{
    if (!sdl_audio_available())
    {
        SKIP("SDL audio not available");
    }

    Registry reg;
    SoundManager sound;
    systems::ParticleSystem particles;
    ScoreManager score;
    systems::BallPhysicsSystem physics(sound, particles, score);

    Entity ball = reg.create_entity();
    // Ball below screen bottom
    reg.add_component<TransformComponent>(ball, {{400.0f, 620.0f}, 16.0f, 16.0f});
    reg.add_component<VelocityComponent>(ball, {{100.0f, 200.0f}});
    reg.add_component<BallComponent>(ball, {});

    physics.update(reg, 0.1f, 800.0f, 600.0f);

    auto *transform = reg.get_component<TransformComponent>(ball);
    // Ball resets to screen center
    REQUIRE(transform->position.x == Catch::Approx(400.0f));
    REQUIRE(transform->position.y == Catch::Approx(300.0f));

    auto *velocity = reg.get_component<VelocityComponent>(ball);
    // Y velocity should be upward after reset
    REQUIRE(velocity->velocity.y < 0.0f);
}

TEST_CASE("BallPhysicsSystem ball bounces off paddle", "[ball_physics]")
{
    if (!sdl_audio_available())
    {
        SKIP("SDL audio not available");
    }

    Registry reg;
    SoundManager sound;
    systems::ParticleSystem particles;
    ScoreManager score;
    systems::BallPhysicsSystem physics(sound, particles, score);

    // Create paddle at center bottom
    Entity paddle = reg.create_entity();
    reg.add_component<TransformComponent>(paddle, {{400.0f, 550.0f}, 100.0f, 20.0f});
    reg.add_component<VelocityComponent>(paddle, {{0.0f, 0.0f}});
    reg.add_component<PaddleComponent>(paddle, {});

    // Ball above paddle, moving downward
    Entity ball = reg.create_entity();
    reg.add_component<TransformComponent>(ball, {{400.0f, 540.0f}, 16.0f, 16.0f});
    reg.add_component<VelocityComponent>(ball, {{0.0f, 300.0f}});
    reg.add_component<BallComponent>(ball, {});

    physics.update(reg, 0.016f, 800.0f, 600.0f);

    auto *velocity = reg.get_component<VelocityComponent>(ball);
    // Ball should move upward after paddle collision
    REQUIRE(velocity->velocity.y < 0.0f);
}

TEST_CASE("BallPhysicsSystem paddle hit position affects bounce angle", "[ball_physics]")
{
    if (!sdl_audio_available())
    {
        SKIP("SDL audio not available");
    }

    Registry reg;
    SoundManager sound;
    systems::ParticleSystem particles;
    ScoreManager score;
    systems::BallPhysicsSystem physics(sound, particles, score);

    Entity paddle = reg.create_entity();
    reg.add_component<TransformComponent>(paddle, {{400.0f, 550.0f}, 100.0f, 20.0f});
    reg.add_component<VelocityComponent>(paddle, {{0.0f, 0.0f}});
    reg.add_component<PaddleComponent>(paddle, {});

    // Ball hits left edge of paddle
    Entity ball = reg.create_entity();
    reg.add_component<TransformComponent>(ball, {{355.0f, 540.0f}, 16.0f, 16.0f});
    reg.add_component<VelocityComponent>(ball, {{0.0f, 300.0f}});
    reg.add_component<BallComponent>(ball, {});

    physics.update(reg, 0.016f, 800.0f, 600.0f);

    auto *velocity = reg.get_component<VelocityComponent>(ball);
    // Hitting left side should send ball to the left
    REQUIRE(velocity->velocity.x < 0.0f);
    REQUIRE(velocity->velocity.y < 0.0f);
}

TEST_CASE("BallPhysicsSystem paddle right edge bounce", "[ball_physics]")
{
    if (!sdl_audio_available())
    {
        SKIP("SDL audio not available");
    }

    Registry reg;
    SoundManager sound;
    systems::ParticleSystem particles;
    ScoreManager score;
    systems::BallPhysicsSystem physics(sound, particles, score);

    Entity paddle = reg.create_entity();
    reg.add_component<TransformComponent>(paddle, {{400.0f, 550.0f}, 100.0f, 20.0f});
    reg.add_component<VelocityComponent>(paddle, {{0.0f, 0.0f}});
    reg.add_component<PaddleComponent>(paddle, {});

    // Ball hits right edge of paddle
    Entity ball = reg.create_entity();
    reg.add_component<TransformComponent>(ball, {{445.0f, 540.0f}, 16.0f, 16.0f});
    reg.add_component<VelocityComponent>(ball, {{0.0f, 300.0f}});
    reg.add_component<BallComponent>(ball, {});

    physics.update(reg, 0.016f, 800.0f, 600.0f);

    auto *velocity = reg.get_component<VelocityComponent>(ball);
    // Hitting right side should send ball to the right
    REQUIRE(velocity->velocity.x > 0.0f);
    REQUIRE(velocity->velocity.y < 0.0f);
}

TEST_CASE("BallPhysicsSystem moving paddle adds velocity influence", "[ball_physics]")
{
    if (!sdl_audio_available())
    {
        SKIP("SDL audio not available");
    }

    Registry reg;
    SoundManager sound;
    systems::ParticleSystem particles;
    ScoreManager score;
    systems::BallPhysicsSystem physics(sound, particles, score);

    // Paddle moving right
    Entity paddle = reg.create_entity();
    reg.add_component<TransformComponent>(paddle, {{400.0f, 550.0f}, 100.0f, 20.0f});
    reg.add_component<VelocityComponent>(paddle, {{500.0f, 0.0f}});
    reg.add_component<PaddleComponent>(paddle, {});

    // Ball hits center of paddle moving down
    Entity ball = reg.create_entity();
    reg.add_component<TransformComponent>(ball, {{400.0f, 540.0f}, 16.0f, 16.0f});
    reg.add_component<VelocityComponent>(ball, {{0.0f, 300.0f}});
    reg.add_component<BallComponent>(ball, {});

    physics.update(reg, 0.016f, 800.0f, 600.0f);

    auto *velocity = reg.get_component<VelocityComponent>(ball);
    // Center hit = straight up + paddle influence pushes right
    // Paddle influence = 500 * 0.5 = 250
    REQUIRE(velocity->velocity.x > 0.0f);
}

TEST_CASE("BallPhysicsSystem brick takes damage on collision", "[ball_physics]")
{
    if (!sdl_audio_available())
    {
        SKIP("SDL audio not available");
    }

    Registry reg;
    SoundManager sound;
    systems::ParticleSystem particles;
    ScoreManager score;
    systems::BallPhysicsSystem physics(sound, particles, score);

    // Red brick with 3 HP
    Entity brick = reg.create_entity();
    reg.add_component<TransformComponent>(brick, {{400.0f, 200.0f}, 60.0f, 20.0f});
    reg.add_component<BrickComponent>(brick, {3, BrickColor::RED});

    // Ball overlapping brick
    Entity ball = reg.create_entity();
    reg.add_component<TransformComponent>(ball, {{400.0f, 195.0f}, 16.0f, 16.0f});
    reg.add_component<VelocityComponent>(ball, {{0.0f, 200.0f}});
    reg.add_component<BallComponent>(ball, {});

    physics.update(reg, 0.016f, 800.0f, 600.0f);

    auto *brick_comp = reg.get_component<BrickComponent>(brick);
    REQUIRE(brick_comp != nullptr);
    REQUIRE(brick_comp->hit_points == 2);
}

TEST_CASE("BallPhysicsSystem brick destroyed when HP depleted", "[ball_physics]")
{
    if (!sdl_audio_available())
    {
        SKIP("SDL audio not available");
    }

    Registry reg;
    SoundManager sound;
    systems::ParticleSystem particles;
    ScoreManager score;
    systems::BallPhysicsSystem physics(sound, particles, score);

    // Blue brick with 1 HP
    Entity brick = reg.create_entity();
    reg.add_component<TransformComponent>(brick, {{400.0f, 200.0f}, 60.0f, 20.0f});
    reg.add_component<BrickComponent>(brick, {1, BrickColor::BLUE});

    // Ball overlapping brick
    Entity ball = reg.create_entity();
    reg.add_component<TransformComponent>(ball, {{400.0f, 195.0f}, 16.0f, 16.0f});
    reg.add_component<VelocityComponent>(ball, {{0.0f, 200.0f}});
    reg.add_component<BallComponent>(ball, {});

    physics.update(reg, 0.016f, 800.0f, 600.0f);

    // Brick should be destroyed (no more brick components, particles may have been created)
    auto bricks = reg.view<BrickComponent>();
    REQUIRE(bricks.empty());
}

TEST_CASE("BallPhysicsSystem blue brick awards 10 points", "[ball_physics][scoring]")
{
    if (!sdl_audio_available())
    {
        SKIP("SDL audio not available");
    }

    Registry reg;
    SoundManager sound;
    systems::ParticleSystem particles;
    ScoreManager score;
    systems::BallPhysicsSystem physics(sound, particles, score);

    Entity brick = reg.create_entity();
    reg.add_component<TransformComponent>(brick, {{400.0f, 200.0f}, 60.0f, 20.0f});
    reg.add_component<BrickComponent>(brick, {1, BrickColor::BLUE});

    Entity ball = reg.create_entity();
    reg.add_component<TransformComponent>(ball, {{400.0f, 195.0f}, 16.0f, 16.0f});
    reg.add_component<VelocityComponent>(ball, {{0.0f, 200.0f}});
    reg.add_component<BallComponent>(ball, {});

    physics.update(reg, 0.016f, 800.0f, 600.0f);

    // Blue = 1 point * 10 multiplier = 10
    REQUIRE(score.current_score() == 10);
}

TEST_CASE("BallPhysicsSystem yellow brick awards 20 points", "[ball_physics][scoring]")
{
    if (!sdl_audio_available())
    {
        SKIP("SDL audio not available");
    }

    Registry reg;
    SoundManager sound;
    systems::ParticleSystem particles;
    ScoreManager score;
    systems::BallPhysicsSystem physics(sound, particles, score);

    Entity brick = reg.create_entity();
    reg.add_component<TransformComponent>(brick, {{400.0f, 200.0f}, 60.0f, 20.0f});
    reg.add_component<BrickComponent>(brick, {1, BrickColor::YELLOW});

    Entity ball = reg.create_entity();
    reg.add_component<TransformComponent>(ball, {{400.0f, 195.0f}, 16.0f, 16.0f});
    reg.add_component<VelocityComponent>(ball, {{0.0f, 200.0f}});
    reg.add_component<BallComponent>(ball, {});

    physics.update(reg, 0.016f, 800.0f, 600.0f);

    // Yellow = 2 points * 10 multiplier = 20
    REQUIRE(score.current_score() == 20);
}

TEST_CASE("BallPhysicsSystem red brick awards 30 points", "[ball_physics][scoring]")
{
    if (!sdl_audio_available())
    {
        SKIP("SDL audio not available");
    }

    Registry reg;
    SoundManager sound;
    systems::ParticleSystem particles;
    ScoreManager score;
    systems::BallPhysicsSystem physics(sound, particles, score);

    Entity brick = reg.create_entity();
    reg.add_component<TransformComponent>(brick, {{400.0f, 200.0f}, 60.0f, 20.0f});
    reg.add_component<BrickComponent>(brick, {1, BrickColor::RED});

    Entity ball = reg.create_entity();
    reg.add_component<TransformComponent>(ball, {{400.0f, 195.0f}, 16.0f, 16.0f});
    reg.add_component<VelocityComponent>(ball, {{0.0f, 200.0f}});
    reg.add_component<BallComponent>(ball, {});

    physics.update(reg, 0.016f, 800.0f, 600.0f);

    // Red = 3 points * 10 multiplier = 30
    REQUIRE(score.current_score() == 30);
}

TEST_CASE("BallPhysicsSystem brick collision reverses ball velocity", "[ball_physics]")
{
    if (!sdl_audio_available())
    {
        SKIP("SDL audio not available");
    }

    Registry reg;
    SoundManager sound;
    systems::ParticleSystem particles;
    ScoreManager score;
    systems::BallPhysicsSystem physics(sound, particles, score);

    // Brick with enough HP to survive
    Entity brick = reg.create_entity();
    reg.add_component<TransformComponent>(brick, {{400.0f, 200.0f}, 60.0f, 20.0f});
    reg.add_component<BrickComponent>(brick, {3, BrickColor::RED});

    // Ball coming from above, moving down
    Entity ball = reg.create_entity();
    reg.add_component<TransformComponent>(ball, {{400.0f, 185.0f}, 16.0f, 16.0f});
    reg.add_component<VelocityComponent>(ball, {{0.0f, 300.0f}});
    reg.add_component<BallComponent>(ball, {});

    physics.update(reg, 0.016f, 800.0f, 600.0f);

    auto *velocity = reg.get_component<VelocityComponent>(ball);
    // Velocity should have been reversed on at least one axis
    bool x_reversed = velocity->velocity.x < 0.0f;
    bool y_reversed = velocity->velocity.y < 0.0f;
    REQUIRE((x_reversed || y_reversed));
}

TEST_CASE("BallPhysicsSystem no collision when ball far from brick", "[ball_physics]")
{
    if (!sdl_audio_available())
    {
        SKIP("SDL audio not available");
    }

    Registry reg;
    SoundManager sound;
    systems::ParticleSystem particles;
    ScoreManager score;
    systems::BallPhysicsSystem physics(sound, particles, score);

    Entity brick = reg.create_entity();
    reg.add_component<TransformComponent>(brick, {{400.0f, 100.0f}, 60.0f, 20.0f});
    reg.add_component<BrickComponent>(brick, {3, BrickColor::RED});

    // Ball far from brick
    Entity ball = reg.create_entity();
    reg.add_component<TransformComponent>(ball, {{400.0f, 400.0f}, 16.0f, 16.0f});
    reg.add_component<VelocityComponent>(ball, {{200.0f, -300.0f}});
    reg.add_component<BallComponent>(ball, {});

    physics.update(reg, 0.016f, 800.0f, 600.0f);

    auto *brick_comp = reg.get_component<BrickComponent>(brick);
    REQUIRE(brick_comp->hit_points == 3);
    REQUIRE(score.current_score() == 0);
}

// ============================================================================
// Level Generation Tests
// ============================================================================

TEST_CASE("Level generates bricks", "[level]")
{
    Registry reg;
    GameTextures textures{nullptr, nullptr, nullptr, nullptr, nullptr};

    Level::Config config;
    config.min_rows = 4;
    config.max_rows = 4;
    config.min_cols = 8;
    config.max_cols = 8;

    Level level(reg, textures, config);
    level.generate();

    auto bricks = reg.view<BrickComponent>();
    // With 40% empty chance over 4*8=32 slots, expect some bricks
    REQUIRE_FALSE(bricks.empty());
}

TEST_CASE("Level bricks have all required components", "[level]")
{
    Registry reg;
    GameTextures textures{nullptr, nullptr, nullptr, nullptr, nullptr};

    Level::Config config;
    config.min_rows = 4;
    config.max_rows = 4;
    config.min_cols = 8;
    config.max_cols = 8;

    Level level(reg, textures, config);
    level.generate();

    auto bricks = reg.view<BrickComponent>();
    for (Entity brick : bricks)
    {
        REQUIRE(reg.has_component<TransformComponent>(brick));
        REQUIRE(reg.has_component<SpriteComponent>(brick));
        REQUIRE(reg.has_component<BrickComponent>(brick));
    }
}

TEST_CASE("Level bricks have valid hit points matching color", "[level]")
{
    Registry reg;
    GameTextures textures{nullptr, nullptr, nullptr, nullptr, nullptr};

    Level::Config config;
    config.min_rows = 6;
    config.max_rows = 6;
    config.min_cols = 10;
    config.max_cols = 10;

    Level level(reg, textures, config);
    level.generate();

    auto bricks = reg.view<BrickComponent>();
    for (Entity brick : bricks)
    {
        auto *bc = reg.get_component<BrickComponent>(brick);
        REQUIRE(bc != nullptr);
        REQUIRE(bc->hit_points >= 1);
        REQUIRE(bc->hit_points <= 3);

        switch (bc->color)
        {
        case BrickColor::RED:
            REQUIRE(bc->hit_points == 3);
            break;
        case BrickColor::YELLOW:
            REQUIRE(bc->hit_points == 2);
            break;
        case BrickColor::BLUE:
            REQUIRE(bc->hit_points == 1);
            break;
        default:
            FAIL("Unknown brick color");
            break;
        }
    }
}

TEST_CASE("Level rows and cols match config when range is fixed", "[level]")
{
    Registry reg;
    GameTextures textures{nullptr, nullptr, nullptr, nullptr, nullptr};

    Level::Config config;
    config.min_rows = 5;
    config.max_rows = 5;
    config.min_cols = 9;
    config.max_cols = 9;

    Level level(reg, textures, config);
    level.generate();

    REQUIRE(level.rows() == 5);
    REQUIRE(level.cols() == 9);
}

TEST_CASE("Level layout type is valid", "[level]")
{
    Registry reg;
    GameTextures textures{nullptr, nullptr, nullptr, nullptr, nullptr};

    Level level(reg, textures);
    level.generate();

    auto lt = level.layout_type();
    REQUIRE((lt == LayoutType::Rectangular || lt == LayoutType::Circular));
}

TEST_CASE("Level bricks use configured dimensions", "[level]")
{
    Registry reg;
    GameTextures textures{nullptr, nullptr, nullptr, nullptr, nullptr};

    Level::Config config;
    config.min_rows = 4;
    config.max_rows = 4;
    config.min_cols = 8;
    config.max_cols = 8;
    config.brick_width = 70.0f;
    config.brick_height = 25.0f;

    Level level(reg, textures, config);
    level.generate();

    auto bricks = reg.view<BrickComponent, TransformComponent>();
    for (Entity brick : bricks)
    {
        auto *tc = reg.get_component<TransformComponent>(brick);
        REQUIRE(tc->width == Catch::Approx(70.0f));
        REQUIRE(tc->height == Catch::Approx(25.0f));
    }
}

TEST_CASE("Level dimensions within configured range", "[level]")
{
    Registry reg;
    GameTextures textures{nullptr, nullptr, nullptr, nullptr, nullptr};

    Level::Config config;
    config.min_rows = 3;
    config.max_rows = 7;
    config.min_cols = 6;
    config.max_cols = 12;

    Level level(reg, textures, config);
    level.generate();

    REQUIRE(level.rows() >= 3);
    REQUIRE(level.rows() <= 7);
    REQUIRE(level.cols() >= 6);
    REQUIRE(level.cols() <= 12);
}

// ============================================================================
// Game Constants Tests
// ============================================================================

TEST_CASE("Game constants have expected values", "[constants]")
{
    REQUIRE(WINDOW_WIDTH == 800);
    REQUIRE(WINDOW_HEIGHT == 600);
    REQUIRE(PADDLE_WIDTH == Catch::Approx(100.0f));
    REQUIRE(PADDLE_HEIGHT == Catch::Approx(20.0f));
    REQUIRE(BALL_SIZE == Catch::Approx(16.0f));
    REQUIRE(BRICK_WIDTH == Catch::Approx(60.0f));
    REQUIRE(BRICK_HEIGHT == Catch::Approx(20.0f));
}

TEST_CASE("Brick generation weights sum to expected total", "[constants]")
{
    int total = BRICK_WEIGHT_EMPTY + BRICK_WEIGHT_RED +
                BRICK_WEIGHT_YELLOW + BRICK_WEIGHT_BLUE;
    REQUIRE(total == 10);
}

TEST_CASE("Default tint is fully opaque white", "[constants]")
{
    REQUIRE(DEFAULT_TINT[0] == Catch::Approx(1.0f));
    REQUIRE(DEFAULT_TINT[1] == Catch::Approx(1.0f));
    REQUIRE(DEFAULT_TINT[2] == Catch::Approx(1.0f));
    REQUIRE(DEFAULT_TINT[3] == Catch::Approx(1.0f));
}
