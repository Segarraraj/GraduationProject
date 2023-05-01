#ifndef __TEXTURE_H__
#define __TEXTURE_H__ 1

#include "renderer/graphics/graphic_resource.h"

#include <vector>
#include <memory>

struct ID3D12Device;
struct ID3D12Resource;
struct ID3D12GraphicsCommandList;
struct D3D12_RESOURCE_DESC;
struct D3D12_CPU_DESCRIPTOR_HANDLE;

namespace RR {
namespace GFX {
class Texture : public GraphicResource {
 public:
  Texture() = default;
  ~Texture() = default;

  int Init(ID3D12Device* device, const wchar_t* file_name);
  int Update(ID3D12Device* device, ID3D12GraphicsCommandList* command_list);
  void CreateResourceView(ID3D12Device* device, D3D12_CPU_DESCRIPTOR_HANDLE& handle);

  void Release() override;

 private:
  ID3D12Resource* _default_buffer = nullptr;
  ID3D12Resource* _upload_buffer = nullptr;

  std::unique_ptr<D3D12_RESOURCE_DESC> _texture_desc = nullptr;
  int _image_byte_row = -1;
  int _image_size = -1;
  std::vector<unsigned char> _texture_data;
};
}
}

#endif  // !__TEXTURE_H__
