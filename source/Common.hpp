///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include <Langulus.hpp>

LANGULUS_EXCEPTION(Graphics);

LANGULUS_DEFINE_TRAIT(Shader, "Shader unit");
LANGULUS_DEFINE_TRAIT(MapMode, "Mapping mode");
LANGULUS_DEFINE_TRAIT(Resolution, "Resolution, usually a vector");
LANGULUS_DEFINE_TRAIT(Tesselation, "Tesselation, usually an integer");
LANGULUS_DEFINE_TRAIT(Topology, "Topology type");
LANGULUS_DEFINE_TRAIT(Interpolator, "Interpolation mode");
LANGULUS_DEFINE_TRAIT(Color, "Color, usually a Vec4");
LANGULUS_DEFINE_TRAIT(Material, "Material unit");
LANGULUS_DEFINE_TRAIT(Texture, "Texture unit");
LANGULUS_DEFINE_TRAIT(Geometry, "Geometry unit");
LANGULUS_DEFINE_TRAIT(Perspective, "Perspective state (boolean)");
LANGULUS_DEFINE_TRAIT(FOV, "Horizontal field of view angle, usually a real number");
LANGULUS_DEFINE_TRAIT(AspectRatio, "Aspect ratio trait (width / height), usually a real number");
LANGULUS_DEFINE_TRAIT(Viewport, "Viewport and depth clipping, usually a Range4");
LANGULUS_DEFINE_TRAIT(Projection, "Camera projection matrix");
LANGULUS_DEFINE_TRAIT(View, "Camera view matrix");
LANGULUS_DEFINE_TRAIT(Model, "Instance model matrix");

using namespace Langulus;
using namespace Langulus::Flow;
using namespace Langulus::Anyness;
using namespace Langulus::Entity;
using namespace Langulus::Math;

class Vulkan;
class VulkanRenderer;
class VulkanLayer;
class VulkanCamera;
class VulkanRenderable;
class VulkanPipeline;
class VulkanGeometry;
class VulkanTexture;
class VulkanShader;


///                                                                           
///   Blending modes                                                          
///                                                                           
/// Represents different tactics for blending colors                          
///                                                                           
struct BlendMode {
   LANGULUS(POD) true;
   LANGULUS(NULLIFIABLE) true;
   LANGULUS(INFO) "Blending mode";

   using Type = uint8_t;

   enum Enum : Type {
      // No blending - final color becomes source color                 
      None = 0,
      // No blending - final color becomes source color only            
      // if an alpha threshold is reached per pixel                     
      Threshold = 1,
      // Final color = source color * alpha + destination color         
      // * (1 - alpha)                                                  
      Alpha = 2,
      // Final color = source color + destination color                 
      Add = 3,
      // Final color = source color * destination color                 
      Multiply = 4,

      Counter
   };

protected:
   Type mMode {Enum::None};

   LANGULUS_NAMED_VALUES(Enum) {
      {"None",       Enum::None,
         "No blending - final color is always source color"},
      {"Threshold",  Enum::Threshold,
         "No blending - final color becomes source color only if an alpha threshold is reached per pixel"},
      {"Alpha",      Enum::Alpha,
         "Final color = source color * alpha + destination color * (1 - alpha)"},
      {"Add",        Enum::Add,
         "Final color = source color + destination color"},
      {"Multiply",   Enum::Multiply,
         "Final color = source color * destination color"}
   };

public:
   constexpr BlendMode() noexcept = default;
   constexpr BlendMode(Type) noexcept;
};


///                                                                           
///   General mapper                                                          
///                                                                           
/// Represents different tactics for mapping two properties to each other     
///                                                                           
struct MapMode {
   LANGULUS(POD) true;
   LANGULUS(NULLIFIABLE) true;
   LANGULUS(INFO) "Mapping mode";

   using Type = uint8_t;

   enum Enum : Type {
      // Texture coordinates are world coordinates                      
      World = 0,
      // Texture coordinates are vertex coordinates                     
      Model,
      // Texture coordinates are camera-like projection                 
      Projector,
      // Texture coordinates are screen coordinates                     
      Screen,
      // Planar projection                                              
      Plane,
      // Cylindrical projection                                         
      Cylinder,
      // Spherical projection                                           
      Sphere,
      // Cube projection (skybox)                                       
      Cube,
      // Face mapping, where each face maps to the same region          
      Face,
      // Injective projection, packing all faces in the map             
      Inject,
      // Contour projection, mapping texture along a spline             
      Contour,
      // Unfold projection, unwrapping texture along precut seams       
      Unfold,
      // Texture coordinates inside the generated geometry are used     
      // If any of the above mappings are used at generation-time,      
      // the mapping transits to this state                             
      Custom,

      Counter
   };

protected:
   Type mMode {Enum::World};

   LANGULUS_NAMED_VALUES(Enum) {
      {"World",      Enum::World,
         "Maps to transformed model's world vertex positions"},
      {"Model",      Enum::Model,
         "Maps to non-transformed vertex positions"},
      {"Projector",  Enum::Projector,
         "Maps to camera-like projection"},
      {"Screen",     Enum::Screen,
         "Maps to screen coordinates"},
      {"Plane",      Enum::Plane,
         "Maps to a planar projection"},
      {"Cylinder",   Enum::Cylinder,
         "Maps to a cylindrical projection"},
      {"Sphere",     Enum::Sphere,
         "Maps to a spherical projection with sharp warping at the poles"},
      {"Cube",       Enum::Cube,
         "Maps to a cube projection (useful for skyboxes)"},
      {"Face",       Enum::Face,
         "Maps each face to the same coordinates"},
      {"Inject",     Enum::Inject,
         "Packs all faces, preserving the relative scale, but ruining locality"},
      {"Contour",    Enum::Contour,
         "Maps along an edge (useful for texturing roads)"},
      {"Unfold",     Enum::Unfold,
         "Unwraps vertex positions along seams and stretches them like a pelt"},
      {"Custom",     Enum::Custom,
         "No mapping is done - you truly trust the raw data, even if it is invalid or non-existent"}
   };

public:
   constexpr MapMode() noexcept = default;
   constexpr MapMode(Type) noexcept;
};


///                                                                           
///   Shader stages                                                           
///                                                                           
namespace ShaderStage
{
   enum Enum : uint32_t {
      Vertex = 0,          // Vertex shader stage                       
      Geometry,            // Geometry shader stage                     
      TessCtrl,            // Tesselation control stage                 
      TessEval,            // Tesselation evaluation stage              
      Pixel,               // Pixel shader stage                        
      Compute,             // Compute shader stage                      

      Counter              // Shader stage counter (keep at end)        
   };

   /// Shader names                                                           
   constexpr Token Names[Enum::Counter] = {
      "vertex",
      "geometry",
      "tesselation control",
      "tesselation evaluation",
      "fragment",
      "compute"
   };
}

///                                                                           
///   Shader layout tokens                                                    
///                                                                           
namespace ShaderToken
{
   enum Enum {
      Version = 0,         // Version token                             
      Defines,             // Defines token                             
      Input,               // Input token                               
      Output,              // Output token                              
      Dependencies,        // Dependencies token                        
      Colorize,            // Colors token                              
      Transform,           // Transform token                           
      Position,            // Vertex position                           
      Uniform,             // Uniform variables                         
      Texturize,           // For utilizing samplers                    
      Functions,           // Function code token                       

      Counter              // Shader token counter                      
   };

   constexpr Token Names[Enum::Counter] = {
      "//#VERSION\n",
      "//#DEFINES\n",
      "//#INPUT\n",
      "//#OUTPUT\n",
      "//#DEPENDENCY\n",
      "//#COLORIZE\n",
      "//#TRANSFORM\n",
      "//#POSITION\n",
      "//#UNIFORM\n",
      "//#TEXTURIZE\n",
      "//#FUNCTIONS\n"
   };
}

///                                                                           
///   Refresh rates                                                           
///                                                                           
/// Represents the frequency at which data is computed                        
/// Many of these states map onto shader stages                               
///                                                                           
struct RefreshRate {
   LANGULUS(POD) true;
   LANGULUS(NULLIFIABLE) true;
   LANGULUS(INFO) "Refresh rate";

   using Type = uint8_t;

   enum Enum : Type {
      Auto = 0,            // Use the default refresh rate of the trait 
      None,                // A constant, essentially                   

      Tick,                // Updated once per time step                
      Pass,                // Updated once per a render pass            
      Camera,              // Updated for each camera                   
      Level,               // Updated for each level                    
      Renderable,          // Updated for each renderable               
      Instance,            // Updated for each instance                 

      // The following are mapped to ShaderStage::Enum                  
      Vertex,              // Updated in vertex shader                  
      Primitive,           // Updated in geometry shader                
      TessCtrl,            // Updated in tesselation control shader     
      TessEval,            // Updated in tesselation evaluation shader  
      Pixel,               // Updated in pixel shader                   

      Counter,
   };

protected:
   Type mMode {Enum::Auto};

   LANGULUS_NAMED_VALUES(Enum) {
      {"Auto",          Enum::Auto,
         "Automatically determined refresh rate, based on traits and context"},
      {"None",          Enum::None,
         "No refresh rate (a constant, never refreshes)"},

      {"Tick",          Enum::Tick,
         "Refresh once per tick (when flow moves forward in time)"},
      {"Pass",          Enum::Pass,
         "Refresh once per render pass"},
      {"Camera",        Enum::Camera,
         "Refresh once per camera"},
      {"Level",         Enum::Level,
         "Refresh once per level"},
      {"Renderable",    Enum::Renderable,
         "Refresh once per renderable"},
      {"Instance",      Enum::Instance,
         "Refresh once per instance"},

      {"Vertex",        Enum::Vertex,
         "Refresh once per vertex (inside vertex shader)"},
      {"Primitive",     Enum::Primitive,
         "Refresh once per geometric primitive (inside geometry shader)"},
      {"TessCtrl",      Enum::TessCtrl,
         "Refresh once per tesselation control unit (inside tesselation control shader)"},
      {"TessEval",      Enum::TessEval,
         "Refresh once per tesselation evaluation unit (inside tesselation evaluation shader)"},
      {"Pixel",         Enum::Pixel,
         "Refresh once per pixel (inside fragment shader)"},
   };

   // Rates that are considered shader stages, mapped to ShaderStage    
   static constexpr Offset StagesBegin = Enum::Vertex;
   static constexpr Offset StagesEnd = Enum::Counter;
   static constexpr Count StagesCount = StagesEnd - StagesBegin;

   // Rates that are considered uniforms                                
   static constexpr Offset UniformBegin = Enum::Tick;
   static constexpr Offset UniformEnd = StagesBegin;
   static constexpr Count UniformCount = UniformEnd - UniformBegin;

   // Rates that are considered inputs                                  
   static constexpr Offset InputBegin = UniformBegin;
   static constexpr Offset InputEnd = StagesEnd;
   static constexpr Count InputCount = InputEnd - InputBegin;

   // Rates that are considered static                                  
   static constexpr Offset StaticUniformBegin = UniformBegin;
   static constexpr Offset StaticUniformEnd = Enum::Camera;
   static constexpr Count StaticUniformCount = StaticUniformEnd - StaticUniformBegin;

   // Rates that are considered dynamic                                 
   static constexpr Offset DynamicUniformBegin = StaticUniformEnd;
   static constexpr Offset DynamicUniformEnd = UniformEnd;
   static constexpr Count DynamicUniformCount = DynamicUniformEnd - DynamicUniformBegin;

public:
   constexpr RefreshRate() noexcept = default;
   constexpr RefreshRate(Type) noexcept;

   constexpr bool IsUniform() const noexcept;
   constexpr bool IsStaticUniform() const noexcept;
   constexpr bool IsDynamicUniform() const noexcept;
   constexpr bool IsAttribute() const noexcept;
   constexpr bool IsInput() const noexcept;
   constexpr Offset GetInputIndex() const;
   constexpr Offset GetStaticUniformIndex() const;
   constexpr Offset GetDynamicUniformIndex() const;
   constexpr bool IsShaderStage() const noexcept;
   constexpr ShaderStage::Enum GetStageIndex() const;
};

using RRate = RefreshRate;


///                                                                           
///   Vulkan begins its existence here                                        
///                                                                           
#include <vulkan/vulkan_core.h>

using Shader = VkPipelineShaderStageCreateInfo;
using VertexInput = VkPipelineVertexInputStateCreateInfo;
using VertexAssembly = VkPipelineInputAssemblyStateCreateInfo;
using VertexBinding = VkVertexInputBindingDescription;
using VertexAttribute = VkVertexInputAttributeDescription;
using Topology = VkPrimitiveTopology;
using UBOLayout = VkDescriptorSetLayout;
using TextureList = TUnorderedMap<TMeta, const VulkanTexture*>;
using Frames = TAny<VkImageView>;
using FrameBuffers = TAny<VkFramebuffer>;
using CmdBuffers = TAny<VkCommandBuffer>;

constexpr uint32_t VK_INDEFINITELY = ::std::numeric_limits<uint32_t>::max();

/// This call must be implemented for each OS individually                    
/// It is the only platform dependent call                                    
bool CreateNativeVulkanSurfaceKHR(const VkInstance&, const void*, VkSurfaceKHR&);

/// Convert a meta data to a VK index format                                  
///   @param meta - the type definition to convert                            
///   @return the vulkan format equivalent                                    
LANGULUS(ALWAYSINLINE)
VkIndexType AsVkIndexType(DMeta meta) {
   if (meta->Is<uint32_t>())
      return VK_INDEX_TYPE_UINT32;
   else if (meta->Is<uint16_t>())
      return VK_INDEX_TYPE_UINT16;
   else if (meta->Is<uint8_t>())
      return VK_INDEX_TYPE_UINT8_EXT;
   else
      LANGULUS_THROW(Graphics, "Unsupported index type");
}

/// Convert meta data to a VK color/depth format                              
///   @param type - the type definition to convert                            
///   @param reverse - whether to invert color definition                     
///   @return the vulkan format equivalent                                    
inline VkFormat AsVkFormat(DMeta type, bool reverse = false) {
   switch (type->mSize) {
   case 1:
      if (type->CastsTo<uint8_t, true>(1))
         return VK_FORMAT_R8_UNORM;
      if (type->CastsTo<int8_t, true>(1))
         return VK_FORMAT_R8_SNORM;
      break;
   case 2:
      if (type->CastsTo<Depth16, true>(1))
         return VK_FORMAT_D16_UNORM;

      if (type->CastsTo<uint8_t, true>(2))
         return VK_FORMAT_R8G8_UNORM;
      if (type->CastsTo<int8_t, true>(2))
         return VK_FORMAT_R8G8_SNORM;

      if (type->CastsTo<uint16_t, true>(1))
         return VK_FORMAT_R16_UNORM;
      if (type->CastsTo<int16_t, true>(1))
         return VK_FORMAT_R16_SNORM;
      break;
   case 3:
      if (type->CastsTo<uint8_t, true>(3))
         return reverse ? VK_FORMAT_B8G8R8_UNORM : VK_FORMAT_R8G8B8_UNORM;
      if (type->CastsTo<int8_t, true>(3))
         return reverse ? VK_FORMAT_B8G8R8_SNORM : VK_FORMAT_R8G8B8_SNORM;
      break;
   case 4:
      if (type->CastsTo<Depth32, true>(1))
         return VK_FORMAT_D32_SFLOAT;

      if (type->CastsTo<Float, true>(1))
         return VK_FORMAT_R32_SFLOAT;

      if (type->CastsTo<uint8_t, true>(4))
         return reverse ? VK_FORMAT_B8G8R8A8_UNORM : VK_FORMAT_R8G8B8A8_UNORM;
      if (type->CastsTo<int8_t, true>(4))
         return reverse ? VK_FORMAT_B8G8R8A8_SNORM : VK_FORMAT_R8G8B8A8_SNORM;
      if (type->CastsTo<uint16_t, true>(2))
         return VK_FORMAT_R16G16_UNORM;
      if (type->CastsTo<int16_t, true>(2))
         return VK_FORMAT_R16G16_SNORM;

      if (type->CastsTo<uint32_t, true>(1))
         return VK_FORMAT_R32_UINT;
      if (type->CastsTo<int32_t, true>(1))
         return VK_FORMAT_R32_SINT;
      break;
   case 6:
      if (type->CastsTo<uint16_t, true>(3))
         return VK_FORMAT_R16G16B16_UNORM;
      if (type->CastsTo<int16_t, true>(3))
         return VK_FORMAT_R16G16B16_SNORM;
      break;
   case 8:
      if (type->CastsTo<Float, true>(2))
         return VK_FORMAT_R32G32_SFLOAT;
      if (type->CastsTo<Double, true>(1))
         return VK_FORMAT_R64_SFLOAT;

      if (type->CastsTo<uint16_t, true>(4))
         return VK_FORMAT_R16G16B16A16_UNORM;
      if (type->CastsTo<int16_t, true>(4))
         return VK_FORMAT_R16G16B16A16_SNORM;

      if (type->CastsTo<uint32_t, true>(2))
         return VK_FORMAT_R32G32_UINT;
      if (type->CastsTo<int32_t, true>(2))
         return VK_FORMAT_R32G32_SINT;

      if (type->CastsTo<uint64_t, true>(1))
         return VK_FORMAT_R64_UINT;
      if (type->CastsTo<int64_t, true>(1))
         return VK_FORMAT_R64_SINT;
      break;
   case 12:
      if (type->CastsTo<Float, true>(3))
         return VK_FORMAT_R32G32B32_SFLOAT;

      if (type->CastsTo<uint32_t, true>(3))
         return VK_FORMAT_R32G32B32_UINT;
      if (type->CastsTo<int32_t, true>(3))
         return VK_FORMAT_R32G32B32_SINT;
      break;
   case 16:
      if (type->CastsTo<Float, true>(4))
         return VK_FORMAT_R32G32B32A32_SFLOAT;
      if (type->CastsTo<Double, true>(2))
         return VK_FORMAT_R64G64_SFLOAT;

      if (type->CastsTo<uint32_t, true>(4))
         return VK_FORMAT_R32G32B32A32_UINT;
      if (type->CastsTo<int32_t, true>(4))
         return VK_FORMAT_R32G32B32A32_SINT;

      if (type->CastsTo<uint64_t, true>(2))
         return VK_FORMAT_R64G64_UINT;
      if (type->CastsTo<int64_t, true>(2))
         return VK_FORMAT_R64G64_SINT;
      break;
   case 24:
      if (type->CastsTo<Double, true>(3))
         return VK_FORMAT_R64G64B64_SFLOAT;

      if (type->CastsTo<uint64_t, true>(3))
         return VK_FORMAT_R64G64B64_UINT;
      if (type->CastsTo<int64_t, true>(3))
         return VK_FORMAT_R64G64B64_SINT;
      break;
   case 32:
      if (type->CastsTo<Double, true>(4))
         return VK_FORMAT_R64G64B64A64_SFLOAT;

      if (type->CastsTo<uint64_t, true>(4))
         return VK_FORMAT_R64G64B64A64_UINT;
      if (type->CastsTo<int64_t, true>(4))
         return VK_FORMAT_R64G64B64A64_SINT;
      break;
   default:
      break;
   }

   LANGULUS_THROW(Graphics, "Unsupported format");
}

/// Convert vulkan format to meta data                                        
///   @param type - the vulkan format type                                    
///   @param reverse - whether or not reversal is required                    
///   @return the piception equivalent                                        
constexpr DMeta VkFormatToDMeta(const VkFormat& type, bool& reverse) {
   reverse = false;
   switch (type) {
      case VK_FORMAT_D16_UNORM:
         return MetaData::Of<Depth16>();
      case VK_FORMAT_D32_SFLOAT:
         return MetaData::Of<Depth32>();
      case VK_FORMAT_R8_UNORM:
         return MetaData::Of<Red8>();
      case VK_FORMAT_R16_UNORM: 
         return MetaData::Of<uint16_t>();
      case VK_FORMAT_R32_UINT: 
         return MetaData::Of<uint32_t>();
      case VK_FORMAT_R64_UINT: 
         return MetaData::Of<uint64_t>();
      case VK_FORMAT_R8_SNORM: 
         return MetaData::Of<int8_t>();
      case VK_FORMAT_R16_SNORM: 
         return MetaData::Of<int16_t>();
      case VK_FORMAT_R32_SINT: 
         return MetaData::Of<int32_t>();
      case VK_FORMAT_R64_SINT: 
         return MetaData::Of<int64_t>();
      case VK_FORMAT_R32_SFLOAT: 
         return MetaData::Of<Red32>();
      case VK_FORMAT_R64_SFLOAT: 
         return MetaData::Of<Double>();
      case VK_FORMAT_R32G32_SFLOAT: 
         return MetaData::Of<Vec2f>();
      case VK_FORMAT_R64G64_SFLOAT: 
         return MetaData::Of<Vec2d>();
      case VK_FORMAT_R8G8B8_UNORM: case VK_FORMAT_B8G8R8_UNORM: 
         reverse = true;
         return MetaData::Of<RGB>();
      case VK_FORMAT_R32G32B32_SFLOAT:   
         return MetaData::Of<RGB96>();
      case VK_FORMAT_R64G64B64_SFLOAT:   
         return MetaData::Of<Vec3d>();
      case VK_FORMAT_R8G8B8A8_UNORM: case VK_FORMAT_B8G8R8A8_UNORM:
         reverse = true;
         return MetaData::Of<RGBA>();
      case VK_FORMAT_R32G32B32A32_SFLOAT:   
         return MetaData::Of<RGBA128>();
      case VK_FORMAT_R64G64B64A64_SFLOAT:   
         return MetaData::Of<Vec4d>();
      case VK_FORMAT_R8G8_UNORM:
         return MetaData::Of<Vec2u8>();
      case VK_FORMAT_R16G16_UNORM:
         return MetaData::Of<Vec2u16>();
      case VK_FORMAT_R32G32_UINT:
         return MetaData::Of<Vec2u32>();
      case VK_FORMAT_R64G64_UINT:
         return MetaData::Of<Vec2u64>();
      case VK_FORMAT_R8G8_SNORM:
         return MetaData::Of<Vec2i8>();
      case VK_FORMAT_R16G16_SNORM:
         return MetaData::Of<Vec2i16>();
      case VK_FORMAT_R32G32_SINT:
         return MetaData::Of<Vec2i32>();
      case VK_FORMAT_R64G64_SINT:
         return MetaData::Of<Vec2i64>();
      case VK_FORMAT_R16G16B16_UNORM:
         return MetaData::Of<Vec2u16>();
      case VK_FORMAT_R32G32B32_UINT:
         return MetaData::Of<Vec2u32>();
      case VK_FORMAT_R64G64B64_UINT:
         return MetaData::Of<Vec2u64>();
      case VK_FORMAT_R8G8B8_SNORM:
         return MetaData::Of<Vec3i8>();
      case VK_FORMAT_R16G16B16_SNORM:
         return MetaData::Of<Vec3i16>();
      case VK_FORMAT_R32G32B32_SINT:
         return MetaData::Of<Vec3i32>();
      case VK_FORMAT_R64G64B64_SINT:
         return MetaData::Of<Vec3i64>();
      case VK_FORMAT_R16G16B16A16_UNORM:
         return MetaData::Of<Vec4u16>();
      case VK_FORMAT_R32G32B32A32_UINT:
         return MetaData::Of<Vec4u32>();
      case VK_FORMAT_R64G64B64A64_UINT:
         return MetaData::Of<Vec4u64>();
      case VK_FORMAT_R8G8B8A8_SNORM:
         return MetaData::Of<Vec4i8>();
      case VK_FORMAT_R16G16B16A16_SNORM:
         return MetaData::Of<Vec4i16>();
      case VK_FORMAT_R32G32B32A32_SINT:
         return MetaData::Of<Vec4i32>();
      case VK_FORMAT_R64G64B64A64_SINT:
         return MetaData::Of<Vec4i64>();
      default:
         break;
   }

   LANGULUS_THROW(Graphics, "Unsupported format");
}

/// Converter from PC shader to VK shader stage                               
///   @param stage - the shader stage                                         
///   @return the vulkan equivalent                                           
constexpr VkShaderStageFlagBits AsVkStage(ShaderStage::Enum stage) noexcept {
   constexpr VkShaderStageFlagBits vkstages[ShaderStage::Counter] = {
      VK_SHADER_STAGE_VERTEX_BIT,
      VK_SHADER_STAGE_GEOMETRY_BIT,
      VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
      VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
      VK_SHADER_STAGE_FRAGMENT_BIT,
      VK_SHADER_STAGE_COMPUTE_BIT
   };

   return vkstages[stage];
}

/// Converter from PC primitive to VK primitive                               
///   @param meta - the primitive type                                        
///   @return the vulkan equivalent                                           
inline VkPrimitiveTopology AsVkPrimitive(DMeta meta) {
   if (meta->CastsTo<A::Point>())
      return VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
   if (meta->CastsTo<A::TriangleStrip>())
      return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
   if (meta->CastsTo<A::TriangleFan>())
      return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
   if (meta->CastsTo<A::Triangle>())
      return VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
   if (meta->CastsTo<A::LineStrip>())
      return VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
   if (meta->CastsTo<A::Line>())
      return VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

   LANGULUS_THROW(Graphics, "Unsupported topology");
}


///                                                                           
///   Vertex/index buffer view                                                
///                                                                           
struct VertexView {
   // Number of primitives                                              
   Count mPrimitiveCount = 0;
   // Starting primitive                                                
   Offset mPrimitiveStart = 0;
   // Number of indices                                                 
   Count mIndexCount = 0;
   // Starting index                                                    
   Offset mIndexStart = 0;
   // Data topology                                                     
   DMeta mPrimitiveType = nullptr;
   // Double-sidedness                                                  
   bool mBilateral = false;

   bool operator == (const VertexView&) const noexcept;
   VertexView Decay() const;
};


///                                                                           
///   Universal pixel buffer view                                             
///                                                                           
struct PixelView {
   Count mWidth = 1;
   Count mHeight = 1;
   Count mDepth = 1;
   Count mFrames = 1;
   DMeta mFormat = nullptr;
   // Reverse RGBA to BGRA                                              
   // This is not a scalable solution and would eventually fail         
   bool mReverseFormat = false;

   constexpr PixelView() noexcept = default;
   constexpr PixelView(Count, Count, Count, Count, DMeta, bool reversed = false) noexcept;

   bool operator == (const PixelView&) const noexcept;

   NOD() constexpr Count CountPixels() const noexcept;
   NOD() constexpr Count CountDimensions() const noexcept;
   NOD() Size GetPixelSize() const noexcept;
   NOD() Size CountBytes() const noexcept;
   NOD() Count CountComponents() const noexcept;
};

using LodIndex = int32_t;
using AbsoluteLodIndex = uint32_t;


///                                                                           
///   Level of detail state                                                   
///                                                                           
/// A helper structure that is used to fetch the correct LOD level LOD level  
/// is simply a geometry, that is designed to represent a zoomed-in or a      
/// zoomed-out region of another geometry. These regions can be generated on  
/// the fly, may reuse existing geometry or may not exist at all              
///                                                                           
struct LodState {
   Level mLevel;
   Matrix4 mView;
   Matrix4 mViewInverted;
   Matrix4 mModel;
   TFrustum<Vec3> mFrustum;

   // Calculated after Transform()                                      
   Matrix4 mModelView;
   Vec4 mOrigin;
   Real mRadius;
   Real mDistanceToSurface;
   Real mLODIndex;

   static constexpr LodIndex MinIndex = -6;
   static constexpr LodIndex MaxIndex = 6;
   static constexpr LodIndex IndexCount = MaxIndex - MinIndex + 1;

   /// Recalculate LOD index for identity model matrix                        
   void Transform() {
      mModel = Matrix4::Identity();
      mModelView = mViewInverted;
      mOrigin = {};
      mRadius = {};
      mDistanceToSurface = {};
      mLODIndex = 0;
   }

   /// Recalculate LOD index by specifying the model matrix                   
   ///   @param model - the model transformation                              
   void Transform(const Matrix4& model) {
      mModel = model;
      mModelView = mModel * mViewInverted;
      mOrigin = mModelView.GetPosition();
      mRadius = mModelView.GetScale().HMax() * Real {0.5};
      mDistanceToSurface = mOrigin.Length() - mRadius;
      mLODIndex = 0;
      if (mDistanceToSurface > 0 && mRadius > 0) {
         // View is outside the sphere                                  
         const Real near = ::std::log10(mRadius / mDistanceToSurface);
         const Real far = ::std::log10(mDistanceToSurface / mRadius);
         mLODIndex = Math::Clamp(near - far, Real(MinIndex), Real(MaxIndex));
      }
      else if (mDistanceToSurface < 0) {
         // Being inside the sphere always gives the most detail        
         mLODIndex = Real(MaxIndex);
      }
   }

   /// Get the distance from the camera to the model's bounding sphere        
   /// surface, divided by the bounding sphere size                           
   ///   @return the distance                                                 
   Real GetNormalizedDistance() const noexcept {
      return mDistanceToSurface / mRadius;
   }

   /// Return the LOD index, which is a real number in the range              
   /// [MinIndex;MaxIndex]                                                    
   /// Calculate the LOD index via log10 distance from a sphere to            
   /// the camera view. You can imagine it as the number of zeroes behind     
   /// or in front of the distance                                            
   /// If index is below zero, then we're observing from afar                 
   /// If index is above zero, then we're observing too close                 
   /// At zero we're observing the default quality asset                      
   ///   @return the LOD index                                                
   Real GetIndex() const noexcept {
      return mLODIndex;
   }

   /// Get LOD index in the range [0;IndexCount)                              
   ///   @return the absolute index                                           
   AbsoluteLodIndex GetAbsoluteIndex() const noexcept {
      return AbsoluteLodIndex(LodIndex(mLODIndex) - MinIndex);
   }
};

///                                                                           
///   VRAM content mirror                                                     
///                                                                           
class ContentVRAM : public Unit {
protected:
   Ptr<Unit> mOriginalContent;
   bool mContentMirrored = false;

public:
   /// VRAM content hash is the same as the original content                  
   NOD() LANGULUS(ALWAYSINLINE)
   Hash GetHash() const {
      return mOriginalContent.GetHash();
   }

   /// Compare VRAM by comparing original contents                            
   LANGULUS(ALWAYSINLINE)
   bool operator == (const ContentVRAM& other) const noexcept {
      return mOriginalContent == other.mOriginalContent;
   }
};

/// Helper function for converting colors to floats                           
/// If data is made of integers, colors will be normalized in the 255 range   
///   @param color - the color to convert to RGBAf                            
///   @return a floating point RGBA vector                                    
LANGULUS(ALWAYSINLINE)
RGBAf AnyColorToVector(const Any& color) {
   // Inspect the data pack, what color components does it contain?     
   const auto redChannel = color.GetMember(MetaTrait::Of<Traits::R>());
   const auto greenChannel = color.GetMember(MetaTrait::Of<Traits::G>());
   const auto blueChannel = color.GetMember(MetaTrait::Of<Traits::B>());
   const auto alphaChannel = color.GetMember(MetaTrait::Of<Traits::A>());

   // Get the values (and normalize them if we have to)                 
   RGBAf result;
   if (!redChannel.IsEmpty()) {
      result[0] = redChannel.AsCast<Real>();
      if (redChannel.CastsTo<A::Integer>())
         result[0] /= 255.0f;
   }
   
   if (!greenChannel.IsEmpty()) {
      result[1] = greenChannel.AsCast<Real>();
      if (greenChannel.CastsTo<A::Integer>())
         result[1] /= 255.0f;
   }

   if (!blueChannel.IsEmpty()) {
      result[2] = blueChannel.AsCast<Real>();
      if (blueChannel.CastsTo<A::Integer>())
         result[2] /= 255.0f;
   }

   if (!alphaChannel.IsEmpty()) {
      result[3] = alphaChannel.AsCast<Real>();
      if (alphaChannel.CastsTo<A::Integer>())
         result[3] /= 255.0f;
   }

   return result;
}