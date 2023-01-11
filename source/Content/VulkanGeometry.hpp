///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "VulkanBuffer.hpp"


///                                                                           
///   Vulkan VRAM geometry                                                    
///                                                                           
/// Can convert RAM content to VRAM vertex/index buffers, by copying the      
/// contents to the GPU                                                       
///                                                                           
class VulkanGeometry : public ContentVRAM {
   LANGULUS(PRODUCER) VulkanRenderer;
   LANGULUS_VERBS(Verbs::Create);

private:
   // Vertex info                                                       
   VertexView mView;
   DMeta mTopology {};

   // The buffers                                                       
   std::vector<VulkanBuffer> mVBuffers;
   std::vector<VulkanBuffer> mIBuffers;

   // The binding offsets                                               
   std::vector<VkDeviceSize> mVOffsets;
   std::vector<VkDeviceSize> mIOffsets;

public:
   VulkanGeometry(VulkanRenderer*);
   VulkanGeometry(VulkanGeometry&&) noexcept = default;
   VulkanGeometry& operator = (VulkanGeometry&&) noexcept = default;
   ~VulkanGeometry();

   void Initialize();
   void Uninitialize();
   void Bind() const;
   void Render() const;

   void Create(Verb&);

   using ContentVRAM::operator ==;
};
