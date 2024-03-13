///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "Common.hpp"
#include <Math/Primitives.hpp>


/// Convert a meta data to a VK index format                                  
///   @param meta - the type definition to convert                            
///   @return the vulkan format equivalent                                    
LANGULUS(INLINED)
VkIndexType AsVkIndexType(DMeta meta) {
   if (meta->Is<uint32_t>())
      return VK_INDEX_TYPE_UINT32;
   else if (meta->Is<uint16_t>())
      return VK_INDEX_TYPE_UINT16;
   else if (meta->Is<uint8_t>())
      return VK_INDEX_TYPE_UINT8_EXT;

   LANGULUS_OOPS(Graphics, "Unsupported index type");
   return VK_INDEX_TYPE_MAX_ENUM;
}

/// Convert meta data to a VK color/depth format                              
///   @param type - the type definition to convert                            
///   @param reverse - whether to invert color definition                     
///   @return the vulkan format equivalent                                    
LANGULUS(INLINED)
VkFormat AsVkFormat(DMeta type, bool reverse) {
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

   LANGULUS_OOPS(Graphics, "Unsupported format");
   return VK_FORMAT_UNDEFINED;
}

/// Convert vulkan format to meta data                                        
///   @param type - the vulkan format type                                    
///   @param reverse - whether or not reversal is required                    
///   @return the piception equivalent                                        
LANGULUS(INLINED)
DMeta VkFormatToDMeta(const VkFormat& type, bool& reverse) {
   reverse = false;
   switch (type) {
      case VK_FORMAT_D16_UNORM:
         return MetaOf<Depth16>();
      case VK_FORMAT_D32_SFLOAT:
         return MetaOf<Depth32>();
      case VK_FORMAT_R8_UNORM:
         return MetaOf<Red8>();
      case VK_FORMAT_R16_UNORM: 
         return MetaOf<uint16_t>();
      case VK_FORMAT_R32_UINT: 
         return MetaOf<uint32_t>();
      case VK_FORMAT_R64_UINT: 
         return MetaOf<uint64_t>();
      case VK_FORMAT_R8_SNORM: 
         return MetaOf<int8_t>();
      case VK_FORMAT_R16_SNORM: 
         return MetaOf<int16_t>();
      case VK_FORMAT_R32_SINT: 
         return MetaOf<int32_t>();
      case VK_FORMAT_R64_SINT: 
         return MetaOf<int64_t>();
      case VK_FORMAT_R32_SFLOAT: 
         return MetaOf<Red32>();
      case VK_FORMAT_R64_SFLOAT: 
         return MetaOf<Double>();
      case VK_FORMAT_R32G32_SFLOAT: 
         return MetaOf<Vec2f>();
      case VK_FORMAT_R64G64_SFLOAT: 
         return MetaOf<Vec2d>();
      case VK_FORMAT_R8G8B8_UNORM: case VK_FORMAT_B8G8R8_UNORM: 
         reverse = true;
         return MetaOf<RGB>();
      case VK_FORMAT_R32G32B32_SFLOAT:   
         return MetaOf<RGB96>();
      case VK_FORMAT_R64G64B64_SFLOAT:   
         return MetaOf<Vec3d>();
      case VK_FORMAT_R8G8B8A8_UNORM: case VK_FORMAT_B8G8R8A8_UNORM:
         reverse = true;
         return MetaOf<RGBA>();
      case VK_FORMAT_R32G32B32A32_SFLOAT:   
         return MetaOf<RGBA128>();
      case VK_FORMAT_R64G64B64A64_SFLOAT:   
         return MetaOf<Vec4d>();
      case VK_FORMAT_R8G8_UNORM:
         return MetaOf<Vec2u8>();
      case VK_FORMAT_R16G16_UNORM:
         return MetaOf<Vec2u16>();
      case VK_FORMAT_R32G32_UINT:
         return MetaOf<Vec2u32>();
      case VK_FORMAT_R64G64_UINT:
         return MetaOf<Vec2u64>();
      case VK_FORMAT_R8G8_SNORM:
         return MetaOf<Vec2i8>();
      case VK_FORMAT_R16G16_SNORM:
         return MetaOf<Vec2i16>();
      case VK_FORMAT_R32G32_SINT:
         return MetaOf<Vec2i32>();
      case VK_FORMAT_R64G64_SINT:
         return MetaOf<Vec2i64>();
      case VK_FORMAT_R16G16B16_UNORM:
         return MetaOf<Vec2u16>();
      case VK_FORMAT_R32G32B32_UINT:
         return MetaOf<Vec2u32>();
      case VK_FORMAT_R64G64B64_UINT:
         return MetaOf<Vec2u64>();
      case VK_FORMAT_R8G8B8_SNORM:
         return MetaOf<Vec3i8>();
      case VK_FORMAT_R16G16B16_SNORM:
         return MetaOf<Vec3i16>();
      case VK_FORMAT_R32G32B32_SINT:
         return MetaOf<Vec3i32>();
      case VK_FORMAT_R64G64B64_SINT:
         return MetaOf<Vec3i64>();
      case VK_FORMAT_R16G16B16A16_UNORM:
         return MetaOf<Vec4u16>();
      case VK_FORMAT_R32G32B32A32_UINT:
         return MetaOf<Vec4u32>();
      case VK_FORMAT_R64G64B64A64_UINT:
         return MetaOf<Vec4u64>();
      case VK_FORMAT_R8G8B8A8_SNORM:
         return MetaOf<Vec4i8>();
      case VK_FORMAT_R16G16B16A16_SNORM:
         return MetaOf<Vec4i16>();
      case VK_FORMAT_R32G32B32A32_SINT:
         return MetaOf<Vec4i32>();
      case VK_FORMAT_R64G64B64A64_SINT:
         return MetaOf<Vec4i64>();
      default:
         break;
   }

   LANGULUS_OOPS(Graphics, "Unsupported format");
   return {};
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
LANGULUS(INLINED)
VkPrimitiveTopology AsVkPrimitive(DMeta meta) {
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

   LANGULUS_OOPS(Graphics, "Unsupported topology");
   return VK_PRIMITIVE_TOPOLOGY_MAX_ENUM;
}

/// Helper function for converting colors to floats                           
/// If data is made of integers, colors will be normalized in the 255 range   
///   @param color - the color to convert to RGBAf                            
///   @return a floating point RGBA vector                                    
LANGULUS(INLINED)
RGBAf AnyColorToVector(const Any& color) {
   const auto ctype = color.GetType();
   LANGULUS_ASSERT(ctype, Meta, "No color type");
   const auto rmember = ctype->GetMember(MetaOf<Traits::R>());
   const auto gmember = ctype->GetMember(MetaOf<Traits::G>());
   const auto bmember = ctype->GetMember(MetaOf<Traits::B>());
   const auto amember = ctype->GetMember(MetaOf<Traits::A>());

   // Inspect the data pack, what color components does it contain?     
   const auto rChannel = color.GetMember(*rmember, 0);
   const auto gChannel = color.GetMember(*gmember, 0);
   const auto bChannel = color.GetMember(*bmember, 0);
   const auto aChannel = color.GetMember(*amember, 0);

   // Get the values (and normalize them if we have to)                 
   RGBAf result;
   if (rChannel) {
      result[0] = rChannel.AsCast<Real>();
      if (rChannel.CastsTo<A::Integer>())
         result[0] /= 255.0f;
   }
   
   if (gChannel) {
      result[1] = gChannel.AsCast<Real>();
      if (gChannel.CastsTo<A::Integer>())
         result[1] /= 255.0f;
   }

   if (bChannel) {
      result[2] = bChannel.AsCast<Real>();
      if (bChannel.CastsTo<A::Integer>())
         result[2] /= 255.0f;
   }

   if (aChannel) {
      result[3] = aChannel.AsCast<Real>();
      if (aChannel.CastsTo<A::Integer>())
         result[3] /= 255.0f;
   }

   return result;
}
