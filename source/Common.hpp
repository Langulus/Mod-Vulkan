///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include <Entity/External.hpp>
#include <Math/Colors.hpp>
#include <Math/Primitives.hpp>

LANGULUS_EXCEPTION(Graphics);

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
using QueueFamilies = TAny<uint32_t>;

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