#include "renderer/components/renderer_component.h"

#include <d3d12.h>

#include "renderer/logger.h"
#include "renderer/renderer.h"

void RR::RendererComponent::Init(const Renderer* renderer,
                                 uint32_t pipeline_type, int geometry) {

  if (_initialized) {
    return;
  }

  D3D12_HEAP_PROPERTIES constant_buffer_properties = {};
  constant_buffer_properties.Type = D3D12_HEAP_TYPE_UPLOAD;
  constant_buffer_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  constant_buffer_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

  D3D12_RESOURCE_DESC constant_buffer_desc = {};
  constant_buffer_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  constant_buffer_desc.Alignment = 0;
  constant_buffer_desc.Height = 1;
  constant_buffer_desc.DepthOrArraySize = 1;
  constant_buffer_desc.MipLevels = 1;
  constant_buffer_desc.Format = DXGI_FORMAT_UNKNOWN;
  constant_buffer_desc.SampleDesc.Count = 1;
  constant_buffer_desc.SampleDesc.Quality = 0;
  constant_buffer_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  constant_buffer_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

  switch (pipeline_type) { 
    case RR::PipelineTypes::kPipelineType_PBR:
      constant_buffer_desc.Width = (sizeof(RR::MVPStruct) + 255) & ~255;
      break;
  }

  _constant_buffers =
      std::vector<ID3D12Resource*>(renderer->kSwapchainBufferCount);

  // Create constant buffer
  for (uint16_t i = 0; i < renderer->kSwapchainBufferCount; ++i) {
    HRESULT result = renderer->_device->CreateCommittedResource(
        &constant_buffer_properties, D3D12_HEAP_FLAG_NONE,
        &constant_buffer_desc, D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
        IID_PPV_ARGS(&_constant_buffers[i]));
  }

  _initialized = true;
  _pipeline_type = pipeline_type;
  this->geometry = geometry;
}

uint64_t RR::RendererComponent::ConstantBufferView(uint32_t frame) {
  return _constant_buffers[frame]->GetGPUVirtualAddress();
}

void RR::RendererComponent::Update(const RendererSettings& settings, uint32_t frame) {
  switch (_pipeline_type) { 
    case RR::PipelineTypes::kPipelineType_PBR:
      this->settings.pbr_settings.mvp.model = settings.pbr_settings.mvp.model;
      this->settings.pbr_settings.mvp.view = settings.pbr_settings.mvp.view;
      this->settings.pbr_settings.mvp.projection = settings.pbr_settings.mvp.projection;

      UINT* buffer_start = nullptr;

      _constant_buffers[frame]->Map(0, nullptr, reinterpret_cast<void**>(&buffer_start));
      memcpy(buffer_start, &settings.pbr_settings.mvp, sizeof(RR::MVPStruct));
      _constant_buffers[frame]->Unmap(0, nullptr);
      break;
  }
}
