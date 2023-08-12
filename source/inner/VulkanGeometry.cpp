///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "../Vulkan.hpp"

#define VERBOSE_VKGEOMETRY(...) //Logger::Verbose(Self(), __VA_ARGS__)


/// Returns true if container has any vertex data                             
///   @param container - container to scan                                    
///   @param indexData - [out] whether or not index data is available         
///   @return true if container has vectors                                   
inline bool IsVertexData(const Block& container, bool& indexData) {
   if (  container.CastsTo<A::Topology>()
      || container.CastsTo<A::Normal>()
      || container.CastsTo<A::Sampler>()
      || container.CastsTo<A::Color>()) {
      indexData = false;
      return true;
   }

   if (container.CastsTo<A::Integer>(1)) {
      indexData = true;
      return true;
   }

   return false;
}

/// Descriptor constructor                                                    
///   @param producer - the producer of the unit                              
///   @param descriptor - the unit descriptor                                 
VulkanGeometry::VulkanGeometry(VulkanRenderer* producer, const Any& descriptor)
   : A::Graphics {MetaOf<VulkanGeometry>(), descriptor}
   , ProducedFrom {producer, descriptor} {
   // Scan the descriptor                                               
   auto& vram = mProducer->mVRAM;
   descriptor.ForEachDeep([&](const A::Mesh& mesh) {
      VERBOSE_VKGEOMETRY(const auto startTime = SteadyClock::now());

      // Create a buffer for each relevant data trait                   
      // Each data request will generate that data, if it hasn't yet    
      const auto indices = mesh.GetData<Traits::Index>();
      if (indices) {
         VERBOSE_VKGEOMETRY("Uploading indices to VRAM: ",
            indices->GetCount(), " of ", indices->GetType());

         auto buffer = vram.Upload(*indices, VK_BUFFER_USAGE_INDEX_BUFFER_BIT);
         mIBuffers.push_back(buffer);
         mIOffsets.push_back(0);
      }

      const auto positions = mesh.GetData<Traits::Place>();
      if (positions) {
         VERBOSE_VKGEOMETRY("Uploading vertex positions to VRAM: ",
            positions->GetCount(), " of ", positions->GetType());

         auto buffer = vram.Upload(*positions, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
         mVBuffers.push_back(buffer);
         mVOffsets.push_back(0);
      }

      const auto normals = mesh.GetData<Traits::Aim>();
      if (normals) {
         VERBOSE_VKGEOMETRY("Uploading vertex normals to VRAM: ",
            normals->GetCount(), " of ", normals->GetType());

         auto buffer = vram.Upload(*normals, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
         mVBuffers.push_back(buffer);
         mVOffsets.push_back(0);
      }

      const auto textureCoords = mesh.GetData<Traits::Sampler>();
      if (textureCoords) {
         VERBOSE_VKGEOMETRY("Uploading vertex texture coordinates to VRAM: ",
            textureCoords->GetCount(), " of ", textureCoords->GetType());

         auto buffer = vram.Upload(*textureCoords, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
         mVBuffers.push_back(buffer);
         mVOffsets.push_back(0);
      }

      const auto materialIds = mesh.GetData<Traits::Material>();
      if (materialIds) {
         VERBOSE_VKGEOMETRY("Uploading vertex material IDs to VRAM: ",
            materialIds->GetCount(), " of ", materialIds->GetType());

         auto buffer = vram.Upload(*materialIds, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
         mVBuffers.push_back(buffer);
         mVOffsets.push_back(0);
      }

      const auto instances = mesh.GetData<Traits::Transform>();
      if (instances) {
         VERBOSE_VKGEOMETRY("Uploading hardware instancing data to VRAM: ",
            instances->GetCount(), " of ", instances->GetType());

         auto buffer = vram.Upload(*instances, VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
         mVBuffers.push_back(buffer);
         mVOffsets.push_back(0);
      }

      LANGULUS_ASSERT(not mVBuffers.empty(), Graphics,
         "Couldn't upload geometry to VRAM");

      // Make sure mView.mPCount means vertex count, and not            
      // primitive count - decay in order to do that                    
      mTopology = mesh.GetTopology();
      mView = mesh.GetView().Decay();

      VERBOSE_VKGEOMETRY(Logger::Green, "Data uploaded in VRAM in ",
         SteadyClock::now() - startTime);
   });
}

/// VRAM destruction                                                          
VulkanGeometry::~VulkanGeometry() {
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
}

/// Bind the vertex & index buffers                                           
void VulkanGeometry::Bind() const {
   // Bind each vertex buffers                                          
   const auto cmdbuffer = mProducer->GetRenderCB();
   for (uint32_t i = 0; i < mVBuffers.size(); ++i) {
      vkCmdBindVertexBuffers(
         cmdbuffer, i, 1, 
         &mVBuffers[i].GetBuffer(),
         &mVOffsets[i]
      );
   }

   for (size_t i = 0; i < mIBuffers.size(); ++i) {
      vkCmdBindIndexBuffer(
         cmdbuffer, 
         mIBuffers[i].GetBuffer(),
         mIOffsets[i], 
         AsVkIndexType(mIBuffers[i].GetMeta())
      );
      break;
   }
}

/// Render the vertex & index buffers                                         
void VulkanGeometry::Render() const {
   const auto cmdbuffer = mProducer->GetRenderCB();
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