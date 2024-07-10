///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright (c) 2020 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include <Langulus.hpp>
#include <Math/Color.hpp>
#include <Langulus/Material.hpp>
#include <Langulus/Graphics.hpp>
#include <Langulus/Platform.hpp>

LANGULUS_EXCEPTION(Graphics);

using namespace Langulus;
using namespace Math;

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
   #define VERBOSE_VULKAN_TAB(...)  const auto tab = Logger::VerboseTab(Self(), __VA_ARGS__)
#else
   #define VERBOSE_VULKAN_ENABLED() 0
   #define VERBOSE_VULKAN(...)      LANGULUS(NOOP)
   #define VERBOSE_VULKAN_TAB(...)  LANGULUS(NOOP)
#endif

///                                                                           
///   Vulkan begins its existence here                                        
///                                                                           
#include <vulkan/vulkan_core.h>

using Shader          = VkPipelineShaderStageCreateInfo;
using VertexInput     = VkPipelineVertexInputStateCreateInfo;
using VertexAssembly  = VkPipelineInputAssemblyStateCreateInfo;
using VertexBinding   = VkVertexInputBindingDescription;
using VertexAttribute = VkVertexInputAttributeDescription;
using Topology        = VkPrimitiveTopology;
using UBOLayout       = VkDescriptorSetLayout;
using TextureList     = TUnorderedMap<TMeta, const VulkanTexture*>;
using FrameViews      = TMany<VkImageView>;
using FrameBuffers    = TMany<VkFramebuffer>;
using CmdBuffers      = ::std::vector<VkCommandBuffer>;
using TokenSet        = ::std::vector<const char*>;
using QueueFamilies   = TMany<uint32_t>;

constexpr uint32_t VK_INDEFINITELY = ::std::numeric_limits<uint32_t>::max();

/// These calls must be implemented for each OS individually                  
bool CreateNativeVulkanSurfaceKHR(const VkInstance&, const A::Window*, VkSurfaceKHR&);

NOD() auto GetRequiredExtensions() -> TokenSet;
NOD() auto AsVkIndexType(DMeta) -> VkIndexType;
NOD() auto AsVkFormat(DMeta, bool reverse = false) -> VkFormat;
NOD() auto VkFormatToDMeta(const VkFormat&, bool& reverse) -> DMeta;
NOD() constexpr auto AsVkStage(ShaderStage::Enum) noexcept -> VkShaderStageFlagBits;
NOD() auto AsVkPrimitive(DMeta) -> VkPrimitiveTopology;
NOD() auto AnyColorToVector(const Many&) -> RGBAf;

#include "Common.inl"
