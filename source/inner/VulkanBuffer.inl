///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#pragma once
#include "VulkanBuffer.hpp"


/// Check if buffer contains valid VRAM allocation                            
///   @return true if VRAM allocation is valid                                
LANGULUS(INLINED)
bool VRAM::IsValid() const noexcept { 
   return mMemory && mDevice; 
}

/// Reset the buffer. This does not deallocate the memory!                    
LANGULUS(INLINED)
void VRAM::Reset() noexcept {
   mMemory.Reset();
   mDevice.Reset();
}

/// Upload data to VRAM                                                       
LANGULUS(INLINED)
bool VRAM::Upload(Offset offset, Size bytes, const void* data) const {
   auto raw = Lock(offset, bytes);
   ::std::memcpy(raw, data, bytes);
   Unlock();
   return true;
}

/// Lock VRAM                                                                 
LANGULUS(INLINED)
Byte* VRAM::Lock(Offset offset, Size bytes) const {
   void* raw;
   vkMapMemory(mDevice, mMemory, offset, bytes, 0, &raw);
   return static_cast<Byte*>(raw);
}

/// Unlock VRAM                                                               
LANGULUS(INLINED)
void VRAM::Unlock() const {
   vkUnmapMemory(mDevice, mMemory);
}

/// Check if buffer contains valid VRAM allocation                            
///   @return true if VRAM allocation is valid                                
LANGULUS(INLINED)
bool VulkanBuffer::IsValid() const noexcept {
   return mBuffer && VRAM::IsValid();
}

/// Reset the buffer. This does not deallocate the memory!                    
LANGULUS(INLINED)
void VulkanBuffer::Reset() noexcept {
   mBuffer.Reset();
   VRAM::Reset();
}

LANGULUS(INLINED)
DMeta VulkanBuffer::GetMeta() const noexcept {
   return mMeta;
}

LANGULUS(INLINED)
const VkBuffer& VulkanBuffer::GetBuffer() const noexcept {
   return mBuffer;
}

/// Check if buffer contains valid VRAM allocation                            
LANGULUS(INLINED)
bool VulkanImage::IsValid() const noexcept {
   return mBuffer && VRAM::IsValid();
}

/// Reset the buffer. This does not deallocate the memory                     
LANGULUS(INLINED)
void VulkanImage::Reset() noexcept {
   mView = {};
   mInfo = {};
   mBuffer.Reset();
   VRAM::Reset();
}

/// Get the texture view                                                      
///   @return a reference to the view                                         
LANGULUS(INLINED)
const TextureView& VulkanImage::GetView() const noexcept {
   return mView;
}

/// Get the VkImage                                                           
///   @return a VkImage                                                       
LANGULUS(INLINED)
VkImage VulkanImage::GetImage() const noexcept {
   return mBuffer;
}

/// Get the VkImageCreateInfo                                                 
///   @return the image create info                                           
LANGULUS(INLINED)
const VkImageCreateInfo& VulkanImage::GetImageCreateInfo() const noexcept {
   return mInfo;
}
