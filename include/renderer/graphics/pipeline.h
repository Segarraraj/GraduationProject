#ifndef __PIPELINE_H__
#define __PIPELINE_H__ 1

#include <cstdint>

#include "renderer/graphics/graphic_resource.h"
  
struct ID3D12PipelineState;
struct ID3D12RootSignature;
struct ID3D12Device;

namespace RR {
namespace GFX {
class Pipeline : public GraphicResource {
 public:
  Pipeline() = default;
  ~Pipeline() = default;

  int Init(ID3D12Device* device, uint32_t type, uint32_t geometry_type);
  void Release() override;

  ID3D12PipelineState* PipelineState();
  ID3D12RootSignature* RootSignature();
  uint32_t GeometryType(); 

 private:
  uint32_t _type = 0U;
  uint32_t _geometry_type = 0U;
  ID3D12PipelineState* _pipeline_state = nullptr;
  ID3D12RootSignature* _root_signature = nullptr;
};
}
}

#endif  // !__PIPELINE_H__
