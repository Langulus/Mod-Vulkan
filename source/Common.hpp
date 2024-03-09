///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include <Entity/External.hpp>
#include <Math/Color.hpp>

LANGULUS_EXCEPTION(Graphics);

using namespace Langulus;

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

#if 1
   #define VERBOSE_VULKAN_ENABLED() 1
   #define VERBOSE_VULKAN(...)      Logger::Verbose(Self(), __VA_ARGS__)
   #define VERBOSE_VULKAN_TAB(...)  const auto tab = Logger::Verbose(Self(), __VA_ARGS__, Logger::Tabs {})
#else
   #define VERBOSE_VULKAN_ENABLED() 0
   #define VERBOSE_VULKAN(...)      
   #define VERBOSE_VULKAN_TAB(...)  
#endif

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
using FrameViews = TAny<VkImageView>;
using FrameBuffers = TAny<VkFramebuffer>;
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
