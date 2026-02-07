#pragma once

#include "component.hpp"
#include <algorithm>
#include <cassert>
#include <cstddef>
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
      has_component_.resize(entity + 1);
    }
    components_[entity] = std::move(component);
    has_component_[entity] = true;
  }

  void remove(Entity entity) override
  {
    if (entity < has_component_.size())
    {
      has_component_[entity] = false;
    }
  }

  [[nodiscard]] bool has(Entity entity) const
  {
    return entity < has_component_.size() && has_component_[entity];
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
  std::vector<bool> has_component_;
};

class Registry
{
public:
  Registry() : alive_entities_(MAX_ENTITIES, false)
  {
    available_entities_.reserve(MAX_ENTITIES);
    for (Entity i = 0; i < MAX_ENTITIES; ++i)
    {
      available_entities_.push_back(i);
    }
  }

  [[nodiscard]] Entity create_entity()
  {
    assert(!available_entities_.empty() && "Entity limit reached");
    Entity id = available_entities_.back();
    available_entities_.pop_back();
    alive_entities_[id] = true;
    ++living_entity_count_;
    return id;
  }

  void destroy_entity(Entity entity)
  {
    assert(entity < MAX_ENTITIES && "Entity out of range");

    for (auto &[type_id, pool] : component_pools_)
    {
      pool->remove(entity);
    }

    alive_entities_[entity] = false;
    available_entities_.push_back(entity);
    --living_entity_count_;
  }

  template <typename T> void add_component(Entity entity, T component)
  {
    get_or_create_pool<T>()->add(entity, std::move(component));
  }

  template <typename T> void remove_component(Entity entity)
  {
    get_or_create_pool<T>()->remove(entity);
  }

  template <typename T> [[nodiscard]] bool has_component(Entity entity) const
  {
    auto it = component_pools_.find(typeid(T).hash_code());
    if (it != component_pools_.end())
    {
      auto *pool = static_cast<ComponentPoolImpl<T> *>(it->second.get());
      return pool->has(entity);
    }
    return false;
  }

  template <typename T> [[nodiscard]] T *get_component(Entity entity)
  {
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
    auto it = component_pools_.find(typeid(T).hash_code());
    if (it != component_pools_.end())
    {
      auto *pool = static_cast<const ComponentPoolImpl<T> *>(it->second.get());
      return pool->get(entity);
    }
    return nullptr;
  }

  template <typename... Components> [[nodiscard]] std::vector<Entity> view() const
  {
    std::vector<Entity> result;
    result.reserve(living_entity_count_);

    for (Entity i = 0; i < MAX_ENTITIES; ++i)
    {
      if (is_alive(i) && (has_component<Components>(i) && ...))
      {
        result.push_back(i);
      }
    }
    return result;
  }

  [[nodiscard]] std::size_t entity_count() const
  {
    return living_entity_count_;
  }

private:
  [[nodiscard]] bool is_alive(Entity entity) const
  {
    return entity < MAX_ENTITIES && alive_entities_[entity];
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
  std::vector<bool> alive_entities_;
  std::size_t living_entity_count_{0};
};

} // namespace breakout::ecs