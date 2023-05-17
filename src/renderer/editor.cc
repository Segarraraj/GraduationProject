#include "renderer/editor.h"

#include "Imgui/imgui.h"

#include "renderer/entity.h"
#include "renderer/common.hpp"
#include "renderer/graphics/geometry.h"
#include "renderer/graphics/pipeline.h"
#include "renderer/graphics/texture.h"

#include "renderer/components/camera_component.h"
#include "renderer/components/local_transform_component.h"

static void AddHierarchyTreeNode(
    std::map<std::shared_ptr<RR::Entity>, std::shared_ptr<RR::Entity>>& parent_child, 
    std::shared_ptr<RR::Entity> entity, std::shared_ptr<RR::Entity> selected_entity,
    ImGuiTreeNodeFlags base_flags, std::shared_ptr<RR::Entity>* node_clicked, int* id) {

  ImGuiTreeNodeFlags node_flags = base_flags;
  const bool is_selected = entity == selected_entity;
  if (is_selected) {
    node_flags |= ImGuiTreeNodeFlags_Selected;
  }

  if (parent_child.count(entity) == 0) {
    node_flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
  }

  (*id)++;

  bool node_open = ImGui::TreeNodeEx((void*)id, node_flags, "%d", *id);
  if (ImGui::IsItemClicked() && !ImGui::IsItemToggledOpen()) {
    *node_clicked = entity;
  }

  if (node_open && ((node_flags & ImGuiTreeNodeFlags_Leaf) == 0)) {
    if (parent_child.count(entity) != 0) {
      AddHierarchyTreeNode(parent_child, parent_child[entity], selected_entity,
                           base_flags, node_clicked, id);
    }
    ImGui::TreePop();
  }
}

void RR::Editor::ShowEditor(
  std::list<std::shared_ptr<RR::Entity>>* entities,
  std::map<uint32_t, RR::GFX::Pipeline>* pipelines,
  const std::vector<RR::GFX::Geometry>* geometries,
  const std::vector<RR::GFX::Texture>* textures) {

  bool editor = true;

  ImGui::Begin("Hierarchy", &editor);
  std::map<std::shared_ptr<RR::Entity>, std::shared_ptr<RR::Entity>>
      parent_child;
  for (std::list<std::shared_ptr<RR::Entity>>::iterator i = entities->begin();
    i != entities->end(); i++) {
    
    std::shared_ptr<RR::LocalTransform> lt = 
      std::static_pointer_cast<RR::LocalTransform>(
        i->get()->GetComponent(RR::ComponentTypes::kComponentType_LocalTransform));
  
    if (lt != nullptr && lt->parent != nullptr) {
      parent_child[lt->parent] = *i;
    }
  }
  
  ImGuiTreeNodeFlags base_flags =
      ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_OpenOnDoubleClick |
      ImGuiTreeNodeFlags_SpanAvailWidth;
  std::shared_ptr<RR::Entity> node_clicked = nullptr;
  
  int id = 0;
  for (std::list<std::shared_ptr<RR::Entity>>::iterator i = entities->begin();
       i != entities->end(); i++) {
    AddHierarchyTreeNode(parent_child, *i, _selected_entity, base_flags, &node_clicked, &id);
  }
  
  if (node_clicked != nullptr) {
    _selected_entity = node_clicked;
  }
  
  ImGui::End();

  ImGui::Begin("Details", &editor);

  if (_selected_entity != nullptr) {
    for (uint32_t i = 0; i < RR::ComponentTypes::kComponentTYpe_Count; i++) {
      switch (i) {
        case RR::ComponentTypes::kComponentType_Camera: {
          std::shared_ptr<RR::Camera> camera =
              std::static_pointer_cast<RR::Camera>(
                  _selected_entity->GetComponent(i));

          if (camera == nullptr) {
            break;
          }

          ImGui::SeparatorText("Camera Component");
          ImGui::DragFloat("Fov", &camera->fov, .1f, 25.0f, 100.0f);
          ImGui::DragFloat("NearZ", &camera->nearZ, .001f, 0.0001f, 1.0f);
          ImGui::DragFloat("FarZ", &camera->farZ, .1f, 0.0f, 9999.0f);
          ImGui::ColorEdit3("Clear Color", camera->clear_color);
          break;
        }
      }
    }
  }



  ImGui::End();

  ImGui::Begin("Renderer properties", NULL);
  RR::GFX::Pipeline& pbr_pipeline = (*pipelines)[RR::PipelineTypes::kPipelineType_PBR];
  ImGui::SeparatorText("PBR Rendering properties");  
  ImGui::DragFloat("Ambient Light Intensity",
                   &pbr_pipeline.properties.pbr_constants.ambient_intensity,
                   0.001f, 0.0f, 1.0f);

  ImGui::DragFloat3("Directional Light 'Position'",
                   pbr_pipeline.properties.pbr_constants.directional_light_position,
                   0.01f);

  ImGui::End();
  

  ImGui::ShowDemoWindow();
}