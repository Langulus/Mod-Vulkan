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
   TextureView mView;
   // Buffer                                                            
   Own<VkImage> mBuffer;
   // VRAM image info                                                   
   VkImageCreateInfo mInfo {};

public:
   NOD() bool IsValid() const noexcept;
   void Reset() noexcept;
   NOD() const TextureView& GetView() const noexcept;
   NOD() VkImage GetImage() const noexcept;
   NOD() const VkImageCreateInfo& GetImageCreateInfo() const noexcept;
};

#include "VulkanBuffer.inl"