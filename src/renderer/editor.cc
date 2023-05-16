#include "renderer/editor.h"

#include "Imgui/imgui.h"

#include "renderer/entity.h"
#include "renderer/common.hpp"
#include "renderer/graphics/geometry.h"
#include "renderer/graphics/pipeline.h"
#include "renderer/graphics/texture.h"

void RR::Editor::ShowEditor(
  std::list<std::shared_ptr<RR::Entity>>* entities,
  std::map<uint32_t, RR::GFX::Pipeline>* pipelines,
  const std::vector<RR::GFX::Geometry>* geometries,
  const std::vector<RR::GFX::Texture>* textures) {

  if (ImGui::Begin("Hierarchy", &_show_hierarchy_window)) {

    ImGui::End();
  }

  if (ImGui::Begin("Details", &_show_details_window)) {
  
    ImGui::End();
  }

  if (ImGui::Begin("Renderer properties", &_show_properties_window)) {
    RR::GFX::Pipeline& pbr_pipeline =
        (*pipelines)[RR::PipelineTypes::kPipelineType_PBR];
    ImGui::SeparatorText("PBR Rendering properties");
    
    ImGui::DragFloat("Ambient Light Intensity",
                     &pbr_pipeline.properties.pbr_constants.ambient_intensity,
                     0.001f, 0.0f, 1.0f);

    ImGui::DragFloat3("Directional Light 'Position'",
                     pbr_pipeline.properties.pbr_constants.directional_light_position,
                     0.01f);

    ImGui::End();
  }

  ImGui::ShowDemoWindow();
}