#pragma once

#include "component.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <memory>
#include <type_traits>
#include <unordered_map>
#include <vector>

namespace breakout::ecs
{

class ComponentPool
{
public:
  virtual ~ComponentPool() = default;
  virtual void remove(Entity entity) = 0;
};

template <typename T> class ComponentPoolImpl : public ComponentPool
{
public:
  void add(Entity entity, T component)
  {
    if (entity >= components_.size())
    {
      components_.resize(entity + 1);
    }
    if (entity >= has_component_.size())
    {
      has_component_.resize(entity + 1, 0);
    }
    components_[entity] = std::move(component);
    has_component_[entity] = 1;
  }

  void remove(Entity entity) override
  {
    if (entity < has_component_.size())
    {
      // Reset to default to clear any resources
      if (has_component_[entity])
      {
        components_[entity] = T{};
        has_component_[entity] = 0;
      }
    }
  }

  [[nodiscard]] bool has(Entity entity) const
  {
    return entity < has_component_.size() && has_component_[entity] != 0;
  }

  [[nodiscard]] T *get(Entity entity)
  {
    if (entity < components_.size() && has_component_[entity])
    {
      return &components_[entity];
    }
    return nullptr;
  }

  [[nodiscard]] const T *get(Entity entity) const
  {
    if (entity < components_.size() && has_component_[entity])
    {
      return &components_[entity];
    }
    return nullptr;
  }

private:
  std::vector<T> components_;
  std::vector<std::uint8_t> has_component_; // 0 = no component, 1 = has component
};

class Registry
{
public:
  Registry() : alive_entities_(MAX_ENTITIES, 0)
  {
    available_entities_.reserve(MAX_ENTITIES);
    living_entities_.reserve(MAX_ENTITIES);
    for (Entity i = 0; i < MAX_ENTITIES; ++i)
    {
      available_entities_.push_back(i);
    }
  }

  [[nodiscard]] Entity create_entity()
  {
    if (available_entities_.empty())
    {
      return INVALID_ENTITY;
    }
    Entity id = available_entities_.back();
    available_entities_.pop_back();
    alive_entities_[id] = 1;
    living_entities_.push_back(id);
    ++living_entity_count_;
    return id;
  }

  void destroy_entity(Entity entity)
  {
    assert(entity < MAX_ENTITIES && "Entity out of range");
    assert(is_alive(entity) && "Cannot destroy already dead entity");

    // Remove all components from this entity
    for (auto &[type_id, pool] : component_pools_)
    {
      pool->remove(entity);
    }

    alive_entities_[entity] = 0;

    // Remove from living_entities_ list (swap-and-pop for O(1))
    auto it = std::find(living_entities_.begin(), living_entities_.end(), entity);
    if (it != living_entities_.end())
    {
      *it = living_entities_.back();
      living_entities_.pop_back();
    }

    available_entities_.push_back(entity);
    --living_entity_count_;
  }

  template <typename T> void add_component(Entity entity, T component)
  {
    assert(is_alive(entity) && "Cannot add component to dead entity");
    assert(!has_component<T>(entity) && "Entity already has this component");
    get_or_create_pool<T>()->add(entity, std::move(component));
  }

  template <typename T> void remove_component(Entity entity)
  {
    assert(is_alive(entity) && "Cannot remove component from dead entity");
    get_or_create_pool<T>()->remove(entity);
  }

  template <typename T> [[nodiscard]] bool has_component(Entity entity) const
  {
    auto it = component_pools_.find(typeid(T).hash_code());
    if (it != component_pools_.end())
    {
      auto *pool = static_cast<const ComponentPoolImpl<T> *>(it->second.get());
      return pool->has(entity);
    }
    return false;
  }

  template <typename T> [[nodiscard]] T *get_component(Entity entity)
  {
    assert(is_alive(entity) && "Accessing component on dead entity");
    auto it = component_pools_.find(typeid(T).hash_code());
    if (it != component_pools_.end())
    {
      auto *pool = static_cast<ComponentPoolImpl<T> *>(it->second.get());
      return pool->get(entity);
    }
    return nullptr;
  }

  template <typename T> [[nodiscard]] const T *get_component(Entity entity) const
  {
    assert(is_alive(entity) && "Accessing component on dead entity");
    auto it = component_pools_.find(typeid(T).hash_code());
    if (it != component_pools_.end())
    {
      auto *pool = static_cast<const ComponentPoolImpl<T> *>(it->second.get());
      return pool->get(entity);
    }
    return nullptr;
  }

  // Optimized view - iterates only living entities
  template <typename... Components> [[nodiscard]] std::vector<Entity> view() const
  {
    std::vector<Entity> result;
    result.reserve(living_entity_count_);

    for (Entity entity : living_entities_)
    {
      if ((has_component<Components>(entity) && ...))
      {
        result.push_back(entity);
      }
    }
    return result;
  }

  // For-each helper - much cleaner for systems
  template <typename... Components, typename Func> void each(Func &&func)
  {
    for (Entity entity : living_entities_)
    {
      if ((has_component<Components>(entity) && ...))
      {
        func(entity, *get_component<Components>(entity)...);
      }
    }
  }

  template <typename... Components, typename Func> void each(Func &&func) const
  {
    for (Entity entity : living_entities_)
    {
      if ((has_component<Components>(entity) && ...))
      {
        func(entity, *get_component<Components>(entity)...);
      }
    }
  }

  [[nodiscard]] std::size_t entity_count() const
  {
    return living_entity_count_;
  }

  [[nodiscard]] const std::vector<Entity> &living_entities() const
  {
    return living_entities_;
  }

private:
  [[nodiscard]] bool is_alive(Entity entity) const
  {
    return entity < MAX_ENTITIES && alive_entities_[entity] != 0;
  }

  template <typename T> ComponentPoolImpl<T> *get_or_create_pool()
  {
    std::size_t type_id = typeid(T).hash_code();
    auto it = component_pools_.find(type_id);
    if (it == component_pools_.end())
    {
      auto pool = std::make_unique<ComponentPoolImpl<T>>();
      auto *raw_pool = pool.get();
      component_pools_[type_id] = std::move(pool);
      return raw_pool;
    }
    return static_cast<ComponentPoolImpl<T> *>(it->second.get());
  }

  std::unordered_map<std::size_t, std::unique_ptr<ComponentPool>> component_pools_;
  std::vector<Entity> available_entities_;
  std::vector<Entity> living_entities_;      // List of currently alive entities
  std::vector<std::uint8_t> alive_entities_; // 0 = dead, 1 = alive
  std::size_t living_entity_count_{0};
};

} // namespace breakout::ecs
