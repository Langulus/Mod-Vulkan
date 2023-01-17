///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "VulkanGeometry.hpp"
#include "../VulkanRenderer.hpp"

#define VERBOSE_VKGEOMETRY(a) //pcLogSelfVerbose << a

/// VRAM geometry construction                                                
///   @param producer - the owner of the content                              
VulkanGeometry::VulkanGeometry(VulkanRenderer* producer)
   : IContentVRAM {MetaData::Of<CVulkanGeometry>()}
   , TProducedFrom {producer} {
   ClassValidate();
}

/// VRAM destruction                                                          
VulkanGeometry::~VulkanGeometry() {
   Uninitialize();
}

/// Create the VRAM texture from a verb                                       
///   @param verb - creation verb                                             
void VulkanGeometry::Create(Verb& verb) {
   verb.GetArgument().ForEachDeep([&](const AGeometry* content) {
      mOriginalContent = content;
      verb.Done();
   });
}

/// Returns true if container has any vertex data                             
///   @param container - container to scan                                    
///   @param index_data - [out] whether or not index data is available        
///   @return true if container has vectors                                   
inline bool IsVertexData(const Trait& container, bool& index_data) {
   if (  container.InterpretsAs<ATopology>() || container.InterpretsAs<ANormal>()
      || container.InterpretsAs<ASampler>()  || container.InterpretsAs<AColor>()) {
      index_data = false;
      return true;
   }
   
   if (container.InterpretsAs<AInteger>(1)) {
      index_data = true;
      return true;
   }

   return false;
}

/// Initialize the geometry from geometry generator                           
void VulkanGeometry::Initialize() {
   if (mContentMirrored)
      return;

   PCTimer loadTime;
   loadTime.Start();

   // Generate content if not generated yet                             
   const auto content = mOriginalContent.As<AGeometry>();
   if (!content) {
      throw Except::Graphics(pcLogSelfError
         << "Bad content: " << mOriginalContent);
   }

   // Generate content if not generated yet                             
   content->Generate();

   // Create a buffer for each provided type, as long as that type can  
   // be reduced to vertices                                            
   auto& vram = mProducer->GetVRAM();
   for (pcptr i = 0; i < content->GetDataList().GetCount(); ++i) {
      const auto& group = content->GetDataList().Get<Trait>(i);
      bool is_index_data = false;
      if (!IsVertexData(group, is_index_data)) {
         pcLogSelfWarning << "Raw content of type " << group.GetToken()
            << " is not considered vertex data and will not be cloned to VRAM";
         continue;
      }

      // Clone the data to VRAM                                         
      VERBOSE_VKGEOMETRY("Uploading " << group.GetCount()
         << " of " << group.GetToken() << " to VRAM...");

      auto final = vram.Upload(group, is_index_data 
         ? VkBufferUsageFlagBits::VK_BUFFER_USAGE_INDEX_BUFFER_BIT 
         : VkBufferUsageFlagBits::VK_BUFFER_USAGE_VERTEX_BUFFER_BIT
      );

      // If everything is in order, we save the buffers                 
      if (final.IsValid()) {
         if (is_index_data) {
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
      throw Except::Graphics(pcLogSelfError
         << "Couldn't upload geometry to VRAM: " << content);
   }

   // Make sure mView.mPCount means vertex count, and not               
   // primitive count. Decay in order to do that.                       
   mTopology = content->GetView().mPrimitive;
   mView = content->GetView().Decay();
   pcLogSelfVerbose << ccGreen << "Data uploaded in VRAM for " << loadTime.Stop();
   mContentMirrored = true;
}

/// Free up VRAM                                                              
void VulkanGeometry::Uninitialize() {
   // Destroy allocated buffers                                         
   auto& vram = mProducer->GetVRAM();
   for (auto& item : mVBuffers)
      vram.DestroyBuffer(item);
   for (auto& item : mIBuffers)
      vram.DestroyBuffer(item);

   // Clear the containers                                              
   mVBuffers.clear();
   mIBuffers.clear();
   mVOffsets.clear();
   mIOffsets.clear();
   ClassInvalidate();

   mContentMirrored = false;
}

/// Bind the vertex & index buffers                                           
void VulkanGeometry::Bind() const {
   // Bind each vertex buffers                                          
   auto& cmdbuffer = mProducer->GetRenderCB();
   for (pcptr i = 0; i < mVBuffers.size(); ++i)
      vkCmdBindVertexBuffers(cmdbuffer, uint32_t(i), 1, &mVBuffers[i].mBuffer.mValue, &mVOffsets[i]);

   for (pcptr i = 0; i < mIBuffers.size(); ++i) {
      vkCmdBindIndexBuffer(cmdbuffer, mIBuffers[i].mBuffer, mIOffsets[i], pcTypeToVkIndexType(mIBuffers[i].mMeta));
      break;
   }
}

/// Render the vertex & index buffers                                         
void VulkanGeometry::Render() const {
   auto& cmdbuffer = mProducer->GetRenderCB();
   if (mIBuffers.empty())
      vkCmdDraw(cmdbuffer, uint32_t(mView.mPCount), 1, uint32_t(mView.mPStart), 0);
   else 
      vkCmdDrawIndexed(cmdbuffer, uint32_t(mView.mICount), 1, uint32_t(mView.mIStart), int32_t(mView.mPStart), 0);
}