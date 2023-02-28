#ifndef __GEOMETRY_H__
#define __GEOMETRY_H__ 1

#include <cstdint>
#include <memory>

#include "renderer/graphics/graphic_resource.h"

struct ID3D12Device;
struct ID3D12Resource;
struct ID3D12GraphicsCommandList;
struct D3D12_VERTEX_BUFFER_VIEW;
struct D3D12_INDEX_BUFFER_VIEW;

namespace RR {
struct GeometryData;
namespace GFX {
class Geometry : public GraphicResource {
 public:
  Geometry();
  ~Geometry() = default;

  uint32_t Indices() const;
  uint32_t Stride() const;
  uint32_t Type() const;
  const D3D12_VERTEX_BUFFER_VIEW* VertexView() const;
  const D3D12_INDEX_BUFFER_VIEW* IndexView() const;

  int Init(ID3D12Device* device, uint32_t geometry_type,
           std::shared_ptr<GeometryData> data);
  int Update(ID3D12GraphicsCommandList* command_list);

  void Release() override;

 private:
  uint32_t _type = 0U;
  uint32_t _indices = 0U;
  std::shared_ptr<GeometryData> _new_data = nullptr;

  ID3D12Resource* _vertex_default_buffer = nullptr;
  ID3D12Resource* _vertex_upload_buffer = nullptr;
  ID3D12Resource* _index_default_buffer = nullptr;
  ID3D12Resource* _index_upload_buffer = nullptr;
  std::unique_ptr<D3D12_VERTEX_BUFFER_VIEW> _vertex_buffer_view = nullptr;
  std::unique_ptr<D3D12_INDEX_BUFFER_VIEW> _index_buffer_view = nullptr;
};
}
}


#endif  // !__GEOMETRY_H__
