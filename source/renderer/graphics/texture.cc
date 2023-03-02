#include "renderer/graphics/texture.h"

#include <d3d12.h>
#include <wincodec.h>

#include "renderer/logger.h"

static DXGI_FORMAT GetDXGIFormatFromWICFormat(
    WICPixelFormatGUID& wicFormatGUID) {
  if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFloat)
    return DXGI_FORMAT_R32G32B32A32_FLOAT;
  else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAHalf)
    return DXGI_FORMAT_R16G16B16A16_FLOAT;
  else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBA)
    return DXGI_FORMAT_R16G16B16A16_UNORM;
  else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA)
    return DXGI_FORMAT_R8G8B8A8_UNORM;
  else if (wicFormatGUID == GUID_WICPixelFormat32bppBGRA)
    return DXGI_FORMAT_B8G8R8A8_UNORM;
  else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR)
    return DXGI_FORMAT_B8G8R8X8_UNORM;
  else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102XR)
    return DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM;
  else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBA1010102)
    return DXGI_FORMAT_R10G10B10A2_UNORM;
  else if (wicFormatGUID == GUID_WICPixelFormat16bppBGRA5551)
    return DXGI_FORMAT_B5G5R5A1_UNORM;
  else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR565)
    return DXGI_FORMAT_B5G6R5_UNORM;
  else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFloat)
    return DXGI_FORMAT_R32_FLOAT;
  else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayHalf)
    return DXGI_FORMAT_R16_FLOAT;
  else if (wicFormatGUID == GUID_WICPixelFormat16bppGray)
    return DXGI_FORMAT_R16_UNORM;
  else if (wicFormatGUID == GUID_WICPixelFormat8bppGray)
    return DXGI_FORMAT_R8_UNORM;
  else if (wicFormatGUID == GUID_WICPixelFormat8bppAlpha)
    return DXGI_FORMAT_A8_UNORM;

  else
    return DXGI_FORMAT_UNKNOWN;
}

WICPixelFormatGUID GetConvertToWICFormat(WICPixelFormatGUID& wicFormatGUID) {
  if (wicFormatGUID == GUID_WICPixelFormatBlackWhite)
    return GUID_WICPixelFormat8bppGray;
  else if (wicFormatGUID == GUID_WICPixelFormat1bppIndexed)
    return GUID_WICPixelFormat32bppRGBA;
  else if (wicFormatGUID == GUID_WICPixelFormat2bppIndexed)
    return GUID_WICPixelFormat32bppRGBA;
  else if (wicFormatGUID == GUID_WICPixelFormat4bppIndexed)
    return GUID_WICPixelFormat32bppRGBA;
  else if (wicFormatGUID == GUID_WICPixelFormat8bppIndexed)
    return GUID_WICPixelFormat32bppRGBA;
  else if (wicFormatGUID == GUID_WICPixelFormat2bppGray)
    return GUID_WICPixelFormat8bppGray;
  else if (wicFormatGUID == GUID_WICPixelFormat4bppGray)
    return GUID_WICPixelFormat8bppGray;
  else if (wicFormatGUID == GUID_WICPixelFormat16bppGrayFixedPoint)
    return GUID_WICPixelFormat16bppGrayHalf;
  else if (wicFormatGUID == GUID_WICPixelFormat32bppGrayFixedPoint)
    return GUID_WICPixelFormat32bppGrayFloat;
  else if (wicFormatGUID == GUID_WICPixelFormat16bppBGR555)
    return GUID_WICPixelFormat16bppBGRA5551;
  else if (wicFormatGUID == GUID_WICPixelFormat32bppBGR101010)
    return GUID_WICPixelFormat32bppRGBA1010102;
  else if (wicFormatGUID == GUID_WICPixelFormat24bppBGR)
    return GUID_WICPixelFormat32bppRGBA;
  else if (wicFormatGUID == GUID_WICPixelFormat24bppRGB)
    return GUID_WICPixelFormat32bppRGBA;
  else if (wicFormatGUID == GUID_WICPixelFormat32bppPBGRA)
    return GUID_WICPixelFormat32bppRGBA;
  else if (wicFormatGUID == GUID_WICPixelFormat32bppPRGBA)
    return GUID_WICPixelFormat32bppRGBA;
  else if (wicFormatGUID == GUID_WICPixelFormat48bppRGB)
    return GUID_WICPixelFormat64bppRGBA;
  else if (wicFormatGUID == GUID_WICPixelFormat48bppBGR)
    return GUID_WICPixelFormat64bppRGBA;
  else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRA)
    return GUID_WICPixelFormat64bppRGBA;
  else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBA)
    return GUID_WICPixelFormat64bppRGBA;
  else if (wicFormatGUID == GUID_WICPixelFormat64bppPBGRA)
    return GUID_WICPixelFormat64bppRGBA;
  else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBFixedPoint)
    return GUID_WICPixelFormat64bppRGBAHalf;
  else if (wicFormatGUID == GUID_WICPixelFormat48bppBGRFixedPoint)
    return GUID_WICPixelFormat64bppRGBAHalf;
  else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBAFixedPoint)
    return GUID_WICPixelFormat64bppRGBAHalf;
  else if (wicFormatGUID == GUID_WICPixelFormat64bppBGRAFixedPoint)
    return GUID_WICPixelFormat64bppRGBAHalf;
  else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBFixedPoint)
    return GUID_WICPixelFormat64bppRGBAHalf;
  else if (wicFormatGUID == GUID_WICPixelFormat64bppRGBHalf)
    return GUID_WICPixelFormat64bppRGBAHalf;
  else if (wicFormatGUID == GUID_WICPixelFormat48bppRGBHalf)
    return GUID_WICPixelFormat64bppRGBAHalf;
  else if (wicFormatGUID == GUID_WICPixelFormat128bppPRGBAFloat)
    return GUID_WICPixelFormat128bppRGBAFloat;
  else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFloat)
    return GUID_WICPixelFormat128bppRGBAFloat;
  else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBAFixedPoint)
    return GUID_WICPixelFormat128bppRGBAFloat;
  else if (wicFormatGUID == GUID_WICPixelFormat128bppRGBFixedPoint)
    return GUID_WICPixelFormat128bppRGBAFloat;
  else if (wicFormatGUID == GUID_WICPixelFormat32bppRGBE)
    return GUID_WICPixelFormat128bppRGBAFloat;
  else if (wicFormatGUID == GUID_WICPixelFormat32bppCMYK)
    return GUID_WICPixelFormat32bppRGBA;
  else if (wicFormatGUID == GUID_WICPixelFormat64bppCMYK)
    return GUID_WICPixelFormat64bppRGBA;
  else if (wicFormatGUID == GUID_WICPixelFormat40bppCMYKAlpha)
    return GUID_WICPixelFormat64bppRGBA;
  else if (wicFormatGUID == GUID_WICPixelFormat80bppCMYKAlpha)
    return GUID_WICPixelFormat64bppRGBA;

#if (_WIN32_WINNT >= _WIN32_WINNT_WIN8) || defined(_WIN7_PLATFORM_UPDATE)
  else if (wicFormatGUID == GUID_WICPixelFormat32bppRGB)
    return GUID_WICPixelFormat32bppRGBA;
  else if (wicFormatGUID == GUID_WICPixelFormat64bppRGB)
    return GUID_WICPixelFormat64bppRGBA;
  else if (wicFormatGUID == GUID_WICPixelFormat64bppPRGBAHalf)
    return GUID_WICPixelFormat64bppRGBAHalf;
#endif

  else
    return GUID_WICPixelFormatDontCare;
}

// get the number of bits per pixel for a dxgi format
static int GetDXGIFormatBitsPerPixel(DXGI_FORMAT& dxgiFormat) {
  if (dxgiFormat == DXGI_FORMAT_R32G32B32A32_FLOAT)
    return 128;
  else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_FLOAT)
    return 64;
  else if (dxgiFormat == DXGI_FORMAT_R16G16B16A16_UNORM)
    return 64;
  else if (dxgiFormat == DXGI_FORMAT_R8G8B8A8_UNORM)
    return 32;
  else if (dxgiFormat == DXGI_FORMAT_B8G8R8A8_UNORM)
    return 32;
  else if (dxgiFormat == DXGI_FORMAT_B8G8R8X8_UNORM)
    return 32;
  else if (dxgiFormat == DXGI_FORMAT_R10G10B10_XR_BIAS_A2_UNORM)
    return 32;
  else if (dxgiFormat == DXGI_FORMAT_R10G10B10A2_UNORM)
    return 32;
  else if (dxgiFormat == DXGI_FORMAT_B5G5R5A1_UNORM)
    return 16;
  else if (dxgiFormat == DXGI_FORMAT_B5G6R5_UNORM)
    return 16;
  else if (dxgiFormat == DXGI_FORMAT_R32_FLOAT)
    return 32;
  else if (dxgiFormat == DXGI_FORMAT_R16_FLOAT)
    return 16;
  else if (dxgiFormat == DXGI_FORMAT_R16_UNORM)
    return 16;
  else if (dxgiFormat == DXGI_FORMAT_R8_UNORM)
    return 8;
  else if (dxgiFormat == DXGI_FORMAT_A8_UNORM)
    return 8;
}

static int LoadImageDataFromFile(std::vector<unsigned char>* image_data,
                                 D3D12_RESOURCE_DESC* resource_description,
                                 const wchar_t* filename, int* bytes_per_row) {
  if (image_data == nullptr || resource_description == nullptr || 
    filename == nullptr || bytes_per_row == nullptr) {
    return -1;
  }

  LOG_DEBUG("RR::GFX", "Initializing texture: %S", filename);

  HRESULT result = {};

  static IWICImagingFactory* factory = nullptr;

  IWICBitmapDecoder* decoder = nullptr;
  IWICBitmapFrameDecode* frame = nullptr;
  IWICFormatConverter* converter = nullptr;

  bool converted = false;

  if (factory == nullptr) {
    CoInitialize(nullptr);

    result = CoCreateInstance(CLSID_WICImagingFactory, NULL, 
                              CLSCTX_INPROC_SERVER,
                              IID_PPV_ARGS(&factory));
    if (FAILED(result)) {
      LOG_ERROR("RR::GFX", "Couldn't create wic factory");
      return -1;
    }
  }

  result = factory->CreateDecoderFromFilename(filename, 0, GENERIC_READ,                  // We want to read from this file
                                              WICDecodeMetadataCacheOnLoad,
                                              &decoder);
  if (FAILED(result)) {
      LOG_ERROR("RR::GFX", "Couldn't create descriptor");
    return -1;
  }

  result = decoder->GetFrame(0, &frame);
  if (FAILED(result)) {
      LOG_ERROR("RR::GFX", "Couldn't get frame");
    return -1;
  }

  WICPixelFormatGUID pixel_format;
  result = frame->GetPixelFormat(&pixel_format);
  if (FAILED(result)) {
    LOG_ERROR("RR::GFX", "Couldn't get pixel format");
    return -1;
  }

  // get size of image
  UINT width, height;
  result = frame->GetSize(&width, &height);
  if (FAILED(result)) {
    LOG_ERROR("RR::GFX", "Couldn't get image size");
    return -1;
  }

  DXGI_FORMAT dxgi_format = GetDXGIFormatFromWICFormat(pixel_format);
  if (dxgi_format == DXGI_FORMAT_UNKNOWN) {
    WICPixelFormatGUID convert_to_pixel_format = GetConvertToWICFormat(pixel_format);

    if (convert_to_pixel_format == GUID_WICPixelFormatDontCare) {
      return -1;
    }

    dxgi_format = GetDXGIFormatFromWICFormat(convert_to_pixel_format);

    result = factory->CreateFormatConverter(&converter);
    if (FAILED(result)) {
      return -1;
    }

    BOOL convert = FALSE;
    result = converter->CanConvert(pixel_format, convert_to_pixel_format,
                                   &convert);
    if (FAILED(result) || !convert) {
      return -1;
    }

    result = converter->Initialize(frame, convert_to_pixel_format,
                                   WICBitmapDitherTypeErrorDiffusion, 0, 0,
                                   WICBitmapPaletteTypeCustom);
    if (FAILED(result)) {
      return -1;
    }

    converted = true;
  }

  int bits_per_pixel = GetDXGIFormatBitsPerPixel(dxgi_format);  // number of bits per pixel
  *bytes_per_row = (width * bits_per_pixel) / 8;  
  int image_size = *bytes_per_row * height;

  image_data->resize(image_size);
 
  if (converted) {
    result = converter->CopyPixels(0, *bytes_per_row, image_size, image_data->data());
    if (FAILED(result)) {
      return -1;
    }
  } else {
    result = frame->CopyPixels(0, *bytes_per_row, image_size, image_data->data());
    if (FAILED(result)) {
      return -1;
    }
  }

  *resource_description = {};
  resource_description->Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
  resource_description->Alignment = 0;
  resource_description->Width = width;
  resource_description->Height = height;
  resource_description->DepthOrArraySize = 1;
  resource_description->MipLevels = 1;
  resource_description->Format = dxgi_format;
  resource_description->SampleDesc.Count = 1;
  resource_description->SampleDesc.Quality = 0;
  resource_description->Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
  resource_description->Flags = D3D12_RESOURCE_FLAG_NONE;

  return image_size;
}

int RR::GFX::Texture::Init(ID3D12Device* device, const wchar_t* file_name) {
  if (_initialized || file_name == nullptr || device == nullptr) {
    return 1;
  }

  _texture_desc = std::make_unique<D3D12_RESOURCE_DESC>();
  _image_size = LoadImageDataFromFile(&_texture_data, _texture_desc.get(),
                                      file_name, &_image_byte_row);

  HRESULT result = {};

  D3D12_HEAP_PROPERTIES default_heap_properties = {};
  default_heap_properties.Type = D3D12_HEAP_TYPE_DEFAULT;
  default_heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  default_heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

  D3D12_HEAP_PROPERTIES upload_heap_properties = {};
  upload_heap_properties.Type = D3D12_HEAP_TYPE_UPLOAD;
  upload_heap_properties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
  upload_heap_properties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;

  result = device->CreateCommittedResource(
      &default_heap_properties, D3D12_HEAP_FLAG_NONE, _texture_desc.get(),
      D3D12_RESOURCE_STATE_COMMON, nullptr,
      IID_PPV_ARGS(&_default_buffer));

  if (FAILED(result)) {
    LOG_ERROR("RR::GFX", "Couldn't create texture default buffer");
    return -1;
  }

  UINT64 upload_buffer_size;
  device->GetCopyableFootprints(_texture_desc.get(), 0, 1, 0, 
                                nullptr, nullptr, nullptr, 
                                &upload_buffer_size);

  D3D12_RESOURCE_DESC upload_heap_desc = {};
  upload_heap_desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
  upload_heap_desc.Alignment = 0;
  upload_heap_desc.Width = upload_buffer_size;
  upload_heap_desc.Height = 1;
  upload_heap_desc.DepthOrArraySize = 1;
  upload_heap_desc.MipLevels = 1;
  upload_heap_desc.Format = DXGI_FORMAT_UNKNOWN;
  upload_heap_desc.SampleDesc.Count = 1;
  upload_heap_desc.SampleDesc.Quality = 0;
  upload_heap_desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
  upload_heap_desc.Flags = D3D12_RESOURCE_FLAG_NONE;

  result = device->CreateCommittedResource(
      &upload_heap_properties, D3D12_HEAP_FLAG_NONE, &upload_heap_desc,
      D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, IID_PPV_ARGS(&_upload_buffer));
  if (FAILED(result)) {
    LOG_ERROR("RR::GFX", "Couldn't create upload heap");
    return -1;
  }

  _initialized = true;
  _updated = false;
  return 0;
}

int RR::GFX::Texture::Update(ID3D12Device* device, ID3D12GraphicsCommandList* command_list) {
  if (!_initialized) {
    return 1;
  }

  if (_updated) {
    return 1;
  }

  if (command_list == nullptr) {
    return 1;
  }

  D3D12_BOX source_region;
  source_region.left = 0;
  source_region.top = 0;
  source_region.right = _texture_desc->Width;
  source_region.bottom = _texture_desc->Height;
  source_region.front = 0;
  source_region.back = 1;

  D3D12_TEXTURE_COPY_LOCATION default_location = {};
  default_location.pResource = _default_buffer;
  default_location.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
  default_location.SubresourceIndex = 0;

  D3D12_TEXTURE_COPY_LOCATION upload_location = {};
  upload_location.pResource = _upload_buffer;
  upload_location.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
  device->GetCopyableFootprints(_texture_desc.get(), 0, 1, 0,
                                &upload_location.PlacedFootprint, nullptr,
                                nullptr, nullptr);

  UINT8* upload_resource_heap_begin;
  _upload_buffer->Map( 0, nullptr, reinterpret_cast<void**>(&upload_resource_heap_begin));
  for (int i = 0; i < upload_location.PlacedFootprint.Footprint.Height; i++) {
    memcpy(upload_resource_heap_begin + i * upload_location.PlacedFootprint.Footprint.RowPitch,
           _texture_data.data() + i * _image_byte_row,
           _image_byte_row);
  }
  _upload_buffer->Unmap(0, nullptr);

  D3D12_RESOURCE_BARRIER vb_upload_resource_heap_barrier = {};
  vb_upload_resource_heap_barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
  vb_upload_resource_heap_barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
  vb_upload_resource_heap_barrier.Transition.pResource = _default_buffer;
  vb_upload_resource_heap_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COMMON;
  vb_upload_resource_heap_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_COPY_DEST;
  vb_upload_resource_heap_barrier.Transition.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;

  command_list->ResourceBarrier(1, &vb_upload_resource_heap_barrier);
  command_list->CopyTextureRegion(&default_location, 0, 0, 0, &upload_location,
                                  &source_region);
  vb_upload_resource_heap_barrier.Transition.StateBefore = D3D12_RESOURCE_STATE_COPY_DEST;
  vb_upload_resource_heap_barrier.Transition.StateAfter = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;
  command_list->ResourceBarrier(1, &vb_upload_resource_heap_barrier);

  _updated = true;

  return 0;
}

void RR::GFX::Texture::CreateResourceView(ID3D12Device* device, 
    D3D12_CPU_DESCRIPTOR_HANDLE& handle) {
  D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
  srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
  srv_desc.Format = _texture_desc->Format;
  srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
  srv_desc.Texture2D.MipLevels = 1;
  device->CreateShaderResourceView(_default_buffer, &srv_desc, handle);
}

void RR::GFX::Texture::Release() {
  if (_default_buffer != nullptr) {
    _default_buffer->Release();
    _default_buffer = nullptr;
  }

  if (_upload_buffer != nullptr) {
    _upload_buffer->Release();
    _upload_buffer = nullptr;
  }
}
