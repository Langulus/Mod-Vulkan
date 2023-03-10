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
//LANGULUS_DEFINE_TRAIT(Resolution, "Resolution, usually a vector");
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

using namespace Langulus;
using namespace Langulus::Flow;
using namespace Langulus::Anyness;
using namespace Langulus::Entity;
using namespace Langulus::Math;

struct Vulkan;
struct VulkanMemory;
struct VulkanRenderer;
struct VulkanLayer;
struct VulkanCamera;
struct VulkanLight;
struct VulkanRenderable;
struct VulkanPipeline;
struct VulkanGeometry;
struct VulkanTexture;
struct VulkanShader;


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
   constexpr Token Version       = "//#VERSION\n";
   constexpr Token Defines       = "//#DEFINES\n";
   constexpr Token Input         = "//#INPUT\n";
   constexpr Token Output        = "//#OUTPUT\n";
   constexpr Token Dependencies  = "//#DEPENDENCY\n";
   constexpr Token Colorize      = "//#COLORIZE\n";
   constexpr Token Transform     = "//#TRANSFORM\n";
   constexpr Token Position      = "//#POSITION\n";
   constexpr Token Uniform       = "//#UNIFORM\n";
   constexpr Token Texturize     = "//#TEXTURIZE\n";
   constexpr Token Functions     = "//#FUNCTIONS\n";
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
   constexpr RefreshRate(const CT::DenseNumber auto&) noexcept;
   constexpr RefreshRate(const Enum&) noexcept;

   NOD() constexpr bool IsUniform() const noexcept;
   NOD() constexpr bool IsStaticUniform() const noexcept;
   NOD() constexpr bool IsDynamicUniform() const noexcept;
   NOD() constexpr bool IsAttribute() const noexcept;
   NOD() constexpr bool IsInput() const noexcept;
   NOD() constexpr bool IsShaderStage() const noexcept;
   NOD() constexpr Offset GetInputIndex() const;
   NOD() constexpr Offset GetStaticUniformIndex() const;
   NOD() constexpr Offset GetDynamicUniformIndex() const;
   NOD() constexpr ShaderStage::Enum GetStageIndex() const;

   constexpr operator Enum () const noexcept {
      return static_cast<Enum>(mMode);
   }
};

using Rate = RefreshRate;


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
using FrameBuffers = ::std::vector<VkFramebuffer>;
using CmdBuffers = ::std::vector<VkCommandBuffer>;
using TokenSet = ::std::vector<const char*>;

constexpr uint32_t VK_INDEFINITELY = ::std::numeric_limits<uint32_t>::max();

/// These calls must be implemented for each OS individually                  
bool CreateNativeVulkanSurfaceKHR(const VkInstance&, const A::Window*, VkSurfaceKHR&);

NOD() TokenSet GetRequiredExtensions();
NOD() VkIndexType AsVkIndexType(DMeta);
NOD() VkFormat AsVkFormat(DMeta, bool reverse = false);
NOD() DMeta VkFormatToDMeta(const VkFormat&, bool& reverse);
NOD() constexpr VkShaderStageFlagBits AsVkStage(ShaderStage::Enum) noexcept;
NOD() VkPrimitiveTopology AsVkPrimitive(DMeta);
NOD() RGBAf AnyColorToVector(const Any&);

#include "Common.inl"