///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "VulkanRenderable.hpp"
#include "VulkanRenderer.hpp"

#define PC_VERBOSE_RENDERABLE(a) a


/// Descriptor constructor                                                    
///   @param descriptor - the renderable descriptor                           
VulkanRenderable::VulkanRenderable(const Any& descriptor)
   : GraphicsUnit {MetaOf<VulkanRenderable>(), descriptor} {
   TODO();
}

/// Get the renderer for the renderable                                       
///   @return a pointer to the renderer                                       
auto VulkanRenderable::GetRenderer() noexcept {
   return GetProducer()->GetProducer();
}

/// Get VRAM geometry corresponding to an octave of this renderable           
/// This is the point where content might be generated upon request           
///   @param lod - information used to extract the best LOD                   
///   @return the VRAM geometry or nullptr if content is not available        
VulkanGeometry* VulkanRenderable::GetGeometry(const LodState& lod) {
   const auto i = lod.GetAbsoluteIndex();
   if (!mLOD[i].mGeometry && mGeometryContent)
      mLOD[i].mGeometry = GetRenderer()->Cache(mGeometryContent->GetLOD(lod));
   return mLOD[i].mGeometry;
}

/// Get VRAM texture corresponding to an octave of this renderable            
/// This is the point where content might be generated upon request           
///   @param lod - information used to extract the best LOD                   
///   @return the VRAM texture or nullptr if content is not available         
VulkanTexture* VulkanRenderable::GetTextures(const LodState& lod) {
   const auto i = lod.GetAbsoluteIndex();
   if (!mLOD[i].mTexture && mTextureContent)
      mLOD[i].mTexture = GetRenderer()->Cache(mTextureContent.Get()); //TODO texture lod?
   return mLOD[i].mTexture;
}

/// Create GPU pipeline able to utilize geometry, textures and shaders        
///   @param lod - information used to extract the best LOD                   
///   @param layer - additional settings might be provided by the used layer  
///   @return the pipeline                                                    
VulkanPipeline* VulkanRenderable::GetOrCreatePipeline(const LodState& lod, const VulkanLayer* layer) {
   // Always return the predefined pipeline if available                
   if (mPredefinedPipeline)
      return mPredefinedPipeline;

   // Always return the cached pipeline if available                    
   const auto i = lod.GetAbsoluteIndex();
   if (mLOD[i].mPipeline)
      return mLOD[i].mPipeline;

   // Construct a pipeline                                              
   bool usingGlobalPipeline = false;
   auto construct = Construct::From<VulkanPipeline>();
   if (mMaterialContent) {
      construct << mMaterialContent;
      usingGlobalPipeline = true;
   }
   else {
      if (mGeometryContent)
         construct << mGeometryContent->GetLOD(lod);
      if (mTextureContent)
         construct << mTextureContent;
   }

   // Add shaders if any such trait exists in unit environment          
   auto shader = SeekTrait<Traits::Shader>();
   if (!shader.IsEmpty())
      construct << shader;

   // An alternative way to set a shader snippet via GLSL code trait    
   shader = SeekTrait<Traits::Code>();
   if (!shader.IsEmpty() && shader.Is<GLSL>())
      construct << shader;

   // Add colorization if available                                     
   auto color = SeekTrait<Traits::Color>();
   if (!color.IsEmpty())
      construct << color;

   // If at this point the construct is empty, then nothing to draw     
   if (construct.IsEmpty()) {
      Logger::Warning(Self(), "No contents available for generating pipeline");
      return nullptr;
   }
   
   // Add the layer, too, if available                                  
   // This is intentionally added after the above check                 
   if (layer)
      construct << layer;

   // Get, or create the pipeline                                       
   Verbs::Create creator {&construct};
   GetRenderer()->Create(creator);
   creator.GetOutput().ForEachDeep([&](VulkanPipeline* p) {
      if (usingGlobalPipeline)
         mPredefinedPipeline = p;
      else
         mLOD[i].mPipeline = p;
   });

   if (mPredefinedPipeline)
      return mPredefinedPipeline;
   else
      return mLOD[i].mPipeline;
}

/// Reset the renderable, releasing all used content and pipelines            
void VulkanRenderable::ResetRenderable() {
   for (auto& lod : mLOD) {
      lod.mGeometry.Reset();
      lod.mTexture.Reset();
      lod.mPipeline.Reset();
   }

   mMaterialContent.Reset();
   mGeometryContent.Reset();
   mTextureContent.Reset();
   mInstances.Reset();
   mPredefinedPipeline.Reset();
}

/// Called when owner changes components/traits                               
void VulkanRenderable::Refresh() {
   // Just reset - new resources will be regenerated or reused upon     
   // request if they need be                                           
   ResetRenderable();

   // Gather all instances for this renderable, and calculate levels    
   mInstances = GetOwners().GatherUnits<SeekStyle::Here>("Instance");
   if (!mInstances.IsEmpty())
      mLevelRange = mInstances[0]->GetLevel();
   else
      mLevelRange = {};

   for (auto instance : mInstances)
      mLevelRange.Embrace(instance->GetLevel());

   // Attempt extracting pipeline/material/geometry/textures from owners
   for (auto owner : GetOwners()) {
      for (auto unit : owner->GetUnits()) {
         // First check for predefined pipeline                         
         auto pipeline = dynamic_cast<VulkanPipeline*>(unit);
         if (pipeline) {
            mPredefinedPipeline = pipeline;
            return;
         }

         // Then check for predefined material                          
         auto material = dynamic_cast<AMaterial*>(unit);
         if (material) {
            mMaterialContent = material;
            return;
         }

         // If no material or pipe, check for geometries and textures   
         auto geometry = dynamic_cast<AGeometry*>(unit);
         if (geometry)
            mGeometryContent = geometry;

         auto texture = dynamic_cast<ATexture*>(unit);
         if (texture)
            mTextureContent = texture;
      }
   }
}
