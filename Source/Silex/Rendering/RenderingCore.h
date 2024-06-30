
#pragma once
#include "Core/Core.h"


namespace Silex
{
    //================================================
    // 定数
    //================================================
    enum : uint32 { INVALID_RENDER_ID = UINT32_MAX };

    //================================================
    // ハンドル
    //================================================
    using QueueFamily = uint32;

    SL_HANDLE(Surface);
    SL_HANDLE(CommandQueue);
    SL_HANDLE(CommandPool);
    SL_HANDLE(CommandBuffer);
    SL_HANDLE(Fence);
    SL_HANDLE(Semaphore);
    SL_HANDLE(SwapChain);
    SL_HANDLE(RenderPass);
    SL_HANDLE(Buffer);
    SL_HANDLE(Pipeline);
    SL_HANDLE(Sampler);
    SL_HANDLE(DescriptorSet);
    SL_HANDLE(VertexFormat );

    // 命名重複のためHandleを追加 (OpenGL廃止後に修正)
    SL_HANDLE(FramebufferHandle);
    SL_HANDLE(TextureHandle);
    SL_HANDLE(ShaderHandle);

    //================================================
    // デバイス
    //================================================
    enum DeviceVendor
    {
        DEVICE_VENDOR_UNKNOWN   = 0x0000,
        DEVICE_VENDOR_AMD       = 0x1002,
        DEVICE_VENDOR_IMGTEC    = 0x1010,
        DEVICE_VENDOR_APPLE     = 0x106B,
        DEVICE_VENDOR_NVIDIA    = 0x10DE,
        DEVICE_VENDOR_ARM       = 0x13B5,
        DEVICE_VENDOR_MICROSOFT = 0x1414,
        DEVICE_VENDOR_QUALCOMM  = 0x5143,
        DEVICE_VENDOR_INTEL     = 0x8086,
    };

    enum DeviceType
    {
        DEVICE_TYPE_UNKNOW,
        DEVICE_TYPE_INTEGRATED_GPU,
        DEVICE_TYPE_DISCRETE_GPU,
        DEVICE_TYPE_VIRTUAL_GPU,
        DEVICE_TYPE_CPU,

        DEVICE_TYPE_MAX,
    };

    struct DeviceInfo
    {
        std::string  name   = "Unknown";
        DeviceVendor vendor = DEVICE_VENDOR_UNKNOWN;
        DeviceType   type   = DEVICE_TYPE_UNKNOW;
    };

    //================================================
    // フォーマット
    //================================================
    enum RenderingFormat
    {
        RENDERING_FORMAT_UNDEFINE,
        RENDERING_FORMAT_R4G4_UNORM_PACK8,
        RENDERING_FORMAT_R4G4B4A4_UNORM_PACK16,
        RENDERING_FORMAT_B4G4R4A4_UNORM_PACK16,
        RENDERING_FORMAT_R5G6B5_UNORM_PACK16,
        RENDERING_FORMAT_B5G6R5_UNORM_PACK16,
        RENDERING_FORMAT_R5G5B5A1_UNORM_PACK16,
        RENDERING_FORMAT_B5G5R5A1_UNORM_PACK16,
        RENDERING_FORMAT_A1R5G5B5_UNORM_PACK16,
        RENDERING_FORMAT_R8_UNORM,
        RENDERING_FORMAT_R8_SNORM,
        RENDERING_FORMAT_R8_USCALED,
        RENDERING_FORMAT_R8_SSCALED,
        RENDERING_FORMAT_R8_UINT,
        RENDERING_FORMAT_R8_SINT,
        RENDERING_FORMAT_R8_SRGB,
        RENDERING_FORMAT_R8G8_UNORM,
        RENDERING_FORMAT_R8G8_SNORM,
        RENDERING_FORMAT_R8G8_USCALED,
        RENDERING_FORMAT_R8G8_SSCALED,
        RENDERING_FORMAT_R8G8_UINT,
        RENDERING_FORMAT_R8G8_SINT,
        RENDERING_FORMAT_R8G8_SRGB,
        RENDERING_FORMAT_R8G8B8_UNORM,
        RENDERING_FORMAT_R8G8B8_SNORM,
        RENDERING_FORMAT_R8G8B8_USCALED,
        RENDERING_FORMAT_R8G8B8_SSCALED,
        RENDERING_FORMAT_R8G8B8_UINT,
        RENDERING_FORMAT_R8G8B8_SINT,
        RENDERING_FORMAT_R8G8B8_SRGB,
        RENDERING_FORMAT_B8G8R8_UNORM,
        RENDERING_FORMAT_B8G8R8_SNORM,
        RENDERING_FORMAT_B8G8R8_USCALED,
        RENDERING_FORMAT_B8G8R8_SSCALED,
        RENDERING_FORMAT_B8G8R8_UINT,
        RENDERING_FORMAT_B8G8R8_SINT,
        RENDERING_FORMAT_B8G8R8_SRGB,
        RENDERING_FORMAT_R8G8B8A8_UNORM,
        RENDERING_FORMAT_R8G8B8A8_SNORM,
        RENDERING_FORMAT_R8G8B8A8_USCALED,
        RENDERING_FORMAT_R8G8B8A8_SSCALED,
        RENDERING_FORMAT_R8G8B8A8_UINT,
        RENDERING_FORMAT_R8G8B8A8_SINT,
        RENDERING_FORMAT_R8G8B8A8_SRGB,
        RENDERING_FORMAT_B8G8R8A8_UNORM,
        RENDERING_FORMAT_B8G8R8A8_SNORM,
        RENDERING_FORMAT_B8G8R8A8_USCALED,
        RENDERING_FORMAT_B8G8R8A8_SSCALED,
        RENDERING_FORMAT_B8G8R8A8_UINT,
        RENDERING_FORMAT_B8G8R8A8_SINT,
        RENDERING_FORMAT_B8G8R8A8_SRGB,
        RENDERING_FORMAT_A8B8G8R8_UNORM_PACK32,
        RENDERING_FORMAT_A8B8G8R8_SNORM_PACK32,
        RENDERING_FORMAT_A8B8G8R8_USCALED_PACK32,
        RENDERING_FORMAT_A8B8G8R8_SSCALED_PACK32,
        RENDERING_FORMAT_A8B8G8R8_UINT_PACK32,
        RENDERING_FORMAT_A8B8G8R8_SINT_PACK32,
        RENDERING_FORMAT_A8B8G8R8_SRGB_PACK32,
        RENDERING_FORMAT_A2R10G10B10_UNORM_PACK32,
        RENDERING_FORMAT_A2R10G10B10_SNORM_PACK32,
        RENDERING_FORMAT_A2R10G10B10_USCALED_PACK32,
        RENDERING_FORMAT_A2R10G10B10_SSCALED_PACK32,
        RENDERING_FORMAT_A2R10G10B10_UINT_PACK32,
        RENDERING_FORMAT_A2R10G10B10_SINT_PACK32,
        RENDERING_FORMAT_A2B10G10R10_UNORM_PACK32,
        RENDERING_FORMAT_A2B10G10R10_SNORM_PACK32,
        RENDERING_FORMAT_A2B10G10R10_USCALED_PACK32,
        RENDERING_FORMAT_A2B10G10R10_SSCALED_PACK32,
        RENDERING_FORMAT_A2B10G10R10_UINT_PACK32,
        RENDERING_FORMAT_A2B10G10R10_SINT_PACK32,
        RENDERING_FORMAT_R16_UNORM,
        RENDERING_FORMAT_R16_SNORM,
        RENDERING_FORMAT_R16_USCALED,
        RENDERING_FORMAT_R16_SSCALED,
        RENDERING_FORMAT_R16_UINT,
        RENDERING_FORMAT_R16_SINT,
        RENDERING_FORMAT_R16_SFLOAT,
        RENDERING_FORMAT_R16G16_UNORM,
        RENDERING_FORMAT_R16G16_SNORM,
        RENDERING_FORMAT_R16G16_USCALED,
        RENDERING_FORMAT_R16G16_SSCALED,
        RENDERING_FORMAT_R16G16_UINT,
        RENDERING_FORMAT_R16G16_SINT,
        RENDERING_FORMAT_R16G16_SFLOAT,
        RENDERING_FORMAT_R16G16B16_UNORM,
        RENDERING_FORMAT_R16G16B16_SNORM,
        RENDERING_FORMAT_R16G16B16_USCALED,
        RENDERING_FORMAT_R16G16B16_SSCALED,
        RENDERING_FORMAT_R16G16B16_UINT,
        RENDERING_FORMAT_R16G16B16_SINT,
        RENDERING_FORMAT_R16G16B16_SFLOAT,
        RENDERING_FORMAT_R16G16B16A16_UNORM,
        RENDERING_FORMAT_R16G16B16A16_SNORM,
        RENDERING_FORMAT_R16G16B16A16_USCALED,
        RENDERING_FORMAT_R16G16B16A16_SSCALED,
        RENDERING_FORMAT_R16G16B16A16_UINT,
        RENDERING_FORMAT_R16G16B16A16_SINT,
        RENDERING_FORMAT_R16G16B16A16_SFLOAT,
        RENDERING_FORMAT_R32_UINT,
        RENDERING_FORMAT_R32_SINT,
        RENDERING_FORMAT_R32_SFLOAT,
        RENDERING_FORMAT_R32G32_UINT,
        RENDERING_FORMAT_R32G32_SINT,
        RENDERING_FORMAT_R32G32_SFLOAT,
        RENDERING_FORMAT_R32G32B32_UINT,
        RENDERING_FORMAT_R32G32B32_SINT,
        RENDERING_FORMAT_R32G32B32_SFLOAT,
        RENDERING_FORMAT_R32G32B32A32_UINT,
        RENDERING_FORMAT_R32G32B32A32_SINT,
        RENDERING_FORMAT_R32G32B32A32_SFLOAT,
        RENDERING_FORMAT_R64_UINT,
        RENDERING_FORMAT_R64_SINT,
        RENDERING_FORMAT_R64_SFLOAT,
        RENDERING_FORMAT_R64G64_UINT,
        RENDERING_FORMAT_R64G64_SINT,
        RENDERING_FORMAT_R64G64_SFLOAT,
        RENDERING_FORMAT_R64G64B64_UINT,
        RENDERING_FORMAT_R64G64B64_SINT,
        RENDERING_FORMAT_R64G64B64_SFLOAT,
        RENDERING_FORMAT_R64G64B64A64_UINT,
        RENDERING_FORMAT_R64G64B64A64_SINT,
        RENDERING_FORMAT_R64G64B64A64_SFLOAT,
        RENDERING_FORMAT_B10G11R11_UFLOAT_PACK32,
        RENDERING_FORMAT_E5B9G9R9_UFLOAT_PACK32,
        RENDERING_FORMAT_D16_UNORM,
        RENDERING_FORMAT_X8_D24_UNORM_PACK32,
        RENDERING_FORMAT_D32_SFLOAT,
        RENDERING_FORMAT_S8_UINT,
        RENDERING_FORMAT_D16_UNORM_S8_UINT,
        RENDERING_FORMAT_D24_UNORM_S8_UINT,
        RENDERING_FORMAT_D32_SFLOAT_S8_UINT,
        RENDERING_FORMAT_BC1_RGB_UNORM_BLOCK,
        RENDERING_FORMAT_BC1_RGB_SRGB_BLOCK,
        RENDERING_FORMAT_BC1_RGBA_UNORM_BLOCK,
        RENDERING_FORMAT_BC1_RGBA_SRGB_BLOCK,
        RENDERING_FORMAT_BC2_UNORM_BLOCK,
        RENDERING_FORMAT_BC2_SRGB_BLOCK,
        RENDERING_FORMAT_BC3_UNORM_BLOCK,
        RENDERING_FORMAT_BC3_SRGB_BLOCK,
        RENDERING_FORMAT_BC4_UNORM_BLOCK,
        RENDERING_FORMAT_BC4_SNORM_BLOCK,
        RENDERING_FORMAT_BC5_UNORM_BLOCK,
        RENDERING_FORMAT_BC5_SNORM_BLOCK,
        RENDERING_FORMAT_BC6H_UFLOAT_BLOCK,
        RENDERING_FORMAT_BC6H_SFLOAT_BLOCK,
        RENDERING_FORMAT_BC7_UNORM_BLOCK,
        RENDERING_FORMAT_BC7_SRGB_BLOCK,
        RENDERING_FORMAT_ETC2_R8G8B8_UNORM_BLOCK,
        RENDERING_FORMAT_ETC2_R8G8B8_SRGB_BLOCK,
        RENDERING_FORMAT_ETC2_R8G8B8A1_UNORM_BLOCK,
        RENDERING_FORMAT_ETC2_R8G8B8A1_SRGB_BLOCK,
        RENDERING_FORMAT_ETC2_R8G8B8A8_UNORM_BLOCK,
        RENDERING_FORMAT_ETC2_R8G8B8A8_SRGB_BLOCK,
        RENDERING_FORMAT_EAC_R11_UNORM_BLOCK,
        RENDERING_FORMAT_EAC_R11_SNORM_BLOCK,
        RENDERING_FORMAT_EAC_R11G11_UNORM_BLOCK,
        RENDERING_FORMAT_EAC_R11G11_SNORM_BLOCK,
        RENDERING_FORMAT_ASTC_4x4_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_4x4_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_5x4_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_5x4_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_5x5_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_5x5_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_6x5_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_6x5_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_6x6_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_6x6_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_8x5_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_8x5_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_8x6_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_8x6_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_8x8_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_8x8_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_10x5_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_10x5_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_10x6_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_10x6_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_10x8_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_10x8_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_10x10_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_10x10_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_12x10_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_12x10_SRGB_BLOCK,
        RENDERING_FORMAT_ASTC_12x12_UNORM_BLOCK,
        RENDERING_FORMAT_ASTC_12x12_SRGB_BLOCK,
        RENDERING_FORMAT_G8B8G8R8_422_UNORM,
        RENDERING_FORMAT_B8G8R8G8_422_UNORM,
        RENDERING_FORMAT_G8_B8_R8_3PLANE_420_UNORM,
        RENDERING_FORMAT_G8_B8R8_2PLANE_420_UNORM,
        RENDERING_FORMAT_G8_B8_R8_3PLANE_422_UNORM,
        RENDERING_FORMAT_G8_B8R8_2PLANE_422_UNORM,
        RENDERING_FORMAT_G8_B8_R8_3PLANE_444_UNORM,
        RENDERING_FORMAT_R10X6_UNORM_PACK16,
        RENDERING_FORMAT_R10X6G10X6_UNORM_2PACK16,
        RENDERING_FORMAT_R10X6G10X6B10X6A10X6_UNORM_4PACK16,
        RENDERING_FORMAT_G10X6B10X6G10X6R10X6_422_UNORM_4PACK16,
        RENDERING_FORMAT_B10X6G10X6R10X6G10X6_422_UNORM_4PACK16,
        RENDERING_FORMAT_G10X6_B10X6_R10X6_3PLANE_420_UNORM_3PACK16,
        RENDERING_FORMAT_G10X6_B10X6R10X6_2PLANE_420_UNORM_3PACK16,
        RENDERING_FORMAT_G10X6_B10X6_R10X6_3PLANE_422_UNORM_3PACK16,
        RENDERING_FORMAT_G10X6_B10X6R10X6_2PLANE_422_UNORM_3PACK16,
        RENDERING_FORMAT_G10X6_B10X6_R10X6_3PLANE_444_UNORM_3PACK16,
        RENDERING_FORMAT_R12X4_UNORM_PACK16,
        RENDERING_FORMAT_R12X4G12X4_UNORM_2PACK16,
        RENDERING_FORMAT_R12X4G12X4B12X4A12X4_UNORM_4PACK16,
        RENDERING_FORMAT_G12X4B12X4G12X4R12X4_422_UNORM_4PACK16,
        RENDERING_FORMAT_B12X4G12X4R12X4G12X4_422_UNORM_4PACK16,
        RENDERING_FORMAT_G12X4_B12X4_R12X4_3PLANE_420_UNORM_3PACK16,
        RENDERING_FORMAT_G12X4_B12X4R12X4_2PLANE_420_UNORM_3PACK16,
        RENDERING_FORMAT_G12X4_B12X4_R12X4_3PLANE_422_UNORM_3PACK16,
        RENDERING_FORMAT_G12X4_B12X4R12X4_2PLANE_422_UNORM_3PACK16,
        RENDERING_FORMAT_G12X4_B12X4_R12X4_3PLANE_444_UNORM_3PACK16,
        RENDERING_FORMAT_G16B16G16R16_422_UNORM,
        RENDERING_FORMAT_B16G16R16G16_422_UNORM,
        RENDERING_FORMAT_G16_B16_R16_3PLANE_420_UNORM,
        RENDERING_FORMAT_G16_B16R16_2PLANE_420_UNORM,
        RENDERING_FORMAT_G16_B16_R16_3PLANE_422_UNORM,
        RENDERING_FORMAT_G16_B16R16_2PLANE_422_UNORM,
        RENDERING_FORMAT_G16_B16_R16_3PLANE_444_UNORM,

        RENDERING_FORMAT_MAX,
    };

    //================================================
    // 比較関数
    //================================================
    enum CompareOperator
    {
        COMPARE_OP_NEVER,
        COMPARE_OP_LESS,
        COMPARE_OP_EQUAL,
        COMPARE_OP_LESS_OR_EQUAL,
        COMPARE_OP_GREATER,
        COMPARE_OP_NOT_EQUAL,
        COMPARE_OP_GREATER_OR_EQUAL,
        COMPARE_OP_ALWAYS,

        COMPARE_OP_MAX
    };

    //================================================
    // メモリ
    //================================================
    enum MemoryAllocationType
    {
        MEMORY_ALLOCATION_TYPE_CPU,
        MEMORY_ALLOCATION_TYPE_GPU,

        MEMORY_ALLOCATION_TYPE_MAX,
    };

    //================================================
    // キューファミリ
    //================================================
    enum QueueFamilyBits
    {
        QUEUE_FAMILY_GRAPHICS_BIT = SL_BIT(0),
        QUEUE_FAMILY_COMPUTE_BIT  = SL_BIT(1),
        QUEUE_FAMILY_TRANSFER_BIT = SL_BIT(2),
    };

    //================================================
    // コマンドバッファ
    //================================================
    enum CommandBufferType
    {
        COMMAND_BUFFER_TYPE_PRIMARY,
        COMMAND_BUFFER_TYPE_SECONDARY,

        COMMAND_BUFFER_TYPE_MAX,
    };

    //=================================================
    // バッファ
    //=================================================
    enum BufferUsageBits
    {
        BUFFER_USAGE_TRANSFER_SRC_BIT         = SL_BIT(0),
        BUFFER_USAGE_TRANSFER_DST_BIT         = SL_BIT(1),
        BUFFER_USAGE_UNIFORM_TEXEL_BUFFER_BIT = SL_BIT(2),
        BUFFER_USAGE_STORAGE_TEXEL_BUFFER_BIT = SL_BIT(3),
        BUFFER_USAGE_UNIFORM_BIT              = SL_BIT(4),
        BUFFER_USAGE_STORAGE_BIT              = SL_BIT(5),
        BUFFER_USAGE_INDEX_BIT                = SL_BIT(6),
        BUFFER_USAGE_VERTEX_BIT               = SL_BIT(7),
        BUFFER_USAGE_INDIRECT_BIT             = SL_BIT(8),
    };

    //=================================================
    // テクスチャ
    //=================================================
    enum TextureType
    {
        TEXTURE_TYPE_1D,
        TEXTURE_TYPE_2D,
        TEXTURE_TYPE_3D,
        TEXTURE_TYPE_CUBE,
        TEXTURE_TYPE_1D_ARRAY,
        TEXTURE_TYPE_2D_ARRAY,
        TEXTURE_TYPE_CUBE_ARRAY,

        TEXTURE_TYPE_MAX,
    };

    enum TextureSamples
    {
        TEXTURE_SAMPLES_1,
        TEXTURE_SAMPLES_2,
        TEXTURE_SAMPLES_4,
        TEXTURE_SAMPLES_8,
        TEXTURE_SAMPLES_16,
        TEXTURE_SAMPLES_32,
        TEXTURE_SAMPLES_64,

        TEXTURE_SAMPLES_MAX,
    };

    enum TextureLayout
    {
        TEXTURE_LAYOUT_UNDEFINED,
        TEXTURE_LAYOUT_GENERAL,
        TEXTURE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL,
        TEXTURE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL,
        TEXTURE_LAYOUT_DEPTH_STENCIL_READ_ONLY_OPTIMAL,
        TEXTURE_LAYOUT_SHADER_READ_ONLY_OPTIMAL,
        TEXTURE_LAYOUT_TRANSFER_SRC_OPTIMAL,
        TEXTURE_LAYOUT_TRANSFER_DST_OPTIMAL,
        TEXTURE_LAYOUT_PREINITIALIZED,

        TEXTURE_LAYOUT_DEPTH_READ_ONLY_STENCIL_ATTACHMENT_OPTIMAL = 1000117000,
        TEXTURE_LAYOUT_DEPTH_ATTACHMENT_STENCIL_READ_ONLY_OPTIMAL = 1000117001,
        TEXTURE_LAYOUT_DEPTH_ATTACHMENT_OPTIMAL                   = 1000241000,
        TEXTURE_LAYOUT_DEPTH_READ_ONLY_OPTIMAL                    = 1000241001,
        TEXTURE_LAYOUT_STENCIL_ATTACHMENT_OPTIMAL                 = 1000241002,
        TEXTURE_LAYOUT_STENCIL_READ_ONLY_OPTIMAL                  = 1000241003,
        TEXTURE_LAYOUT_READ_ONLY_OPTIMAL                          = 1000314000,
        TEXTURE_LAYOUT_ATTACHMENT_OPTIMAL                         = 1000314001,
        TEXTURE_LAYOUT_PRESENT_SRC                                = 1000001002,
    };

    enum TextureAspectBits
    {
        TEXTURE_ASPECT_COLOR,
        TEXTURE_ASPECT_DEPTH,
        TEXTURE_ASPECT_STENCIL,
        TEXTURE_ASPECT_MAX,

        TEXTURE_ASPECT_COLOR_BIT   = SL_BIT(TEXTURE_ASPECT_COLOR),
        TEXTURE_ASPECT_DEPTH_BIT   = SL_BIT(TEXTURE_ASPECT_DEPTH),
        TEXTURE_ASPECT_STENCIL_BIT = SL_BIT(TEXTURE_ASPECT_STENCIL),
    };

    enum TextureUsageBits
    {
        TEXTURE_USAGE_COPY_SRC_BIT                 = SL_BIT(0),
        TEXTURE_USAGE_COPY_DST_BIT                 = SL_BIT(1),
        TEXTURE_USAGE_SAMPLING_BIT                 = SL_BIT(2),
        TEXTURE_USAGE_STORAGE_BIT                  = SL_BIT(3),
        TEXTURE_USAGE_COLOR_ATTACHMENT_BIT         = SL_BIT(4),
        TEXTURE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT = SL_BIT(5),
        TEXTURE_USAGE_TRANSIENT_ATTACHMENT_BIT     = SL_BIT(6),
        TEXTURE_USAGE_INPUT_ATTACHMENT_BIT         = SL_BIT(7),
        TEXTURE_USAGE_CPU_READ_BIT                 = SL_BIT(8),
    };

    struct TextureFormat
    {
        RenderingFormat format    = RENDERING_FORMAT_UNDEFINE;
        uint32          width     = 0;
        uint32          height    = 0;
        uint32          depth     = 1;
        uint32          array     = 1;
        uint32          mipmap    = 1;
        TextureType     type      = TEXTURE_TYPE_2D;
        TextureSamples  samples   = TEXTURE_SAMPLES_1;
        uint32          usageBits = 0; 
    };

    struct TextureSubresource
    {
        TextureAspectBits aspect = TEXTURE_ASPECT_COLOR_BIT;
        uint32            layer  = 0;
        uint32            mipmap = 0;
    };

    struct TextureSubresourceLayers
    {
        TextureAspectBits aspect;
        uint32            mipmap     = 0;
        uint32            baseLayer  = 0;
        uint32            layerCount = 0;
    };

    struct TextureSubresourceRange
    {
        TextureAspectBits aspect;
        uint32            baseMipmap  = 0;
        uint32            mipmapCount = 0;
        uint32            baseLayer   = 0;
        uint32            layerCount  = 0;
    };

    //================================================
    // サンプラー
    //================================================
    enum SamplerFilter
    {
        SAMPLER_FILTER_NEAREST,
        SAMPLER_FILTER_LINEAR,

        SAMPLER_FILTER_MAX,
    };

    enum SamplerRepeatMode
    {
        SAMPLER_REPEAT_MODE_REPEAT,
        SAMPLER_REPEAT_MODE_MIRRORED_REPEAT,
        SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE,
        SAMPLER_REPEAT_MODE_CLAMP_TO_BORDER,
        SAMPLER_REPEAT_MODE_MIRROR_CLAMP_TO_EDGE,

        SAMPLER_REPEAT_MODE_MAX
    };

    enum SamplerBorderColor
    {
        SAMPLER_BORDER_COLOR_FLOAT_TRANSPARENT_BLACK,
        SAMPLER_BORDER_COLOR_INT_TRANSPARENT_BLACK,
        SAMPLER_BORDER_COLOR_FLOAT_OPAQUE_BLACK,
        SAMPLER_BORDER_COLOR_INT_OPAQUE_BLACK,
        SAMPLER_BORDER_COLOR_FLOAT_OPAQUE_WHITE,
        SAMPLER_BORDER_COLOR_INT_OPAQUE_WHITE,

        SAMPLER_BORDER_COLOR_MAX
    };

    struct SamplerState
    {
        SamplerFilter      magFilter     = SAMPLER_FILTER_LINEAR;
        SamplerFilter      minFilter     = SAMPLER_FILTER_LINEAR;
        SamplerFilter      mipFilter     = SAMPLER_FILTER_LINEAR;
        SamplerRepeatMode  repeatU       = SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
        SamplerRepeatMode  repeatV       = SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
        SamplerRepeatMode  repeatW       = SAMPLER_REPEAT_MODE_CLAMP_TO_EDGE;
        float              lodBias       = 0.0f;
        bool               useAnisotropy = false;
        float              anisotropyMax = 1.0f;
        bool               enableCompare = false;
        CompareOperator    compareOp     = COMPARE_OP_ALWAYS;
        float              minLod        = 0.0f;
        float              maxLod        = 1e20;
        SamplerBorderColor borderColor   = SAMPLER_BORDER_COLOR_FLOAT_OPAQUE_BLACK;
        bool               unnormalized  = false;
    };

    //================================================
    // バリア
    //================================================
    enum PipelineStageBits
    {
        PIPELINE_STAGE_TOP_OF_PIPE_BIT                    = SL_BIT(0),
        PIPELINE_STAGE_DRAW_INDIRECT_BIT                  = SL_BIT(1),
        PIPELINE_STAGE_VERTEX_INPUT_BIT                   = SL_BIT(2),
        PIPELINE_STAGE_VERTEX_SHADER_BIT                  = SL_BIT(3),
        PIPELINE_STAGE_TESSELLATION_CONTROL_SHADER_BIT    = SL_BIT(4),
        PIPELINE_STAGE_TESSELLATION_EVALUATION_SHADER_BIT = SL_BIT(5),
        PIPELINE_STAGE_GEOMETRY_SHADER_BIT                = SL_BIT(6),
        PIPELINE_STAGE_FRAGMENT_SHADER_BIT                = SL_BIT(7),
        PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT           = SL_BIT(8),
        PIPELINE_STAGE_LATE_FRAGMENT_TESTS_BIT            = SL_BIT(9),
        PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT        = SL_BIT(10),
        PIPELINE_STAGE_COMPUTE_SHADER_BIT                 = SL_BIT(11),
        PIPELINE_STAGE_TRANSFER_BIT                       = SL_BIT(12),
        PIPELINE_STAGE_BOTTOM_OF_PIPE_BIT                 = SL_BIT(13),
        PIPELINE_STAGE_HOST_BIT                           = SL_BIT(14),
        PIPELINE_STAGE_ALL_GRAPHICS_BIT                   = SL_BIT(15),
        PIPELINE_STAGE_ALL_COMMANDS_BIT                   = SL_BIT(16),
    };

    enum BarrierAccessBits
    {
        BARRIER_ACCESS_INDIRECT_COMMAND_READ_BIT          = SL_BIT(0),
        BARRIER_ACCESS_INDEX_READ_BIT                     = SL_BIT(1),
        BARRIER_ACCESS_VERTEX_ATTRIBUTE_READ_BIT          = SL_BIT(2),
        BARRIER_ACCESS_UNIFORM_READ_BIT                   = SL_BIT(3),
        BARRIER_ACCESS_INPUT_ATTACHMENT_READ_BIT          = SL_BIT(4),
        BARRIER_ACCESS_SHADER_READ_BIT                    = SL_BIT(5),
        BARRIER_ACCESS_SHADER_WRITE_BIT                   = SL_BIT(6),
        BARRIER_ACCESS_COLOR_ATTACHMENT_READ_BIT          = SL_BIT(7),
        BARRIER_ACCESS_COLOR_ATTACHMENT_WRITE_BIT         = SL_BIT(8),
        BARRIER_ACCESS_DEPTH_STENCIL_ATTACHMENT_READ_BIT  = SL_BIT(9),
        BARRIER_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT = SL_BIT(10),
        BARRIER_ACCESS_TRANSFER_READ_BIT                  = SL_BIT(11),
        BARRIER_ACCESS_TRANSFER_WRITE_BIT                 = SL_BIT(12),
        BARRIER_ACCESS_HOST_READ_BIT                      = SL_BIT(13),
        BARRIER_ACCESS_HOST_WRITE_BIT                     = SL_BIT(14),
        BARRIER_ACCESS_MEMORY_READ_BIT                    = SL_BIT(15),
        BARRIER_ACCESS_MEMORY_WRITE_BIT                   = SL_BIT(16),
    };

    struct MemoryBarrier
    {
        BarrierAccessBits srcAccess;
        BarrierAccessBits dstAccess;
    };

    struct BufferBarrier
    {
        Buffer*           buffer;
        BarrierAccessBits srcAccess;
        BarrierAccessBits dstAccess;
        uint64            offset;
        uint64            size;
    };

    struct TextureBarrier
    {
        TextureHandle*          texture;
        BarrierAccessBits       srcAccess;
        BarrierAccessBits       dstAccess;
        TextureLayout           oldLayout = TEXTURE_LAYOUT_UNDEFINED;
        TextureLayout           newLayout = TEXTURE_LAYOUT_UNDEFINED;
        TextureSubresourceRange subresources;
    };

    //================================================
    // レンダーパス
    //================================================
    enum AttachmentLoadOp
    {
        ATTACHMENT_LOAD_OP_LOAD,
        ATTACHMENT_LOAD_OP_CLEAR,
        ATTACHMENT_LOAD_OP_DONT_CARE,

        ATTACHMENT_LOAD_OP_MAX,
    };

    enum AttachmentStoreOp
    {
        ATTACHMENT_STORE_OP_STORE,
        ATTACHMENT_STORE_OP_DONT_CARE,

        ATTACHMENT_STORE_OP_MAX,
    };

    struct Attachment
    {
        RenderingFormat   format         = RENDERING_FORMAT_MAX;
        TextureSamples    samples        = TEXTURE_SAMPLES_MAX;
        AttachmentLoadOp  loadOp         = ATTACHMENT_LOAD_OP_DONT_CARE;
        AttachmentStoreOp storeOp        = ATTACHMENT_STORE_OP_DONT_CARE;
        AttachmentLoadOp  stencilLoadOp  = ATTACHMENT_LOAD_OP_DONT_CARE;
        AttachmentStoreOp stencilStoreOp = ATTACHMENT_STORE_OP_DONT_CARE;
        TextureLayout     initialLayout  = TEXTURE_LAYOUT_UNDEFINED;
        TextureLayout     finalLayout    = TEXTURE_LAYOUT_UNDEFINED;
    };

    struct AttachmentReference
    {
        uint32            attachment = INVALID_RENDER_ID;
        TextureLayout     layout     = TEXTURE_LAYOUT_UNDEFINED;
        TextureAspectBits aspect     = TEXTURE_ASPECT_COLOR_BIT;
    };

    struct Subpass
    {
        std::vector<AttachmentReference> inputReferences;
        std::vector<AttachmentReference> colorReferences;
        AttachmentReference              depthstencilReference;
        AttachmentReference              resolveReferences;
        std::vector<uint32>              preserveAttachments;
    };

    struct SubpassDependency
    {
        uint32            srcSubpass = INVALID_RENDER_ID;
        uint32            dstSubpass = INVALID_RENDER_ID;
        PipelineStageBits srcStages;
        PipelineStageBits dstStages;
        BarrierAccessBits srcAccess;
        BarrierAccessBits dstAccess;
    };

    //================================================
    // 頂点レイアウト
    //================================================
    enum IndexBufferFormat
    {
        INDEX_BUFFER_FORMAT_UINT16,
        INDEX_BUFFER_FORMAT_UINT32,
    };

    enum VertexFrequency
    {
        VERTEX_FREQUENCY_VERTEX,
        VERTEX_FREQUENCY_INSTANCE,
    };

    struct VertexAttribute
    {
        uint32          location  = 0;
        uint32          offset    = 0;
        RenderingFormat format    = RENDERING_FORMAT_MAX;
        uint32_t        stride    = 0;
        VertexFrequency frequency = VERTEX_FREQUENCY_VERTEX;
    };

    //================================================
    // デスクリプタ
    //================================================
    enum DescriptorType
    {
        DESCRIPTOR_TYPE_SAMPLER,                // VK_DESCRIPTOR_TYPE_SAMPLER
        DESCRIPTOR_TYPE_IMAGE_SAMPLER,          // VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER
        DESCRIPTOR_TYPE_IMAGE,                  // VK_DESCRIPTOR_TYPE_SAMPLED_IMAGE
        DESCRIPTOR_TYPE_STORAGE_IMAGE,          // VK_DESCRIPTOR_TYPE_STORAGE_IMAGE
        DESCRIPTOR_TYPE_UNIFORM_TEXTURE_BUFFER, // VK_DESCRIPTOR_TYPE_UNIFORM_TEXEL_BUFFER
        DESCRIPTOR_TYPE_STORAGE_TEXTURE_BUFFER, // VK_DESCRIPTOR_TYPE_STORAGE_TEXEL_BUFFER
        DESCRIPTOR_TYPE_UNIFORM_BUFFER,         // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER
        DESCRIPTOR_TYPE_STORAGE_BUFFER,         // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER
        DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC, // VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER_DYNAMIC
        DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC, // VK_DESCRIPTOR_TYPE_STORAGE_BUFFER_DYNAMIC
        DESCRIPTOR_TYPE_INPUT_ATTACHMENT,       // VK_DESCRIPTOR_TYPE_INPUT_ATTACHMENT

        DESCRIPTOR_TYPE_MAX
    };

    struct DescriptorInfo
    {
        DescriptorType       type    = DESCRIPTOR_TYPE_MAX;
        uint32               binding = INVALID_RENDER_ID;
        std::vector<Handle*> handles = {};

        Handle* image   = nullptr;
        Handle* sampler = nullptr;
        Handle* buffer  = nullptr;
    };

    //================================================
    // シェーダー
    //================================================
    enum ShaderStage
    {
        SHADER_STAGE_VERTEX,
        SHADER_STAGE_TESSELATION_CONTROL,
        SHADER_STAGE_TESSELATION_EVALUATION,
        SHADER_STAGE_GEOMETRY,
        SHADER_STAGE_FRAGMENT,
        SHADER_STAGE_COMPUTE,
        SHADER_STAGE_ALL,

        SHADER_STAGE_MAX,

        SHADER_STAGE_VERTEX_BIT                 = (1 << SHADER_STAGE_VERTEX),
        SHADER_STAGE_TESSELATION_CONTROL_BIT    = (1 << SHADER_STAGE_TESSELATION_CONTROL),
        SHADER_STAGE_TESSELATION_EVALUATION_BIT = (1 << SHADER_STAGE_TESSELATION_EVALUATION),
        SHADER_STAGE_GEOMETRY_BIT               = (1 << SHADER_STAGE_GEOMETRY),
        SHADER_STAGE_FRAGMENT_BIT               = (1 << SHADER_STAGE_FRAGMENT),
        SHADER_STAGE_COMPUTE_BIT                = (1 << SHADER_STAGE_COMPUTE),
    };

    //================================================
    // パイプライン
    //================================================
    enum PrimitiveTopology
    {
        PRIMITIVE_TOPOLOGY_POINTS,
        PRIMITIVE_TOPOLOGY_LINES,
        PRIMITIVE_TOPOLOGY_LINES_WITH_ADJACENCY,
        PRIMITIVE_TOPOLOGY_LINESTRIPS,
        PRIMITIVE_TOPOLOGY_LINESTRIPS_WITH_ADJACENCY,
        PRIMITIVE_TOPOLOGY_TRIANGLES,
        PRIMITIVE_TOPOLOGY_TRIANGLES_WITH_ADJACENCY,
        PRIMITIVE_TOPOLOGY_TRIANGLE_STRIPS,
        PRIMITIVE_TOPOLOGY_TRIANGLE_STRIPS_WITH_AJACENCY,
        PRIMITIVE_TOPOLOGY_TRIANGLE_STRIPS_WITH_RESTART_INDEX,
        PRIMITIVE_TOPOLOGY_TESSELATION_PATCH,

        RENDER_PRIMITIVE_MAX
    };

    enum PolygonCullMode
    {
        POLYGON_CULL_DISABLED,
        POLYGON_CULL_FRONT,
        POLYGON_CULL_BACK,

        POLYGON_CULL_MAX
    };

    enum PolygonFrontFace
    {
        POLYGON_FRONT_FACE_CLOCKWISE,
        POLYGON_FRONT_FACE_COUNTER_CLOCKWISE,

        POLYGON_FRONT_FACE_MAX,
    };

    enum StencilOperation
    {
        STENCIL_OP_KEEP,
        STENCIL_OP_ZERO,
        STENCIL_OP_REPLACE,
        STENCIL_OP_INCREMENT_AND_CLAMP,
        STENCIL_OP_DECREMENT_AND_CLAMP,
        STENCIL_OP_INVERT,
        STENCIL_OP_INCREMENT_AND_WRAP,
        STENCIL_OP_DECREMENT_AND_WRAP,

        STENCIL_OP_MAX
    };

    enum LogicOperation
    {
        LOGIC_OP_CLEAR,
        LOGIC_OP_AND,
        LOGIC_OP_AND_REVERSE,
        LOGIC_OP_COPY,
        LOGIC_OP_AND_INVERTED,
        LOGIC_OP_NO_OP,
        LOGIC_OP_XOR,
        LOGIC_OP_OR,
        LOGIC_OP_NOR,
        LOGIC_OP_EQUIVALENT,
        LOGIC_OP_INVERT,
        LOGIC_OP_OR_REVERSE,
        LOGIC_OP_COPY_INVERTED,
        LOGIC_OP_OR_INVERTED,
        LOGIC_OP_NAND,
        LOGIC_OP_SET,

        LOGIC_OP_MAX
    };

    enum BlendFactor
    {
        BLEND_FACTOR_ZERO,
        BLEND_FACTOR_ONE,
        BLEND_FACTOR_SRC_COLOR,
        BLEND_FACTOR_ONE_MINUS_SRC_COLOR,
        BLEND_FACTOR_DST_COLOR,
        BLEND_FACTOR_ONE_MINUS_DST_COLOR,
        BLEND_FACTOR_SRC_ALPHA,
        BLEND_FACTOR_ONE_MINUS_SRC_ALPHA,
        BLEND_FACTOR_DST_ALPHA,
        BLEND_FACTOR_ONE_MINUS_DST_ALPHA,
        BLEND_FACTOR_CONSTANT_COLOR,
        BLEND_FACTOR_ONE_MINUS_CONSTANT_COLOR,
        BLEND_FACTOR_CONSTANT_ALPHA,
        BLEND_FACTOR_ONE_MINUS_CONSTANT_ALPHA,
        BLEND_FACTOR_SRC_ALPHA_SATURATE,
        BLEND_FACTOR_SRC1_COLOR,
        BLEND_FACTOR_ONE_MINUS_SRC1_COLOR,
        BLEND_FACTOR_SRC1_ALPHA,
        BLEND_FACTOR_ONE_MINUS_SRC1_ALPHA,

        BLEND_FACTOR_MAX
    };

    enum BlendOperation
    {
        BLEND_OP_ADD,
        BLEND_OP_SUBTRACT,
        BLEND_OP_REVERSE_SUBTRACT,
        BLEND_OP_MINIMUM,
        BLEND_OP_MAXIMUM,

        BLEND_OP_MAX
    };

    struct PipelineRasterizationState
    {
        bool             enable_depth_clamp      = false;
        bool             discard_primitives      = false;
        bool             wireframe               = false;
        PolygonCullMode  cullMode                = POLYGON_CULL_DISABLED;
        PolygonFrontFace frontFace               = POLYGON_FRONT_FACE_CLOCKWISE;
        bool             depthBiasEnabled        = false;
        float            depthBiasConstantFactor = 0.0f;
        float            depthBiasClamp          = 0.0f;
        float            depthBiasSlopeFactor    = 0.0f;
        float            lineWidth               = 1.0f;
        uint32           patchControlPoints      = 1;
    };

    struct PipelineMultisampleState
    {
        TextureSamples      sampleCount           = TEXTURE_SAMPLES_1;
        bool                enableSampleShading   = false;
        float               minSampleShading      = 0.0f;
        std::vector<uint32> sampleMask            = {};
        bool                enableAlphaToCoverage = false;
        bool                enableAlphaToOne      = false;
    };

    struct PipelineDepthStencilState
    {
        bool            enableDepthTest  = false;
        bool            enableDepthWrite = false;
        bool            enableDepthRange = false;
        bool            enableStencil    = false;
        CompareOperator depthCompareOp   = COMPARE_OP_ALWAYS;
        float           depthRangeMin    = 0;
        float           depthRangeMax    = 0;

        struct StencilOperationState
        {
            StencilOperation fail        = STENCIL_OP_ZERO;
            StencilOperation pass        = STENCIL_OP_ZERO;
            StencilOperation depthFail   = STENCIL_OP_ZERO;
            CompareOperator  compare     = COMPARE_OP_ALWAYS;
            uint32           compareMask = 0;
            uint32           writeMask   = 0;
            uint32           reference   = 0;
        };

        StencilOperationState frontOp;
        StencilOperationState backOp;
    };

    struct PipelineColorBlendState
    {
        bool           enableLogicOp = false;
        LogicOperation logicOp       = LOGIC_OP_CLEAR;

        struct Attachment
        {
            bool           enableBlend         = false;
            BlendFactor    srcColorBlendFactor = BLEND_FACTOR_ZERO;
            BlendFactor    dstColorBlendFactor = BLEND_FACTOR_ZERO;
            BlendOperation colorBlendOp        = BLEND_OP_ADD;
            BlendFactor    srcAlphaBlendFactor = BLEND_FACTOR_ZERO;
            BlendFactor    dstAlphaBlendFactor = BLEND_FACTOR_ZERO;
            BlendOperation alphaBlendOp        = BLEND_OP_ADD;
            bool           write_r             = true;
            bool           write_g             = true;
            bool           write_b             = true;
            bool           write_a             = true;
        };

        static PipelineColorBlendState CreateDisabled(int attachments = 1)
        {
            PipelineColorBlendState bs;
            for (int i = 0; i < attachments; i++)
            {
                bs.attachments.push_back(Attachment());
            }

            return bs;
        }

        static PipelineColorBlendState CreateBlend(int attachments = 1)
        {
            PipelineColorBlendState bs;
            for (int i = 0; i < attachments; i++)
            {
                Attachment ba;
                ba.enableBlend = true;
                ba.srcColorBlendFactor = BLEND_FACTOR_SRC_ALPHA;
                ba.dstColorBlendFactor = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
                ba.srcAlphaBlendFactor = BLEND_FACTOR_SRC_ALPHA;
                ba.dstAlphaBlendFactor = BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;

                bs.attachments.push_back(ba);
            }

            return bs;
        }

        std::vector<Attachment> attachments;
        glm::vec4               blendConstant;
    };

    enum PipelineDynamicStateFlags
    {
        DYNAMIC_STATE_LINE_WIDTH           = SL_BIT(0),
        DYNAMIC_STATE_DEPTH_BIAS           = SL_BIT(1),
        DYNAMIC_STATE_BLEND_CONSTANTS      = SL_BIT(2),
        DYNAMIC_STATE_DEPTH_BOUNDS         = SL_BIT(3),
        DYNAMIC_STATE_STENCIL_COMPARE_MASK = SL_BIT(4),
        DYNAMIC_STATE_STENCIL_WRITE_MASK   = SL_BIT(5),
        DYNAMIC_STATE_STENCIL_REFERENCE    = SL_BIT(6),

        DYNAMIC_STATE_MAX = 7,
    };

    //================================================
    // コマンド
    //================================================
    union RenderPassClearValue
    {
        glm::vec4 color = {};
        struct { float  depth; uint32 stencil; };
    };

    struct AttachmentClear
    {
        TextureAspectBits    aspect;
        uint32               colorAttachment = 0xffffffff;
        RenderPassClearValue value;
    };

    struct BufferCopyRegion
    {
        uint64 srcOffset = 0;
        uint64 dstOffset = 0;
        uint64 size      = 0;
    };

    struct TextureCopyRegion
    {
        TextureSubresourceLayers srcSubresources;
        glm::ivec3               srcOffset;
        TextureSubresourceLayers dstSubresources;
        glm::ivec3               dstOffset;
        glm::ivec3               size;
    };

    struct BufferTextureCopyRegion
    {
        uint64                   bufferOffset = 0;
        TextureSubresourceLayers textureSubresources;
        glm::ivec3               textureOffset;
        glm::ivec3               textureRegionSize;
    };
    
    
}


