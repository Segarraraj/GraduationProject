#include "renderer/geometry.h"

#include <d3d12.h>

#include "renderer/common.hpp"

RR::Geometry::Geometry() {}

uint32_t RR::Geometry::Indices() const { return _indices; }

uint32_t RR::Geometry::Stride() const { 
  switch (_type) {
    case RR::kGeometryType_Positions_Normals:
      return sizeof(float) * 3 + sizeof(float) * 3;
      break;
    case RR::kGeometryType_Positions_Normals_UV:
      return sizeof(float) * 3 + sizeof(float) * 3 + sizeof(float) * 2;
      break;
    default:
      return 0;
      break;
  }
}

uint32_t RR::Geometry::Type() const { return _type; }

const D3D12_VERTEX_BUFFER_VIEW* RR::Geometry::VertexView() const {
  return _vertex_buffer_view.get();
}

const D3D12_INDEX_BUFFER_VIEW* RR::Geometry::IndexView() const {
  return _index_buffer_view.get();
}

int RR::Geometry::Init(ID3D12Device* device, uint32_t geometry_type, std::shared_ptr<RR::GeometryData> data) {
  if (_initialized) {
    return 1;
  }

  if (data == nullptr) {
    return 1;
  }

  HRESULT result = {};

  D3D12_HEAP_PROPERTIES default_heap_properties = {};
  default_heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
  default_heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  default_heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

  D3D12_HEAP_PROPERTIES upload_heap_properties = {};
  upload_heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;
  upload_heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  upload_heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

  D3D12_RESOURCE_DESC buffer_resource_desc = {};
  buffer_resource_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  buffer_resource_desc.Alignment = 0;
  buffer_resource_desc.Width = data->vertex_size;
  buffer_resource_desc.Height = 1;
  buffer_resource_desc.DepthOrArraySize = 1;
  buffer_resource_desc.MipLevels = 1;
  buffer_resource_desc.Format = DXGI_FORMAT_UNKNOWN;
  buffer_resource_desc.SampleDesc.Count = 1;
  buffer_resource_desc.SampleDesc.Quality = 0;
  buffer_resource_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  buffer_resource_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

  result = device->CreateCommittedResource(
      &default_heap_properties, D3D12_HEAP_FLAG_NONE, &buffer_resource_desc,
      D3D12_RESOURCE_STATE_COMMON, nullptr,
      IID_PPV_ARGS(&_vertex_default_buffer));

  if (FAILED(result)) {
    return 1;
  }

  result = device->CreateCommittedResource(
      &upload_heap_properties, D3D12_HEAP_FLAG_NONE, &buffer_resource_desc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
      IID_PPV_ARGS(&_vertex_upload_buffer));

  if (FAILED(result)) {
    return 1;
  }

  buffer_resource_desc.Width = data->index_size;
  result = device->CreateCommittedResource(
      &default_heap_properties, D3D12_HEAP_FLAG_NONE, &buffer_resource_desc,
      D3D12_RESOURCE_STATE_COMMON, nullptr,
      IID_PPV_ARGS(&_index_default_buffer));

  if (FAILED(result)) {
    return 1;
  }

  result = device->CreateCommittedResource(
      &upload_heap_properties, D3D12_HEAP_FLAG_NONE, &buffer_resource_desc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
      IID_PPV_ARGS(&_index_upload_buffer));

  if (FAILED(result)) {
    return 1;
  }

  _initialized = true;
  _updated = false;
  _indices = data->index_size / sizeof(uint32_t);
  _type = geometry_type;
  // FIXME: this is not safe as user could release 
  // data->vertex_data or whatever
  _new_data = data;

  _vertex_buffer_view = std::make_unique<D3D12_VERTEX_BUFFER_VIEW>();
  _vertex_buffer_view->BufferLocation =
      _vertex_default_buffer->GetGPUVirtualAddress();
  _vertex_buffer_view->SizeInBytes = data->vertex_size;
  _vertex_buffer_view->StrideInBytes = Stride();

  _index_buffer_view = std::make_unique<D3D12_INDEX_BUFFER_VIEW>();
  _index_buffer_view->BufferLocation =
      _index_default_buffer->GetGPUVirtualAddress();
  _index_buffer_view->SizeInBytes = data->index_size;
  _index_buffer_view->Format = DXGI_FORMAT_R32_UINT;

  return 0;
}

int RR::Geometry::Update(ID3D12GraphicsCommandList* command_list) {
  if (!_initialized) {
    return 1;
  }

  if (_updated) {
    return 1;
  }

  if (command_list == nullptr) {
    return 1;
  }

  UINT8* upload_resource_heap_begin;

  // Copy data to upload resource heap
  _vertex_upload_buffer->Map(
      0, nullptr, reinterpret_cast<void**>(&upload_resource_heap_begin));
  memcpy(upload_resource_heap_begin, _new_data->vertex_data, _new_data->vertex_size);
  _vertex_upload_buffer->Unmap(0, nullptr);

  _index_upload_buffer->Map(
      0, nullptr, reinterpret_cast<void**>(&upload_resource_heap_begin));
  memcpy(upload_resource_heap_begin, _new_data->index_data, _new_data->index_size);
  _index_upload_buffer->Unmap(0, nullptr);

  D3D12_RESOURCE_BARRIER vb_upload_resource_heap_barrier = {};
  vb_upload_resource_heap_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  vb_upload_resource_heap_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  vb_upload_resource_heap_barrier.Transition.pResource = _vertex_default_buffer;
  vb_upload_resource_heap_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
  vb_upload_resource_heap_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
  vb_upload_resource_heap_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

  D3D12_RESOURCE_BARRIER index_upload_resource_heap_barrier = {};
  index_upload_resource_heap_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  index_upload_resource_heap_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  index_upload_resource_heap_barrier.Transition.pResource = _index_default_buffer;
  index_upload_resource_heap_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
  index_upload_resource_heap_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
  index_upload_resource_heap_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

  command_list->ResourceBarrier(1, &vb_upload_resource_heap_barrier);
  command_list->ResourceBarrier(1, &index_upload_resource_heap_barrier);
  command_list->CopyResource(_vertex_default_buffer, _vertex_upload_buffer);
  command_list->CopyResource(_index_default_buffer, _index_upload_buffer);
  vb_upload_resource_heap_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
  vb_upload_resource_heap_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_VERTEX_AND_CONSTANT_BUFFER;
  index_upload_resource_heap_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
  index_upload_resource_heap_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_INDEX_BUFFER;
  command_list->ResourceBarrier(1, &vb_upload_resource_heap_barrier);
  command_list->ResourceBarrier(1, &index_upload_resource_heap_barrier);

  _updated = true;

  return 0;
}

void RR::Geometry::Release() {
  if (!_initialized) {
    return;
  }

  if (_vertex_default_buffer != nullptr) {
    _vertex_default_buffer->Release();
    _vertex_default_buffer = nullptr;
  }

  if (_vertex_upload_buffer != nullptr) {
    _vertex_upload_buffer->Release();
    _vertex_upload_buffer = nullptr;
  }

  if (_index_default_buffer != nullptr) {
    _index_default_buffer->Release();
    _index_default_buffer = nullptr;
  }

  if (_index_upload_buffer != nullptr) {
    _index_upload_buffer->Release();
    _index_upload_buffer = nullptr;
  }

  if (_vertex_buffer_view != nullptr) {
    _vertex_buffer_view.release();
    _vertex_buffer_view = nullptr;
  }

  if (_index_buffer_view != nullptr) {
    _index_buffer_view.release();
    _index_buffer_view = nullptr;
  }

  if (_new_data != nullptr) {
    _new_data.reset();
    _new_data = nullptr;
  }
}
