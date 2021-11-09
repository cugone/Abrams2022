#pragma once

#include "Engine/Core/TypeUtils.hpp"

#include <cstdint>
#include <ratio>
#include <string>
#include <type_traits>
#include <utility>

struct ViewportDesc {
    float x = 0.0f;
    float y = 0.0f;
    float width = 0.0f;
    float height = 0.0f;
    float minDepth = 0.0f;
    float maxDepth = 1.0f;
};

bool operator==(const ViewportDesc& a, const ViewportDesc& b) noexcept;
bool operator!=(const ViewportDesc& a, const ViewportDesc& b) noexcept;

struct DisplayDesc {
    unsigned int width = 0u;
    unsigned int height = 0u;
    unsigned int refreshRateHz = 0u;
};

struct DisplayDescLTComparator {
    bool operator()(const DisplayDesc& a, const DisplayDesc& b) const noexcept;
};

struct DisplayDescGTComparator {
    bool operator()(const DisplayDesc& a, const DisplayDesc& b) const noexcept;
};

struct GraphicsCardDesc {
    std::string Description{};
    std::size_t DedicatedVideoMemory = 0u;
    std::size_t DedicatedSystemMemory = 0u;
    std::size_t SharedSystemMemory = 0u;
    unsigned int VendorId = 0u;
    unsigned int DeviceId = 0u;
    unsigned int SubSysId = 0u;
    unsigned int Revision = 0u;
    bool is_software = false;
    bool is_unspecified = false;
    friend std::ostream& operator<<(std::ostream& out_stream, const GraphicsCardDesc& adapterInfo) noexcept;
};
// clang-format off
enum class RHIOutputMode : uint8_t {
    First_
    , Windowed = First_
    , Borderless_Fullscreen
    , Last_
};

template<>
struct TypeUtils::is_incrementable_enum_type<RHIOutputMode> : std::true_type {};

enum class BufferType : uint8_t {
    None
    , Vertex
    , Index
    , Structured
    , Constant
    , ReadWrite
};

enum class RenderTargetType : uint8_t {
    None
    , Color
    , Depth
    , Both
};

enum class PipelineStage : uint8_t {
    None = 0b00000000
    , Vs = 0b00000001
    , Hs = 0b00000010
    , Ds = 0b00000100
    , Gs = 0b00001000
    , Ps = 0b00010000
    , Cs = 0b00100000
    , All = Vs | Hs | Ds | Gs | Ps | Cs
};

template<>
struct TypeUtils::is_bitflag_enum_type<PipelineStage> : std::true_type {};

enum class ComparisonFunction {
    Never
    , Less
    , Equal
    , Less_Equal
    , Greater
    , Not_Equal
    , Greater_Equal
    , Always
};

enum class StencilOperation {
    Keep
    , Zero
    , Replace
    , Increment_Clamp
    , Decrement_Clamp
    , Invert
    , Increment_Wrap
    , Decrement_Wrap
};

enum class ImageFormat : unsigned int {
    Unknown
    , R32G32B32A32_Typeless
    , R32G32B32A32_Float
    , R32G32B32A32_UInt
    , R32G32B32A32_SInt
    , R32G32B32_Typeless
    , R32G32B32_Float
    , R32G32B32_UInt
    , R32G32B32_SInt
    , R16G16B16A16_Typeless
    , R16G16B16A16_Float
    , R16G16B16A16_UNorm
    , R16G16B16A16_UInt
    , R16G16B16A16_SNorm
    , R16G16B16A16_SInt
    , R32G32_Typeless
    , R32G32_Float
    , R32G32_UInt
    , R32G32_SInt
    , R32G8X24_Typeless
    , D32_Float_S8X24_UInt
    , R32_Float_X8X24_Typeless
    , X32_Typeless_G8X24_UInt
    , R10G10B10A2_Typeless
    , R10G10B10A2_UNorm
    , R10G10B10A2_UInt
    , R11G11B10_Float
    , R8G8B8A8_Typeless
    , R8G8B8A8_UNorm
    , R8G8B8A8_UNorm_Srgb
    , R8G8B8A8_UInt
    , R8G8B8A8_SNorm
    , R8G8B8A8_SInt
    , R16G16_Typeless
    , R16G16_Float
    , R16G16_UNorm
    , R16G16_UInt
    , R16G16_SNorm
    , R16G16_SInt
    , R32_Typeless
    , D32_Float
    , R32_Float
    , R32_UInt
    , R32_SInt
    , R24G8_Typeless
    , D24_UNorm_S8_UInt
    , R24_UNorm_X8_Typeless
    , X24_Typeless_G8_UInt
    , R8G8_Typeless
    , R8G8_UNorm
    , R8G8_UInt
    , R8G8_SNorm
    , R8G8_SInt
    , R16_Typeless
    , R16_Float
    , D16_UNorm
    , R16_UNorm
    , R16_UInt
    , R16_SNorm
    , R16_SInt
    , R8_Typeless
    , R8_UNorm
    , R8_UInt
    , R8_SNorm
    , R8_SInt
    , A8_UNorm
    , R1_UNorm
    , R9G9B9E5_SharedExp
    , R8G8_B8G8_UNorm
    , G8R8_G8B8_UNorm
    , BC1_Typeless
    , BC1_UNorm
    , BC1_UNorm_Srgb
    , BC2_Typeless
    , BC2_UNorm
    , BC2_UNorm_Srgb
    , BC3_Typeless
    , BC3_UNorm
    , BC3_UNorm_Srgb
    , BC4_Typeless
    , BC4_UNorm
    , BC4_SNorm
    , BC5_Typeless
    , BC5_UNorm
    , BC5_SNorm
    , B5G6R5_UNorm
    , B5G5R5A1_UNorm
    , B8G8R8A8_UNorm
    , B8G8R8X8_UNorm
    , R10G10B10_XR_Bias_A2_UNorm
    , B8G8R8A8_Typeless
    , B8G8R8A8_UNorm_Srgb
    , B8G8R8X8_Typeless
    , B8G8R8X8_UNorm_Srgb
    , BC6H_Typeless
    , BC6H_UF16
    , BC6H_SF16
    , BC7_Typeless
    , BC7_UNorm
    , BC7_UNorm_Srgb
    , Ayuv
    , Y410
    , Y416
    , Nv12
    , P010
    , P016
    , Opaque_420
    , Yuy2
    , Y210
    , Y216
    , Nv11
    , Ai44
    , Ia44
    , P8
    , A8P8
    , B4G4R4A4_UNorm
};

enum class PrimitiveType : uint32_t {
    None
    , Points
    , Lines
    , Triangles
    , Lines_Adj
    , LinesStrip
    , LinesStrip_Adj
    , TriangleStrip
    , Triangles_Adj
    , TriangleStrip_Adj
    , Control_Point_PatchList_1
    , Control_Point_PatchList_2
    , Control_Point_PatchList_3
    , Control_Point_PatchList_4
    , Control_Point_PatchList_5
    , Control_Point_PatchList_6
    , Control_Point_PatchList_7
    , Control_Point_PatchList_8
    , Control_Point_PatchList_9
    , Control_Point_PatchList_10
    , Control_Point_PatchList_11
    , Control_Point_PatchList_12
    , Control_Point_PatchList_13
    , Control_Point_PatchList_14
    , Control_Point_PatchList_15
    , Control_Point_PatchList_16
    , Control_Point_PatchList_17
    , Control_Point_PatchList_18
    , Control_Point_PatchList_19
    , Control_Point_PatchList_20
    , Control_Point_PatchList_21
    , Control_Point_PatchList_22
    , Control_Point_PatchList_23
    , Control_Point_PatchList_24
    , Control_Point_PatchList_25
    , Control_Point_PatchList_26
    , Control_Point_PatchList_27
    , Control_Point_PatchList_28
    , Control_Point_PatchList_29
    , Control_Point_PatchList_30
    , Control_Point_PatchList_31
    , Control_Point_PatchList_32
};

enum class BufferUsage : uint8_t {
    Default = 0b00000000
    , Gpu = 0b00000001
    , Static = 0b00000010
    , Dynamic = 0b00000100
    , Staging = 0b00001000
};

template<>
struct TypeUtils::is_bitflag_enum_type<BufferUsage> : std::true_type {};

enum class BufferBindUsage : uint32_t {
    Vertex_Buffer = 0x001
    , Index_Buffer = 0x002
    , Constant_Buffer = 0x004
    , Shader_Resource = 0x008
    , Stream_Output = 0x010
    , Render_Target = 0x020
    , Depth_Stencil = 0x040
    , Unordered_Access = 0x080
    , Decoder = 0x100
    , Video_Encoder = 0x200
};

template<>
struct TypeUtils::is_bitflag_enum_type<BufferBindUsage> : std::true_type {};

enum class FilterMode {
    Point
    , Linear
    , Anisotropic
};

enum class FilterComparisonMode {
    None
    , Minimum
    , Maximum
    , Comparison
};

enum class TextureAddressMode {
    Wrap
    , Mirror
    , Clamp
    , Border
    , Mirror_Once
};

enum class BlendFactor {
    Zero
    , One
    , Src_Color
    , Inv_Src_Color
    , Src_Alpha
    , Inv_Src_Alpha
    , Dest_Alpha
    , Inv_Dest_Alpha
    , Dest_Color
    , Inv_Dest_Color
    , Src_Alpha_Sat
    , Blend_Factor
    , Inv_Blend_Factor
    , Src1_Color
    , Inv_Src1_Color
    , Src1_Alpha
    , Inv_Src1_Alpha
};

enum class BlendOperation {
    Add
    , Subtract
    , Reverse_Subtract
    , Min
    , Max
};

enum class BlendColorWriteEnable : unsigned char {
    None = 0x00
    , Red = 0x01
    , Green = 0x02
    , Blue = 0x04
    , Alpha = 0x08
    , All = 0x0F
};

template<>
struct TypeUtils::is_bitflag_enum_type<BlendColorWriteEnable> : std::true_type {};

enum class FillMode {
    Solid
    , Wireframe
};

enum class CullMode {
    None
    , Front
    , Back
};

enum class WindingOrder {
    CCW
    , CW
};

enum class ResourceMiscFlag : long {
    Generate_Mips = 0x1L
    , Shared = 0x2L
    , TextureCube = 0x4L
    , DrawIndirect_Args = 0x10L
    , Allow_Raw_Views = 0x20L
    , Structured_Buffer = 0x40L
    , Resource_Clamp = 0x80L
    , Shared_KeyedMutex = 0x100L
    , Gdi_Compatible = 0x200L
    , Shared_NthHandle = 0x800L
    , Restricted_Content = 0x1000L
    , Restrict_Shared_Resource = 0x2000L
    , Restrict_Shared_Resource_Driver = 0x4000L
    , Guarded = 0x8000L
    , Tile_Pool = 0x20000L
    , Tiled = 0x40000L
    , Hw_Protected = 0x80000L
};

template<>
struct TypeUtils::is_bitflag_enum_type<ResourceMiscFlag> : std::true_type {};
// clang-format on
