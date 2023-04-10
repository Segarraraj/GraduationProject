#include "renderer/graphics/pipeline.h"

#include <d3d12.h>
#include <d3d12sdklayers.h>
#include <d3dcompiler.h>

#include <vector>

#include "renderer/logger.h"
#include "renderer/common.hpp"

int RR::GFX::Pipeline::Init(ID3D12Device* device, uint32_t type,
                        uint32_t geometry_type) {
  if (_initialized) {
    return 1;
  }

  LOG_DEBUG("RR::GFX", "Creating pipeline type: %i", type);

  HRESULT result;

  D3D12_FEATURE_DATA_ROOT_SIGNATURE root_signature_feature_data = {};
  root_signature_feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
  result = device->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE,
                                       &root_signature_feature_data,
                                       sizeof(root_signature_feature_data));

  if (FAILED(result)) {
    LOG_WARNING("RR::GFX", "Current device doesn't support root signature v1.1, using v1.0");
    root_signature_feature_data.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
  }

  std::vector<D3D12_ROOT_PARAMETER1> parameters = std::vector<D3D12_ROOT_PARAMETER1>(2);
  std::vector<D3D12_STATIC_SAMPLER_DESC> samplers = std::vector<D3D12_STATIC_SAMPLER_DESC>(0);

  D3D12_ROOT_DESCRIPTOR1 mvp_cb_descriptor = {};
  mvp_cb_descriptor.RegisterSpace = 0;
  mvp_cb_descriptor.ShaderRegister = 0;
  mvp_cb_descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;

  parameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  parameters[0].Descriptor = mvp_cb_descriptor;
  parameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

  D3D12_ROOT_DESCRIPTOR1 material_cb_descriptor = {};
  material_cb_descriptor.RegisterSpace = 0;
  material_cb_descriptor.ShaderRegister = 1;
  material_cb_descriptor.Flags = D3D12_ROOT_DESCRIPTOR_FLAG_NONE;

  parameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
  parameters[1].Descriptor = material_cb_descriptor;
  parameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

  switch (type) {
    case RR::kPipelineType_PBR: {
      parameters.resize(4);
      samplers.resize(1);

      D3D12_ROOT_CONSTANTS pipeline_constants = {};
      pipeline_constants.RegisterSpace = 0;
      pipeline_constants.ShaderRegister = 2;
      pipeline_constants.Num32BitValues = 4;

      parameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_32BIT_CONSTANTS;
      parameters[2].Constants = pipeline_constants;
      parameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

      D3D12_DESCRIPTOR_RANGE1 table_ranges[1] = {};
      table_ranges[0].RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_SRV;
      table_ranges[0].NumDescriptors = 5;
      table_ranges[0].BaseShaderRegister = 0;
      table_ranges[0].RegisterSpace = 0;
      table_ranges[0].OffsetInDescriptorsFromTableStart =
          D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

      D3D12_ROOT_DESCRIPTOR_TABLE1 descriptor_table = {};
      descriptor_table.NumDescriptorRanges = 1;
      descriptor_table.pDescriptorRanges = &table_ranges[0];

      parameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
      parameters[3].DescriptorTable = descriptor_table;
      parameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

      samplers[0].Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
      samplers[0].AddressU = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
      samplers[0].AddressV = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
      samplers[0].AddressW = D3D12_TEXTURE_ADDRESS_MODE_WRAP;
      samplers[0].MipLODBias = 0;
      samplers[0].MaxAnisotropy = 0;
      samplers[0].ComparisonFunc = D3D12_COMPARISON_FUNC_NEVER;
      samplers[0].BorderColor = D3D12_STATIC_BORDER_COLOR_TRANSPARENT_BLACK;
      samplers[0].MinLOD = 0.0f;
      samplers[0].MaxLOD = D3D12_FLOAT32_MAX;
      samplers[0].ShaderRegister = 0;
      samplers[0].RegisterSpace = 0;
      samplers[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;
      break;
    }
  }  

  D3D12_VERSIONED_ROOT_SIGNATURE_DESC root_signature_desc;
  root_signature_desc.Version = D3D_ROOT_SIGNATURE_VERSION_1_1;
  root_signature_desc.Desc_1_1.NumParameters = parameters.size();
  root_signature_desc.Desc_1_1.pParameters = parameters.data();
  root_signature_desc.Desc_1_1.NumStaticSamplers = samplers.size();
  root_signature_desc.Desc_1_1.pStaticSamplers = samplers.data();
  root_signature_desc.Desc_1_1.Flags =
      D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT |
      D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS |
      D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS |
      D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS;

  ID3DBlob* signature = nullptr;
  ID3DBlob* error = nullptr;

  result = D3D12SerializeVersionedRootSignature(&root_signature_desc,
                                                &signature, &error);
  if (FAILED(result)) {
    LOG_ERROR("RR::GFX", "Couldn't serialeze root signature");
    LOG_ERROR("RR::GFX", "Error: %s", error->GetBufferPointer());
    return 1;
  }

  result = device->CreateRootSignature(0, signature->GetBufferPointer(),
                                       signature->GetBufferSize(),
                                       IID_PPV_ARGS(&_root_signature));
  if (FAILED(result)) {
    LOG_ERROR("RR::GFX", "Couldn't create root signature");
    return 1;
  }

  ID3DBlob *vertex_shader = nullptr, *fragment_shader = nullptr;
  UINT compile_flags = 0;

#ifdef DEBUG
  compile_flags |= D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#else
  compile_flags |= D3DCOMPILE_OPTIMIZATION_LEVEL3;
#endif  // DEBUG

  switch (type) { 
    case RR::PipelineTypes::kPipelineType_PBR:
      result = D3DCompileFromFile(L"../../shaders/pbr.vert.hlsl", nullptr,
                                  nullptr, "main", "vs_5_1", compile_flags, 0,
                                  &vertex_shader, &error);
      break;
    case RR::PipelineTypes::kPipelineType_Phong:
      result = D3DCompileFromFile(L"../../shaders/phong.vert.hlsl", nullptr,
                                  nullptr, "main", "vs_5_1", compile_flags, 0,
                                  &vertex_shader, &error);
      break;
  }

  if (FAILED(result)) {
    LOG_ERROR("RR::GFX", "Couldn't compile vertex shader");
    LOG_ERROR("RR::GFX", "Compile error: /n%s", error->GetBufferPointer());
    return 1;
  }

  switch (type) {
    case RR::PipelineTypes::kPipelineType_PBR:
      result = D3DCompileFromFile(L"../../shaders/pbr.frag.hlsl", nullptr,
                                  nullptr, "main", "ps_5_1", compile_flags, 0,
                                  &fragment_shader, &error);
      break;
    case RR::PipelineTypes::kPipelineType_Phong:
      result = D3DCompileFromFile(L"../../shaders/phong.frag.hlsl", nullptr,
                                  nullptr, "main", "ps_5_1", compile_flags, 0,
                                  &fragment_shader, &error);
      break;
  }

  if (FAILED(result)) {
    LOG_ERROR("RR::GFX", "Couldn't compile fragment shader");
    LOG_ERROR("RR::GFX", "Compile error: /n%s", error->GetBufferPointer());
    return 1;
  }

  D3D12_SHADER_BYTECODE vertex_shader_bytecode = {};
  vertex_shader_bytecode.BytecodeLength = vertex_shader->GetBufferSize();
  vertex_shader_bytecode.pShaderBytecode = vertex_shader->GetBufferPointer();

  D3D12_SHADER_BYTECODE pixel_shader_bytecode = {};
  pixel_shader_bytecode.BytecodeLength = fragment_shader->GetBufferSize();
  pixel_shader_bytecode.pShaderBytecode = fragment_shader->GetBufferPointer();

  std::vector<D3D12_INPUT_ELEMENT_DESC> input_layout;

  switch (geometry_type) { 
    case RR::GeometryTypes::kGeometryType_Positions_Normals:
      input_layout = std::vector<D3D12_INPUT_ELEMENT_DESC>(2);

      input_layout[0] = {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
                         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};

      input_layout[1] = {"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
                         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
      break;
    case RR::GeometryTypes::kGeometryType_Positions_Normals_UV:
      input_layout = std::vector<D3D12_INPUT_ELEMENT_DESC>(3);

      input_layout[0] = {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0,
                         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};

      input_layout[1] = {"NORMAL", 0,  DXGI_FORMAT_R32G32B32_FLOAT, 0, 12,
                         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};

      input_layout[2] = {"UV", 0,  DXGI_FORMAT_R32G32_FLOAT, 0, 24,
                         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
      break;
    case RR::GeometryTypes::kGeometryType_Positions_Normals_Tangents_UV:
      input_layout = std::vector<D3D12_INPUT_ELEMENT_DESC>(4);

      input_layout[0] = {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, 
                         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};

      input_layout[1] = {"NORMAL", 0,  DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, 
                         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};

      input_layout[2] = {"TANGENT", 0,  DXGI_FORMAT_R32G32B32_FLOAT, 0, 24,
                         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};

      input_layout[3] = {"UV", 0,  DXGI_FORMAT_R32G32_FLOAT, 0, 36, 
                         D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0};
      break;
  }

  D3D12_INPUT_LAYOUT_DESC input_layout_desc = {};
  input_layout_desc.pInputElementDescs = &input_layout[0];
  input_layout_desc.NumElements = input_layout.size();

  D3D12_RASTERIZER_DESC rasterizer_desc = {};
  rasterizer_desc.FillMode = D3D12_FILL_MODE_SOLID;
  rasterizer_desc.CullMode = D3D12_CULL_MODE_BACK;
  rasterizer_desc.FrontCounterClockwise = FALSE;
  rasterizer_desc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
  rasterizer_desc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
  rasterizer_desc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
  rasterizer_desc.DepthClipEnable = TRUE;
  rasterizer_desc.MultisampleEnable = FALSE;
  rasterizer_desc.AntialiasedLineEnable = FALSE;
  rasterizer_desc.ForcedSampleCount = 0;
  rasterizer_desc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;

  D3D12_BLEND_DESC blend_desc = {};
  blend_desc.AlphaToCoverageEnable = FALSE;
  blend_desc.IndependentBlendEnable = FALSE;

  const D3D12_RENDER_TARGET_BLEND_DESC render_target_blend_desc = {
      FALSE,
      FALSE,
      D3D12_BLEND_ONE,
      D3D12_BLEND_ZERO,
      D3D12_BLEND_OP_ADD,
      D3D12_BLEND_ONE,
      D3D12_BLEND_ZERO,
      D3D12_BLEND_OP_ADD,
      D3D12_LOGIC_OP_NOOP,
      D3D12_COLOR_WRITE_ENABLE_ALL,
  };

  for (UINT i = 0; i < D3D12_SIMULTANEOUS_RENDER_TARGET_COUNT; ++i) {
    blend_desc.RenderTarget[i] = render_target_blend_desc;
  }
  D3D12_DEPTH_STENCILOP_DESC depth_stencil_op = {
      D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP,
      D3D12_COMPARISON_FUNC_ALWAYS};

  D3D12_GRAPHICS_PIPELINE_STATE_DESC pipeline_desc = {};
  pipeline_desc.InputLayout = input_layout_desc;
  pipeline_desc.pRootSignature = _root_signature;
  pipeline_desc.VS = vertex_shader_bytecode;
  pipeline_desc.PS = pixel_shader_bytecode;
  pipeline_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
  pipeline_desc.RasterizerState = rasterizer_desc;
  pipeline_desc.BlendState = blend_desc;
  pipeline_desc.NumRenderTargets = 1;
  pipeline_desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
  pipeline_desc.SampleDesc.Count = 1;
  pipeline_desc.DSVFormat = DXGI_FORMAT_D32_FLOAT;
  pipeline_desc.DepthStencilState.DepthEnable = TRUE;
  pipeline_desc.DepthStencilState.StencilEnable = FALSE;
  pipeline_desc.DepthStencilState.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
  pipeline_desc.DepthStencilState.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
  pipeline_desc.DepthStencilState.FrontFace = depth_stencil_op;
  pipeline_desc.DepthStencilState.BackFace = depth_stencil_op;
  pipeline_desc.SampleMask = UINT_MAX;

  result = device->CreateGraphicsPipelineState(&pipeline_desc, IID_PPV_ARGS(&_pipeline_state));
  if (FAILED(result)) {
    LOG_ERROR("RR::GFX", "Couldn't create pipeline type: %i", type);
    return 1;
  }

  _initialized = true;
  _type = type;
  _geometry_type = geometry_type;

  LOG_DEBUG("RR::GFX", "Pipeline type: %i, initialized", type);
}

void RR::GFX::Pipeline::Release() {
  if (!_initialized) {
    return;
  }  

  if (_pipeline_state != nullptr) {
    _pipeline_state->Release();
    _pipeline_state = nullptr;
  }

  if (_root_signature != nullptr) {
    _root_signature->Release();
    _root_signature = nullptr;
  }
}

ID3D12PipelineState* RR::GFX::Pipeline::PipelineState() { 
  return _pipeline_state; 
}

ID3D12RootSignature* RR::GFX::Pipeline::RootSignature() {
  return _root_signature;
}

uint32_t RR::GFX::Pipeline::GeometryType() { 
  return _geometry_type; 
}

uint32_t RR::GFX::Pipeline::Type() { return _type; }
