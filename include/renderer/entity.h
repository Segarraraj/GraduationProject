#ifndef __ENTITY_H__
#define __ENTITY_H__ 1

#include <map>
#include <memory>

namespace RR {
class EntityComponent;
class Renderer;

class Entity {
 public:
  Entity() = default;
  Entity(uint32_t components);
  
  Entity(Entity&&) = delete;
  void operator=(Entity&&) = delete;

  std::shared_ptr<EntityComponent> GetComponent(uint32_t component_type) const;

  ~Entity() = default;

  friend class Renderer;
  friend class LocalTransform;

 private:
  std::map<uint32_t, std::shared_ptr<EntityComponent>> _components;
};
}

#endif  // !__ENTITY_H__
