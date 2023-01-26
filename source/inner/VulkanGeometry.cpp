///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "../Vulkan.hpp"

#define VERBOSE_VKGEOMETRY(...) //Logger::Verbose(Self(), __VA_ARGS__)


/// Descriptor constructor                                                    
///   @param producer - the producer of the unit                              
///   @param descriptor - the unit descriptor                                 
VulkanGeometry::VulkanGeometry(VulkanRenderer* producer, const Any& descriptor)
   : ContentVRAM {MetaOf<VulkanGeometry>(), descriptor}
   , ProducedFrom {producer, descriptor} {
   TODO();
}

/// VRAM destruction                                                          
VulkanGeometry::~VulkanGeometry() {
   Uninitialize();
}

/// Create the VRAM texture from a verb                                       
///   @param verb - creation verb                                             
void VulkanGeometry::Create(Verb& verb) {
   verb.ForEachDeep([&](const A::Geometry* content) {
      mOriginalContent = content;
      verb.Done();
   });
}

/// Returns true if container has any vertex data                             
///   @param container - container to scan                                    
///   @param indexData - [out] whether or not index data is available         
///   @return true if container has vectors                                   
inline bool IsVertexData(const Block& container, bool& indexData) {
   if (  container.CastsTo<A::Topology>() || container.CastsTo<A::Normal>()
      || container.CastsTo<A::Sampler>()  || container.CastsTo<A::Color>()) {
      indexData = false;
      return true;
   }
   
   if (container.CastsTo<A::Integer>(1)) {
      indexData = true;
      return true;
   }

   return false;
}

/// Initialize the geometry from geometry generator                           
void VulkanGeometry::Initialize() {
   if (mContentMirrored)
      return;

   // Generate content if not generated yet                             
   const auto startTime = SteadyClock::now();
   const auto geometry = mOriginalContent.As<A::Geometry>();
   if (!geometry)
      LANGULUS_THROW(Graphics, "Bad content");

   // Create a buffer for each provided type, as long as that type can  
   // be reduced to vertices                                            
   auto& vram = mProducer->mVRAM;
   for (auto data : geometry->GetData()) {
      const auto& group = data.mValue;
      bool isIndexData = false;
      if (!IsVertexData(group, isIndexData)) {
         Logger::Warning(Self(),
            "Raw content of type ", group.GetToken(),
            " is not considered vertex data and will not be cloned to VRAM"
         );
         continue;
      }

      // Clone the data to VRAM                                         
      VERBOSE_VKGEOMETRY("Uploading " << group.GetCount()
         << " of " << group.GetToken() << " to VRAM...");

      auto final = vram.Upload(group, isIndexData 
         ? VK_BUFFER_USAGE_INDEX_BUFFER_BIT 
         : VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
      );

      // If everything is in order, we save the buffers                 
      if (final.IsValid()) {
         if (isIndexData) {
            mIBuffers.push_back(final);
            mIOffsets.push_back(0);
         }
         else {
            mVBuffers.push_back(final);
            mVOffsets.push_back(0);
         }

         VERBOSE_VKGEOMETRY("Uploaded " << group.GetCount()
            << " of " << group.GetToken() << " to VRAM");
      }
   }

   if (mVBuffers.empty()) {
      Uninitialize();
      LANGULUS_THROW(Graphics, "Couldn't upload geometry to VRAM");
   }

   // Make sure mView.mPCount means vertex count, and not               
   // primitive count. Decay in order to do that.                       
   mTopology = geometry->GetView().mPrimitive;
   mView = geometry->GetView().Decay();
   VERBOSE_VKGEOMETRY(Logger::Green, "Data uploaded in VRAM for ",
      SteadyClock::now() - startTime);
   mContentMirrored = true;
}

/// Free up VRAM                                                              
void VulkanGeometry::Uninitialize() {
   // Destroy allocated buffers                                         
   auto& vram = mProducer->mVRAM;
   for (auto& item : mVBuffers)
      vram.DestroyBuffer(item);
   for (auto& item : mIBuffers)
      vram.DestroyBuffer(item);

   // Clear the containers                                              
   mVBuffers.clear();
   mIBuffers.clear();
   mVOffsets.clear();
   mIOffsets.clear();

   mContentMirrored = false;
}

/// Bind the vertex & index buffers                                           
void VulkanGeometry::Bind() const {
   // Bind each vertex buffers                                          
   auto& cmdbuffer = mProducer->GetRenderCB();
   for (size_t i = 0; i < mVBuffers.size(); ++i)
      vkCmdBindVertexBuffers(cmdbuffer, uint32_t(i), 1, &mVBuffers[i].mBuffer.mValue, &mVOffsets[i]);

   for (size_t i = 0; i < mIBuffers.size(); ++i) {
      vkCmdBindIndexBuffer(cmdbuffer, mIBuffers[i].mBuffer, mIOffsets[i], AsVkIndexType(mIBuffers[i].mMeta));
      break;
   }
}

/// Render the vertex & index buffers                                         
void VulkanGeometry::Render() const {
   auto& cmdbuffer = mProducer->GetRenderCB();
   if (mIBuffers.empty()) {
      // Draw unindexed                                                 
      vkCmdDraw(
         cmdbuffer, 
         mView.mPrimitiveCount,  // Vertex count                        
         1,                      // Instance count                      
         mView.mPrimitiveStart,  // First vertex                        
         0                       // First instance                      
      );
   }
   else {
      // Draw indexed                                                   
      vkCmdDrawIndexed(
         cmdbuffer, 
         mView.mIndexCount,      // Index count                         
         1,                      // Instance count                      
         mView.mIndexStart,      // First index                         
         static_cast<int32_t>(mView.mPrimitiveStart), // Vertex offset (can be negative?)
         0                       // First instance                      
      );
   }
}