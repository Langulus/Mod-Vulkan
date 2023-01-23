///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "Common.hpp"

constexpr BlendMode::BlendMode(Type value) noexcept
   : mMode {value} {}

constexpr MapMode::MapMode(Type value) noexcept
   : mMode {value} {}

constexpr RefreshRate::RefreshRate(const CT::DenseNumber auto& value) noexcept
   : mMode {value} {}

constexpr RefreshRate::RefreshRate(const Enum& value) noexcept
   : mMode {value} {}

LANGULUS(ALWAYSINLINE)
constexpr bool RefreshRate::IsUniform() const noexcept {
   return mMode >= UniformBegin && mMode < UniformEnd;
}

LANGULUS(ALWAYSINLINE)
constexpr bool RefreshRate::IsStaticUniform() const noexcept {
   return mMode >= StaticUniformBegin && mMode < StaticUniformEnd;
}

LANGULUS(ALWAYSINLINE)
constexpr bool RefreshRate::IsDynamicUniform() const noexcept {
   return mMode >= DynamicUniformBegin && mMode < DynamicUniformEnd;
}

LANGULUS(ALWAYSINLINE)
constexpr bool RefreshRate::IsAttribute() const noexcept {
   return mMode == Enum::Vertex;
}

LANGULUS(ALWAYSINLINE)
constexpr bool RefreshRate::IsInput() const noexcept {
   return mMode >= InputBegin && mMode < InputEnd;
}

LANGULUS(ALWAYSINLINE)
constexpr bool RefreshRate::IsShaderStage() const noexcept {
   return mMode >= StagesBegin && mMode < StagesEnd;
}

LANGULUS(ALWAYSINLINE)
constexpr Offset RefreshRate::GetInputIndex() const {
   if (!IsInput())
      LANGULUS_THROW(Graphics, "Not an input");
   return mMode - InputBegin;
}

LANGULUS(ALWAYSINLINE)
constexpr Offset RefreshRate::GetStaticUniformIndex() const {
   if (!IsStaticUniform())
      LANGULUS_THROW(Graphics, "Not a static uniform");
   return mMode - StaticUniformBegin;
}

LANGULUS(ALWAYSINLINE)
constexpr Offset RefreshRate::GetDynamicUniformIndex() const {
   if (!IsDynamicUniform())
      LANGULUS_THROW(Graphics, "Not a dynamic uniform");
   return mMode - DynamicUniformBegin;
}

LANGULUS(ALWAYSINLINE)
constexpr ShaderStage::Enum RefreshRate::GetStageIndex() const {
   if (!IsShaderStage())
      LANGULUS_THROW(Graphics, "Not a shader stage");
   return static_cast<ShaderStage::Enum>(mMode - StagesBegin);
}

/// Compare geometry views                                                    
///   @param rhs - the view to compare against                                
///   @return true if both views are the same                                 
LANGULUS(ALWAYSINLINE)
bool GeometryView::operator == (const GeometryView& rhs) const noexcept {
   return mPrimitiveCount == rhs.mPrimitiveCount
      && mPrimitiveStart == rhs.mPrimitiveStart
      && mIndexCount == rhs.mIndexCount
      && mIndexStart == rhs.mIndexStart
      && mBilateral == rhs.mBilateral
      && mPrimitiveType->Is(rhs.mPrimitiveType);
}

/// Decay the geometry view to a simpler topology                             
///   @return the generated view                                              
LANGULUS(ALWAYSINLINE)
GeometryView GeometryView::Decay() const {
   TODO();
   return {};
}

/// Compare texture views                                                     
///   @param rhs - the view to compare against                                
///   @return true if both views are the same                                 
LANGULUS(ALWAYSINLINE)
bool TextureView::operator == (const TextureView& rhs) const noexcept {
   return mWidth == rhs.mWidth
      && mHeight == rhs.mHeight
      && mDepth == rhs.mDepth
      && mFrames == rhs.mFrames
      && mReverseFormat == rhs.mReverseFormat
      && mFormat->Is(rhs.mFormat);
}

/// Get the number of pixels across all dimensions                            
///   @return the pixel count                                                 
LANGULUS(ALWAYSINLINE)
constexpr uint32_t TextureView::GetPixelCount() const noexcept {
   return mWidth * mHeight * mDepth * mFrames;
}

/// Get the number of dimensions                                              
///   @return the number of used dimensions                                   
LANGULUS(ALWAYSINLINE)
constexpr uint32_t TextureView::GetDimensionCount() const noexcept {
   return  Count {mWidth > 1}
         + Count {mHeight > 1}
         + Count {mDepth > 1}
         + Count {mFrames > 1};
}

/// Get the bytesize of a single pixel                                        
///   @return the size of a single pixel in bytes                             
LANGULUS(ALWAYSINLINE)
Size TextureView::GetPixelBytesize() const noexcept {
   return mFormat->mSize;
}

/// Get the bytesize of the entire texture across all dimensions              
///   @return the bytesize of the entire texture                              
LANGULUS(ALWAYSINLINE)
Size TextureView::GetBytesize() const noexcept {
   return GetPixelBytesize() * GetPixelCount();
}

/// Return the number of channels inside the texture format                   
///   @return the number of channels                                          
LANGULUS(ALWAYSINLINE)
uint32_t TextureView::GetChannelCount() const noexcept {
   return mFormat->GetMemberCount();
}

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
LANGULUS(ALWAYSINLINE)
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

   LANGULUS_THROW(Graphics, "Unsupported format");
}

/// Convert vulkan format to meta data                                        
///   @param type - the vulkan format type                                    
///   @param reverse - whether or not reversal is required                    
///   @return the piception equivalent                                        
LANGULUS(ALWAYSINLINE)
DMeta VkFormatToDMeta(const VkFormat& type, bool& reverse) {
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
LANGULUS(ALWAYSINLINE)
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

   LANGULUS_THROW(Graphics, "Unsupported topology");
}

/// Recalculate LOD index for identity model matrix                           
LANGULUS(ALWAYSINLINE)
void LodState::Transform() {
   mModel = {};
   mModelView = mViewInverted;
   mOrigin = {};
   mRadius = {};
   mDistanceToSurface = {};
   mLODIndex = 0;
}

/// Recalculate LOD index by specifying the model matrix                      
///   @param model - the model transformation                                 
LANGULUS(ALWAYSINLINE)
void LodState::Transform(const Matrix4& model) {
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
      mLODIndex = Math::Clamp(near - far, Real {MinIndex}, Real {MaxIndex});
   }
   else if (mDistanceToSurface < 0) {
      // Being inside the sphere always gives the most detail           
      mLODIndex = Real {MaxIndex};
   }
}

/// Get the distance from the camera to the model's bounding sphere           
/// surface, divided by the bounding sphere size                              
///   @return the distance                                                    
LANGULUS(ALWAYSINLINE)
Real LodState::GetNormalizedDistance() const noexcept {
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
LANGULUS(ALWAYSINLINE)
Real LodState::GetIndex() const noexcept {
   return mLODIndex;
}

/// Get LOD index in the range [0;IndexCount)                                 
///   @return the absolute index                                              
LANGULUS(ALWAYSINLINE)
AbsoluteLodIndex LodState::GetAbsoluteIndex() const noexcept {
   return AbsoluteLodIndex(LodIndex(mLODIndex) - MinIndex);
}

/// VRAM content hash is the same as the original content                     
LANGULUS(ALWAYSINLINE)
Hash ContentVRAM::GetHash() const {
   return mOriginalContent.GetHash();
}

/// Compare VRAM by comparing original contents                               
LANGULUS(ALWAYSINLINE)
bool ContentVRAM::operator == (const ContentVRAM& other) const noexcept {
   return mOriginalContent == other.mOriginalContent;
}


/// Helper function for converting colors to floats                           
/// If data is made of integers, colors will be normalized in the 255 range   
///   @param color - the color to convert to RGBAf                            
///   @return a floating point RGBA vector                                    
LANGULUS(ALWAYSINLINE)
RGBAf AnyColorToVector(const Any& color) {
   // Inspect the data pack, what color components does it contain?     
   const auto redChannel = color.GetMember<Traits::R>();
   const auto greenChannel = color.GetMember<Traits::G>();
   const auto blueChannel = color.GetMember<Traits::B>();
   const auto alphaChannel = color.GetMember<Traits::A>();

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