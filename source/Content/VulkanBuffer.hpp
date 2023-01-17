///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "../Common.hpp"


///                                                                           
///   VRAM                                                                    
///                                                                           
class VRAM {
protected:
   friend class VulkanMemory;

   // Memory associated with buffer                                     
   Own<VkDeviceMemory> mMemory;
   // Device associated with the buffer                                 
   Own<VkDevice> mDevice;

public:
   bool IsValid() const noexcept;
   void Reset() noexcept;
   bool Upload(Offset, Size, const void*) const;
   Byte* Lock(Offset, Size) const;
   void Unlock() const;
};


///                                                                           
///   VRAM buffer                                                             
///                                                                           
class VulkanBuffer : public VRAM {
protected:
   friend class VulkanMemory;

   // Meta                                                              
   DMeta mMeta {};
   // Buffer                                                            
   Own<VkBuffer> mBuffer;

public:
   bool IsValid() const noexcept;
   void Reset() noexcept;
};


///                                                                           
///   VRAM image                                                              
///                                                                           
class VulkanImage : public VRAM {
protected:
   friend class VulkanMemory;

   // Meta                                                              
   PixelView mView;
   // Buffer                                                            
   Own<VkImage> mBuffer;
   // VRAM image info                                                   
   VkImageCreateInfo mInfo {};

public:
   bool IsValid() const noexcept;
   void Reset() noexcept;
};

#include "VulkanBuffer.inl"