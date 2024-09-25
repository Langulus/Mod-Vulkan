///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright (c) 2020 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#pragma once
#include "../Common.hpp"
#include <Langulus/Image.hpp>


///                                                                           
///   VRAM                                                                    
///                                                                           
class VRAM {
protected:
   friend struct VulkanMemory;

   // Memory associated with buffer                                     
   Own<VkDeviceMemory> mMemory;
   // Device associated with the buffer                                 
   Own<VkDevice> mDevice;

public:
   NOD() bool IsValid() const noexcept;
   void Reset() noexcept;
   bool Upload(Offset, Size, const void*) const;
   Byte* Lock(Offset, Size) const;
   void Unlock() const;

   NOD() auto& GetDevice() const noexcept { return mDevice; }
   NOD() auto& GetMemory() const noexcept { return mMemory; }
};


///                                                                           
///   VRAM buffer                                                             
///                                                                           
class VulkanBuffer : public VRAM {
protected:
   friend struct VulkanMemory;

   // Meta                                                              
   DMeta mMeta {};
   // Buffer                                                            
   Own<VkBuffer> mBuffer;

public:
   NOD() bool IsValid() const noexcept;
   void Reset() noexcept;
   NOD() DMeta GetMeta() const noexcept;
   NOD() const VkBuffer& GetBuffer() const noexcept;
};


///                                                                           
///   VRAM image                                                              
///                                                                           
class VulkanImage : public VRAM {
protected:
   friend struct VulkanMemory;

   // Meta                                                              
   ImageView mView;
   // Buffer                                                            
   Own<VkImage> mBuffer;
   // VRAM image info                                                   
   VkImageCreateInfo mInfo {};

public:
   static VulkanImage FromSwapchain(const VkDevice&, const VkImage&, const ImageView&) noexcept;

   NOD() bool IsValid() const noexcept;
   void Reset() noexcept;
   NOD() const ImageView& GetView() const noexcept;
   NOD() VkImage GetImage() const noexcept;
   NOD() const VkImageCreateInfo& GetImageCreateInfo() const noexcept;
};

#include "VulkanBuffer.inl"