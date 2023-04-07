#include "renderer/components/renderer_component.h"

#include <d3d12.h>

#include "renderer/logger.h"
#include "renderer/renderer.h"
#include "renderer/graphics/texture.h"

void RR::RendererComponent::Init(const Renderer* renderer, 
    uint32_t pipeline_type, int geometry) 
{
  if (_initialized) {
    return;
  }

  _mvp_constant_buffers = std::vector<ID3D12Resource*>(renderer->kSwapchainBufferCount);
  _material_constant_buffers = std::vector<ID3D12Resource*>(renderer->kSwapchainBufferCount);
  _srv_descriptor_heaps = std::vector<ID3D12DescriptorHeap*>(renderer->kSwapchainBufferCount);

  D3D12_HEAP_PROPERTIES mvp_cb_properties = {};
  mvp_cb_properties.Type = D3D12_HEAP_TYPE_UPLOAD;
  mvp_cb_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  mvp_cb_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

  D3D12_RESOURCE_DESC mvp_cb_desc = {};
  mvp_cb_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  mvp_cb_desc.Alignment = 0;
  mvp_cb_desc.Width = (sizeof(RR::MVPStruct) + 255) & ~255;
  mvp_cb_desc.Height = 1;
  mvp_cb_desc.DepthOrArraySize = 1;
  mvp_cb_desc.MipLevels = 1;
  mvp_cb_desc.Format = DXGI_FORMAT_UNKNOWN;
  mvp_cb_desc.SampleDesc.Count = 1;
  mvp_cb_desc.SampleDesc.Quality = 0;
  mvp_cb_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  mvp_cb_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

  D3D12_HEAP_PROPERTIES material_cb_properties = {};
  material_cb_properties.Type = D3D12_HEAP_TYPE_UPLOAD;
  material_cb_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  material_cb_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

  D3D12_RESOURCE_DESC material_cb_desc = {};
  material_cb_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  material_cb_desc.Alignment = 0;
  material_cb_desc.Height = 1;
  material_cb_desc.DepthOrArraySize = 1;
  material_cb_desc.MipLevels = 1;
  material_cb_desc.Format = DXGI_FORMAT_UNKNOWN;
  material_cb_desc.SampleDesc.Count = 1;
  material_cb_desc.SampleDesc.Quality = 0;
  material_cb_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  material_cb_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

  D3D12_DESCRIPTOR_HEAP_DESC heap_desc = {};
  heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
  heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
  heap_desc.NumDescriptors = 0;

  switch (pipeline_type) { 
    case RR::PipelineTypes::kPipelineType_PBR:
      material_cb_desc.Width = (sizeof(RR::PBRSettings) + 255) & ~255;
      heap_desc.NumDescriptors = 5;
      break;
    case RR::PipelineTypes::kPipelineType_Phong:
      material_cb_desc.Width = (sizeof(RR::PhongSettings) + 255) & ~255;
      break;
  }  

  // Create constant buffer
  for (uint16_t i = 0; i < renderer->kSwapchainBufferCount; ++i) {
    HRESULT result = renderer->_device->CreateCommittedResource(
        &mvp_cb_properties, D3D12_HEAP_FLAG_NONE,
        &mvp_cb_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&_mvp_constant_buffers[i]));

    result = renderer->_device->CreateCommittedResource(
        &material_cb_properties, D3D12_HEAP_FLAG_NONE, 
        &material_cb_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&_material_constant_buffers[i]));

    if (heap_desc.NumDescriptors != 0) {
      renderer->_device->CreateDescriptorHeap(
          &heap_desc, IID_PPV_ARGS(&_srv_descriptor_heaps[i]));
    }    
  }

  _initialized = true;
  _pipeline_type = pipeline_type;
  this->geometry = geometry;
}

void RR::RendererComponent::CreateResourceViews(
    ID3D12Device* device, std::vector<GFX::Texture>& textures, uint32_t frame) {
  switch (_pipeline_type) {
    case RR::PipelineTypes::kPipelineType_PBR: {
      settings.pbr_settings.base_color_texture = 
          textureSettings.pbr_textures.base_color != -1;
      
      settings.pbr_settings.metallic_texture = 
          textureSettings.pbr_textures.metallic != -1;
      
      settings.pbr_settings.normal_texture = 
          textureSettings.pbr_textures.normal != -1;

      settings.pbr_settings.roughness_texture = 
          textureSettings.pbr_textures.roughness != -1;

      settings.pbr_settings.reflectance_texture = 
          textureSettings.pbr_textures.reflectance != -1;

      unsigned int descriptor_size = device->GetDescriptorHandleIncrementSize(
          D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

      D3D12_CPU_DESCRIPTOR_HANDLE descriptor_handle(
          _srv_descriptor_heaps[frame]->GetCPUDescriptorHandleForHeapStart());

      if (settings.pbr_settings.base_color_texture) {
        textures[textureSettings.pbr_textures.base_color].CreateResourceView(
            device, descriptor_handle);
      } else {
        textures[0].CreateResourceView(device, descriptor_handle);
      }

      descriptor_handle.ptr += descriptor_size;

      if (settings.pbr_settings.metallic_texture) {
        textures[textureSettings.pbr_textures.metallic].CreateResourceView(
            device, descriptor_handle);
      } else {
        textures[0].CreateResourceView(device, descriptor_handle);
      }

      descriptor_handle.ptr += descriptor_size;

      if (settings.pbr_settings.normal_texture) {
        textures[textureSettings.pbr_textures.normal].CreateResourceView(
            device, descriptor_handle);
      } else {
        textures[0].CreateResourceView(device, descriptor_handle);
      }

      descriptor_handle.ptr += descriptor_size;

      if (settings.pbr_settings.roughness_texture) {
        textures[textureSettings.pbr_textures.roughness].CreateResourceView(
            device, descriptor_handle);
      } else {
        textures[0].CreateResourceView(device, descriptor_handle);
      }

      descriptor_handle.ptr += descriptor_size;

      if (settings.pbr_settings.reflectance_texture) {
        textures[textureSettings.pbr_textures.reflectance].CreateResourceView(
            device, descriptor_handle);
      } else {
        textures[0].CreateResourceView(device, descriptor_handle);
      }
      break;
    }      
  }
}

uint64_t RR::RendererComponent::MVPConstantBufferView(uint32_t frame) {
  return _mvp_constant_buffers[frame]->GetGPUVirtualAddress();
}

uint64_t RR::RendererComponent::MaterialConstantBufferView(uint32_t frame) {
  return _material_constant_buffers[frame]->GetGPUVirtualAddress();
}

void RR::RendererComponent::Update(const MVPStruct& mvp, uint32_t frame) {
  UINT* buffer_start = nullptr;
  _mvp_constant_buffers[frame]->Map(0, nullptr, reinterpret_cast<void**>(&buffer_start));
  memcpy(buffer_start, &mvp, sizeof(RR::MVPStruct));
  _mvp_constant_buffers[frame]->Unmap(0, nullptr);

  switch (_pipeline_type) { 
    case RR::PipelineTypes::kPipelineType_PBR:
      _material_constant_buffers[frame]->Map(0, nullptr, reinterpret_cast<void**>(&buffer_start));
      memcpy(buffer_start, &this->settings.pbr_settings, sizeof(RR::PBRSettings));
      _material_constant_buffers[frame]->Unmap(0, nullptr);
      break;
    case RR::PipelineTypes::kPipelineType_Phong:
      _material_constant_buffers[frame]->Map(0, nullptr, reinterpret_cast<void**>(&buffer_start));
      memcpy(buffer_start, &this->settings.phong_settings, sizeof(RR::PhongSettings));
      _material_constant_buffers[frame]->Unmap(0, nullptr);
      break;
  }
}
