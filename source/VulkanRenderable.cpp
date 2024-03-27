///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright (c) 2020 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Vulkan.hpp"
#include <Langulus/Physical.hpp>


/// Descriptor constructor                                                    
///   @param producer - the renderable producer                               
///   @param descriptor - the renderable descriptor                           
VulkanRenderable::VulkanRenderable(VulkanLayer* producer, const Neat& descriptor)
   : Resolvable {this} 
   , ProducedFrom {producer, descriptor} {
   VERBOSE_VULKAN("Initializing...");
   Couple(descriptor);
   VERBOSE_VULKAN("Initialized");
}

VulkanRenderable::~VulkanRenderable() {
   Detach();
}

/// Reset the renderable, releasing all used content and pipelines            
void VulkanRenderable::Detach() {
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
   ProducedFrom<VulkanLayer>::Detach();
}

/// Get the renderer                                                          
///   @return a pointer to the renderer                                       
VulkanRenderer* VulkanRenderable::GetRenderer() const noexcept {
   return GetProducer()->GetProducer();
}

/// Get VRAM geometry corresponding to an octave of this renderable           
/// This is the point where content might be generated upon request           
///   @param lod - information used to extract the best LOD                   
///   @return the VRAM geometry or nullptr if content is not available        
VulkanGeometry* VulkanRenderable::GetGeometry(const LOD& lod) const {
   const auto i = lod.GetAbsoluteIndex();
   if (not mLOD[i].mGeometry and mGeometryContent) {
      // Cache geometry to VRAM                                         
      Verbs::Create creator {
         Construct::From<VulkanGeometry>(mGeometryContent->GetLOD(lod))
      };
      mProducer->Create(creator);
      mLOD[i].mGeometry = creator->template As<VulkanGeometry*>();
   }

   return mLOD[i].mGeometry;
}

/// Get VRAM texture corresponding to an octave of this renderable            
/// This is the point where content might be generated upon request           
///   @param lod - information used to extract the best LOD                   
///   @return the VRAM texture or nullptr if content is not available         
VulkanTexture* VulkanRenderable::GetTexture(const LOD& lod) const {
   const auto i = lod.GetAbsoluteIndex();
   if (not mLOD[i].mTexture and mTextureContent) {
      // Cache texture to VRAM                                          
      Verbs::Create creator {
         Construct::From<VulkanTexture>(mTextureContent->GetLOD(lod))
      };
      mProducer->Create(creator);
      mLOD[i].mTexture = creator->template As<VulkanTexture*>();
   }

   return mLOD[i].mTexture;
}

/// Create GPU pipeline able to utilize geometry, textures and shaders        
///   @param lod - information used to extract the best LOD                   
///   @param layer - additional settings might be provided by the used layer  
///   @return the pipeline                                                    
VulkanPipeline* VulkanRenderable::GetOrCreatePipeline(
   const LOD& lod, const VulkanLayer* layer
) const {
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
   if (shader)
      construct << shader;

   // Add colorization if available                                     
   auto color = SeekTrait<Traits::Color>();
   if (color)
      construct << color;

   // If at this point the construct is empty, then nothing to draw     
   if (not construct.GetDescriptor()) {
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
   creator->ForEachDeep([&](VulkanPipeline& p) {
      if (usingGlobalPipeline)
         mPredefinedPipeline = &p;
      else
         mLOD[i].mPipeline = &p;
   });

   if (mPredefinedPipeline)
      return mPredefinedPipeline;
   else
      return mLOD[i].mPipeline;
}

/// Called when owner changes components/traits                               
void VulkanRenderable::Refresh() {
   // Just reset - new resources will be regenerated or reused upon     
   // request if they need be                                           
   Detach();

   // Gather all instances for this renderable, and calculate levels    
   mInstances = GatherUnits<A::Instance, Seek::Here>();
   if (mInstances)
      mLevelRange = mInstances[0]->GetLevel();
   else
      mLevelRange = {};

   for (auto instance : mInstances)
      mLevelRange.Embrace(instance->GetLevel());

   // Attempt extracting pipeline/material/geometry/textures from owners
   const auto pipeline = SeekUnit<VulkanPipeline, Seek::Here>();
   if (pipeline) {
      mPredefinedPipeline = pipeline;
      return;
   }

   const auto material = SeekUnit<A::Material, Seek::Here>();
   if (material) {
      mMaterialContent = material;
      return;
   }

   const auto geometry = SeekUnit<A::Mesh, Seek::Here>();
   if (geometry)
      mGeometryContent = geometry;

   const auto texture = SeekUnit<A::Image, Seek::Here>();
   if (texture)
      mTextureContent = texture;
}
