///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "VulkanBuffer.hpp"
#include <Langulus/Mesh.hpp>


///                                                                           
///   Vulkan VRAM geometry                                                    
///                                                                           
/// Can convert RAM content to VRAM vertex/index buffers, by copying the      
/// contents to the GPU                                                       
///                                                                           
struct VulkanGeometry : A::Graphics, ProducedFrom<VulkanRenderer> {
   LANGULUS(ABSTRACT) false;
   LANGULUS_BASES(A::Graphics);

private:
   // Vertex info                                                       
   MeshView mView;
   DMeta mTopology {};

   // The buffers                                                       
   std::vector<VulkanBuffer> mVBuffers;
   std::vector<VulkanBuffer> mIBuffers;

   // The binding offsets                                               
   std::vector<VkDeviceSize> mVOffsets;
   std::vector<VkDeviceSize> mIOffsets;

public:
   VulkanGeometry(VulkanRenderer*, const Neat&);
   ~VulkanGeometry();

   void Bind() const;
   void Render() const;
};
