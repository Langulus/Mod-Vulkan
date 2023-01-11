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
///   VULKAN VRAM GEOMETRY BUFFER                                             
///                                                                           
/// Can convert RAM content to VRAM vertex/index buffers                      
///                                                                           
class CVulkanGeometry : public IContentVRAM {
   LANGULUS(PRODUCER) CVulkanRenderer;
   LANGULUS_VERBS(Verbs::Create);

private:
   // Vertex info                                                       
   VertexView mView;
   DMeta mTopology = nullptr;

   // The buffers                                                       
   std::vector<VRAMBuffer> mVBuffers;
   std::vector<VRAMBuffer> mIBuffers;

   // The binding offsets                                               
   std::vector<VkDeviceSize> mVOffsets;
   std::vector<VkDeviceSize> mIOffsets;

public:
   CVulkanGeometry(CVulkanRenderer*);
   CVulkanGeometry(CVulkanGeometry&&) noexcept = default;
   CVulkanGeometry& operator = (CVulkanGeometry&&) noexcept = default;
   ~CVulkanGeometry();

   void Initialize();
   void Uninitialize();
   void Bind() const;
   void Render() const;

   void Create(Verb&);

   using IContentVRAM::operator ==;
};
