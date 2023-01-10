///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "Content/VulkanBuffer.hpp"

using PCCmdBuffer = VkCommandBuffer;


///                                                                           
///   VRAM INTERFACE                                                          
///                                                                           
/// Has tools to manage the VRAM, produce VRAM buffers, move memory, etc.     
///                                                                           
class VRAM {
public:
   bool Initialize(VkPhysicalDevice, VkDevice, uint32_t transferIndex);
   void Destroy();

   uint32_t ChooseMemory(uint32_t type, VkMemoryPropertyFlags) const;
   bool CheckFormatSupport(VkFormat, VkImageTiling, VkFormatFeatureFlags) const;

   VRAMBuffer CreateBuffer(DMeta, VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags) const;
   void DestroyBuffer(VRAMBuffer&) const;

   VRAMImage CreateImage(const PixelView&, VkImageUsageFlags) const;
   void DestroyImage(VRAMImage&) const;

   VkImageView CreateImageView(const VkImage&, const PixelView&, VkImageAspectFlags);
   VkImageView CreateImageView(const VkImage&, const PixelView&);

   void ImageTransfer(VRAMImage&, VkImageLayout from, VkImageLayout to);
   void ImageTransfer(VkImage&, VkImageLayout from, VkImageLayout to);

   VRAMBuffer Upload(const Block&, VkBufferUsageFlags);

public:
   VkPhysicalDeviceMemoryProperties mVRAM{};
   VkPhysicalDevice mAdapter{};
   VkDevice mDevice{};

   // For transfer commands                                             
   VkCommandPool mTransferPool = 0;
   // For transferring                                                  
   VkQueue mTransferer = nullptr;
   // Transfer commands buffer                                          
   PCCmdBuffer mTransferBuffer = nullptr;
};