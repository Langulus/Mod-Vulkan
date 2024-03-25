///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright (c) 2020 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "VulkanBuffer.hpp"

using PCCmdBuffer = VkCommandBuffer;


///                                                                           
///   Video memory interface                                                  
///                                                                           
/// Has tools to manage the VRAM, and produce VRAM buffers                    
///                                                                           
struct VulkanMemory {
   VkPhysicalDeviceMemoryProperties mVRAM {};
   VkPhysicalDevice mAdapter {};
   VkDevice mDevice {};

   // For transfer commands                                             
   VkCommandPool mTransferPool = 0;
   // For transferring                                                  
   VkQueue mTransferer = nullptr;
   // Transfer commands buffer                                          
   PCCmdBuffer mTransferBuffer = nullptr;

public:
   void Initialize(VkPhysicalDevice, VkDevice, uint32_t transferIndex);
   void Destroy();

   uint32_t ChooseMemory(uint32_t type, VkMemoryPropertyFlags) const;
   bool CheckFormatSupport(VkFormat, VkImageTiling, VkFormatFeatureFlags) const;

   VulkanBuffer CreateBuffer(DMeta, VkDeviceSize, VkBufferUsageFlags, VkMemoryPropertyFlags) const;
   void DestroyBuffer(VulkanBuffer&) const;

   VulkanImage CreateImage(const ImageView&, VkImageUsageFlags) const;
   void DestroyImage(VulkanImage&) const;

   VkImageView CreateImageView(const VkImage&, const ImageView&, VkImageAspectFlags);
   VkImageView CreateImageView(const VulkanImage&, const ImageView&, VkImageAspectFlags);
   VkImageView CreateImageView(const VulkanImage&);

   void ImageTransfer(const VulkanImage&, VkImageLayout from, VkImageLayout to);
   void ImageTransfer(const VkImage&, VkImageLayout from, VkImageLayout to);

   VulkanBuffer Upload(const Block&, VkBufferUsageFlags);
};