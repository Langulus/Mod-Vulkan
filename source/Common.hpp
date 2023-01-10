///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include <Langulus.hpp>

using namespace Langulus;
using namespace Langulus::Flow;
using namespace Langulus::Anyness;
using namespace Langulus::Entity;
using namespace Langulus::Math;

/// Toggle vulkan debug layers and default precision from here                
#define LGLS_VKVERBOSE() LANGULUS_DISABLED()

#include <vulkan/vulkan_core.h>

// Undefine shitty windows macros >:(                                   
#undef IsMinimized

LANGULUS_DEFINE_TRAIT(Shader, "Shader trait");

class CVulkanRenderer;
class CVulkanLayer;
class CVulkanCamera;
class CVulkanRenderable;
class CVulkanPipeline;
class CVulkanGeometry;
class CVulkanTexture;
class CVulkanShader;

/// Shorter Vulkan types                                                      
using Shader = VkPipelineShaderStageCreateInfo;
using VertexInput = VkPipelineVertexInputStateCreateInfo;
using VertexAssembly = VkPipelineInputAssemblyStateCreateInfo;
using VertexBinding = VkVertexInputBindingDescription;
using VertexAttribute = VkVertexInputAttributeDescription;
using Topology = VkPrimitiveTopology;
using UBOLayout = VkDescriptorSetLayout;
using TextureList = TAny<const CVulkanTexture*>;
using Frames = std::vector<VkImageView>;
using FrameBuffers = std::vector<VkFramebuffer>;
using CmdBuffers = std::vector<VkCommandBuffer>;

struct PipeSubscriber {
   uint32_t offsets[RRate::DynamicUniformCount] {};
   pcptr samplerSet {};
   pcptr geometrySet {};
};

struct LayerSubscriber {
   const CVulkanPipeline* pipeline {};
   PipeSubscriber sub {};
};

#define VK_INDEFINITELY std::numeric_limits<uint32_t>::max()

/// This call must be implemented for each OS individually                    
bool CreateNativeVulkanSurfaceKHR(const VkInstance&, const void*, VkSurfaceKHR&);


/// Convert a PC format to a VK index format                                  
///   @param meta - the type definition to convert                            
///   @return the vulkan format equivalent                                    
inline VkIndexType pcTypeToVkIndexType(DMeta meta) {
   switch (meta->GetSwitch()) {
   case DataID::Switch<pcu16>():
      return VkIndexType::VK_INDEX_TYPE_UINT16;
   case DataID::Switch<pcu32>():
      return VkIndexType::VK_INDEX_TYPE_UINT32;
   default:
      throw Except::Graphics(pcLogFuncError
         << "Unsupported index type " << meta->GetToken());
   }
}

/// Convert a PC format to a VK format                                        
///   @param type - the type definition to convert                            
///   @param reverse - whether to invert color definition                     
///   @return the vulkan format equivalent                                    
inline VkFormat pcTypeToVkFormat(DMeta type, bool reverse = false) {
   if (type->Is<depth32>())
      return VK_FORMAT_D32_SFLOAT;
   else if (type->Is<depth16>())
      return VK_FORMAT_D16_UNORM;

   LinkedBase base;
   // Unsigned integers                                                 
   if (type->GetBase<pcu8>(0, base) && base.mStaticBase.mMapping) {
      switch (base.mStaticBase.mCount) {
      case 1:   return VK_FORMAT_R8_UNORM;
      case 2:   return VK_FORMAT_R8G8_UNORM;
      case 3:  return reverse ? VK_FORMAT_B8G8R8_UNORM : VK_FORMAT_R8G8B8_UNORM;
      case 4:  return reverse ? VK_FORMAT_B8G8R8A8_UNORM : VK_FORMAT_R8G8B8A8_UNORM;
      default: goto fallback;
      }
   }
   else if (type->GetBase<pcu16>(0, base) && base.mStaticBase.mMapping) {
      switch (base.mStaticBase.mCount) {
      case 1:   return VK_FORMAT_R16_UNORM;
      case 2:   return VK_FORMAT_R16G16_UNORM;
      case 3:  return /*reverse ? VK_FORMAT_B16G16R16_UNORM :*/ VK_FORMAT_R16G16B16_UNORM;
      case 4:  return /*reverse ? VK_FORMAT_B16G16R16A16_UNORM :*/ VK_FORMAT_R16G16B16A16_UNORM;
      default: goto fallback;
      }
   }
   else if (type->GetBase<pcu32>(0, base) && base.mStaticBase.mMapping) {
      switch (base.mStaticBase.mCount) {
      case 1:   return VK_FORMAT_R32_UINT;
      case 2:   return VK_FORMAT_R32G32_UINT;
      case 3:  return /*reverse ? VK_FORMAT_B32G32R32_UINT :*/ VK_FORMAT_R32G32B32_UINT;
      case 4:  return /*reverse ? VK_FORMAT_B32G32R32A32_UINT :*/ VK_FORMAT_R32G32B32A32_UINT;
      default: goto fallback;
      }
   }
   else if (type->GetBase<pcu64>(0, base) && base.mStaticBase.mMapping) {
      switch (base.mStaticBase.mCount) {
      case 1:   return VK_FORMAT_R64_UINT;
      case 2:   return VK_FORMAT_R64G64_UINT;
      case 3:  return /*reverse ? VK_FORMAT_B64G64R64_UINT :*/ VK_FORMAT_R64G64B64_UINT;
      case 4:  return /*reverse ? VK_FORMAT_B64G64R64A64_UINT :*/ VK_FORMAT_R64G64B64A64_UINT;
      default: goto fallback;
      }
   }
   // Signed integers                                                   
   else if (type->GetBase<pci8>(0, base) && base.mStaticBase.mMapping) {
      switch (base.mStaticBase.mCount) {
      case 1:   return VK_FORMAT_R8_SNORM;
      case 2:   return VK_FORMAT_R8G8_SNORM;
      case 3:  return reverse ? VK_FORMAT_B8G8R8_SNORM : VK_FORMAT_R8G8B8_SNORM;
      case 4:  return reverse ? VK_FORMAT_B8G8R8A8_SNORM : VK_FORMAT_R8G8B8A8_SNORM;
      default: goto fallback;
      }
   }
   else if (type->GetBase<pci16>(0, base) && base.mStaticBase.mMapping) {
      switch (base.mStaticBase.mCount) {
      case 1:   return VK_FORMAT_R16_SNORM;
      case 2:   return VK_FORMAT_R16G16_SNORM;
      case 3:  return /*reverse ? VK_FORMAT_B16G16R16_SNORM :*/ VK_FORMAT_R16G16B16_SNORM;
      case 4:  return /*reverse ? VK_FORMAT_B16G16R16A16_SNORM :*/ VK_FORMAT_R16G16B16A16_SNORM;
      default: goto fallback;
      }
   }
   else if (type->GetBase<pci32>(0, base) && base.mStaticBase.mMapping) {
      switch (base.mStaticBase.mCount) {
      case 1:   return VK_FORMAT_R32_SINT;
      case 2:   return VK_FORMAT_R32G32_SINT;
      case 3:  return /*reverse ? VK_FORMAT_B32G32R32_SINT :*/ VK_FORMAT_R32G32B32_SINT;
      case 4:  return /*reverse ? VK_FORMAT_B32G32R32A32_SINT :*/ VK_FORMAT_R32G32B32A32_SINT;
      default: goto fallback;
      }
   }
   else if (type->GetBase<pci64>(0, base) && base.mStaticBase.mMapping) {
      switch (base.mStaticBase.mCount) {
      case 1:   return VK_FORMAT_R64_SINT;
      case 2:   return VK_FORMAT_R64G64_SINT;
      case 3:  return /*reverse ? VK_FORMAT_B64G64R64_SINT :*/ VK_FORMAT_R64G64B64_SINT;
      case 4:  return /*reverse ? VK_FORMAT_B64G64R64A64_SINT :*/ VK_FORMAT_R64G64B64A64_SINT;
      default: goto fallback;
      }
   }
   // Real numbers                                                      
   else if (type->GetBase<pcr32>(0, base) && base.mStaticBase.mMapping) {
      switch (base.mStaticBase.mCount) {
      case 1:   return VK_FORMAT_R32_SFLOAT;
      case 2:   return VK_FORMAT_R32G32_SFLOAT;
      case 3:  return /*reverse ? VK_FORMAT_B32G32R32_SFLOAT :*/ VK_FORMAT_R32G32B32_SFLOAT;
      case 4:  return /*reverse ? VK_FORMAT_B32G32R32A32_SFLOAT :*/ VK_FORMAT_R32G32B32A32_SFLOAT;
      default: goto fallback;
      }
   }
   else if (type->GetBase<pcr64>(0, base) && base.mStaticBase.mMapping) {
      switch (base.mStaticBase.mCount) {
      case 1:   return VK_FORMAT_R64_SFLOAT;
      case 2:   return VK_FORMAT_R64G64_SFLOAT;
      case 3:  return /*reverse ? VK_FORMAT_B64G64R64_SFLOAT :*/ VK_FORMAT_R64G64B64_SFLOAT;
      case 4:  return /*reverse ? VK_FORMAT_B64G64R64A64_SFLOAT :*/ VK_FORMAT_R64G64B64A64_SFLOAT;
      default: goto fallback;
      }
   }

fallback:
   throw Except::Graphics(pcLogError
      << "Unsupported element count: " << base.mStaticBase.mCount 
      << " elements of " << base.mBase->GetToken() 
      << " (decayed from " << type->GetToken() << ")");
}

/// Convert a vulkan format to a PC format                                    
///   @param type - the vulkan format type                                    
///   @param reverse - whether or not reversal is required                    
///   @return the piception equivalent                                        
constexpr DataID pcVkFormatToDataID(const VkFormat& type, bool& reverse) {
   reverse = false;
   switch (type) {
      case VK_FORMAT_D16_UNORM:
         return DataID::Of<depth16>;
      case VK_FORMAT_D32_SFLOAT:
         return DataID::Of<depth32>;
      case VK_FORMAT_R8_UNORM:
         return DataID::Of<red8>;
      case VK_FORMAT_R16_UNORM: 
         return DataID::Of<pcu16>;
      case VK_FORMAT_R32_UINT: 
         return DataID::Of<pcu32>;
      case VK_FORMAT_R64_UINT: 
         return DataID::Of<pcu64>;
      case VK_FORMAT_R8_SNORM: 
         return DataID::Of<pci8>;
      case VK_FORMAT_R16_SNORM: 
         return DataID::Of<pci16>;
      case VK_FORMAT_R32_SINT: 
         return DataID::Of<pci32>;
      case VK_FORMAT_R64_SINT: 
         return DataID::Of<pci64>;
      case VK_FORMAT_R32_SFLOAT: 
         return DataID::Of<red32>;
      case VK_FORMAT_R64_SFLOAT: 
         return DataID::Of<pcr64>;
      case VK_FORMAT_R32G32_SFLOAT: 
         return DataID::Of<vec2f>;
      case VK_FORMAT_R64G64_SFLOAT: 
         return DataID::Of<vec2d>;
      case VK_FORMAT_R8G8B8_UNORM: case VK_FORMAT_B8G8R8_UNORM: 
         reverse = true;
         return DataID::Of<rgb>;
      case VK_FORMAT_R32G32B32_SFLOAT:   
         return DataID::Of<rgb96>;
      case VK_FORMAT_R64G64B64_SFLOAT:   
         return DataID::Of<vec3d>;
      case VK_FORMAT_R8G8B8A8_UNORM: case VK_FORMAT_B8G8R8A8_UNORM:
         reverse = true;
         return DataID::Of<rgba>;
      case VK_FORMAT_R32G32B32A32_SFLOAT:   
         return DataID::Of<rgba128>;
      case VK_FORMAT_R64G64B64A64_SFLOAT:   
         return DataID::Of<vec4d>;
      case VK_FORMAT_R8G8_UNORM:
         return DataID::Of<TVec<pcu8, 2>>;
      case VK_FORMAT_R16G16_UNORM:
         return DataID::Of<TVec<pcu16, 2>>;
      case VK_FORMAT_R32G32_UINT:
         return DataID::Of<TVec<pcu32, 2>>;
      case VK_FORMAT_R64G64_UINT:
         return DataID::Of<TVec<pcu64, 2>>;
      case VK_FORMAT_R8G8_SNORM:
         return DataID::Of<TVec<pci8, 2>>;
      case VK_FORMAT_R16G16_SNORM:
         return DataID::Of<TVec<pci16, 2>>;
      case VK_FORMAT_R32G32_SINT:
         return DataID::Of<TVec<pci32, 2>>;
      case VK_FORMAT_R64G64_SINT:
         return DataID::Of<TVec<pci64, 2>>;
      case VK_FORMAT_R16G16B16_UNORM:
         return DataID::Of<TVec<pcu16, 3>>;
      case VK_FORMAT_R32G32B32_UINT:
         return DataID::Of<TVec<pcu32, 3>>;
      case VK_FORMAT_R64G64B64_UINT:
         return DataID::Of<TVec<pcu64, 3>>;
      case VK_FORMAT_R8G8B8_SNORM:
         return DataID::Of<TVec<pci8, 3>>;
      case VK_FORMAT_R16G16B16_SNORM:
         return DataID::Of<TVec<pci16, 3>>;
      case VK_FORMAT_R32G32B32_SINT:
         return DataID::Of<TVec<pci32, 3>>;
      case VK_FORMAT_R64G64B64_SINT:
         return DataID::Of<TVec<pci64, 3>>;
      case VK_FORMAT_R16G16B16A16_UNORM:
         return DataID::Of<TVec<pcu16, 4>>;
      case VK_FORMAT_R32G32B32A32_UINT:
         return DataID::Of<TVec<pcu32, 4>>;
      case VK_FORMAT_R64G64B64A64_UINT:
         return DataID::Of<TVec<pcu64, 4>>;
      case VK_FORMAT_R8G8B8A8_SNORM:
         return DataID::Of<TVec<pci8, 4>>;
      case VK_FORMAT_R16G16B16A16_SNORM:
         return DataID::Of<TVec<pci16, 4>>;
      case VK_FORMAT_R32G32B32A32_SINT:
         return DataID::Of<TVec<pci32, 4>>;
      case VK_FORMAT_R64G64B64A64_SINT:
         return DataID::Of<TVec<pci64, 4>>;
      default: break;
   }
   return {};
}

/// Converter from PC shader to VK shader stage                               
///   @param stage - the shader stage                                         
///   @return the vulkan equivalent                                           
constexpr VkShaderStageFlagBits pcShaderToVkStage(ShaderStage::Enum stage) noexcept {
   constexpr VkShaderStageFlagBits vkstages[ShaderStage::Counter] = {
      VkShaderStageFlagBits::VK_SHADER_STAGE_VERTEX_BIT,
      VkShaderStageFlagBits::VK_SHADER_STAGE_GEOMETRY_BIT,
      VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_CONTROL_BIT,
      VkShaderStageFlagBits::VK_SHADER_STAGE_TESSELLATION_EVALUATION_BIT,
      VkShaderStageFlagBits::VK_SHADER_STAGE_FRAGMENT_BIT,
      VkShaderStageFlagBits::VK_SHADER_STAGE_COMPUTE_BIT
   };

   return vkstages[stage];
}

/// Converter from PC primitive to VK primitive                               
///   @param meta - the primitive type                                        
///   @return the vulkan equivalent                                           
inline VkPrimitiveTopology pcPrimitiveToVkPrimitive(DMeta meta) {
   if (meta->InterpretsAs<APoint>())
      return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
   if (meta->InterpretsAs<ATriangleStrip>())
      return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_STRIP;
   if (meta->InterpretsAs<ATriangleFan>())
      return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_FAN;
   if (meta->InterpretsAs<ATriangle>())
      return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
   if (meta->InterpretsAs<ALineStrip>())
      return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_STRIP;
   if (meta->InterpretsAs<ALine>())
      return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_LINE_LIST;

   pcLogWarning << "pcPrimitiveToVkPrimitive can't decide topology, defaulting to a point list...";
   return VkPrimitiveTopology::VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
}