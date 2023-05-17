#ifndef __EDITOR_H__
#define __EDITOR_H__ 1

#include <vector>
#include <list>
#include <memory>
#include <map>




namespace RR {
class Entity;
class Renderer;

namespace GFX {
class Texture;
class Geometry;
class Pipeline;
}
  
class Editor {
 public:
  Editor() = default;
  ~Editor() = default;

  void Init() { };
  void ShowEditor(std::list<std::shared_ptr<RR::Entity>>* entities,
                  std::map<uint32_t, GFX::Pipeline>* pipelines,
                  const std::vector<GFX::Geometry>* geometries,
                  const std::vector<GFX::Texture>* textures);

 private:
  std::shared_ptr<RR::Entity> _selected_entity = nullptr;

  bool _show_hierarchy_window = true;
  bool _show_details_window = true;
  bool _show_properties_window = true;
};
}

#endif  // !__EDITOR_H__
