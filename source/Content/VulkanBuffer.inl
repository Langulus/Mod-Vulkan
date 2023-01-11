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
inline bool VRAM::IsValid() const noexcept { 
   return mMemory && mDevice; 
}

/// Reset the buffer. This does not deallocate the memory!                    
inline void VRAM::Reset() noexcept {
   mMemory.Reset();
   mDevice.Reset();
}

/// Upload data to VRAM                                                       
inline bool VRAM::Upload(Offset offset, Size bytes, const void* data) const {
   auto raw = Lock(offset, bytes);
   ::std::memcpy(raw, data, bytes);
   Unlock();
   return true;
}

/// Lock VRAM                                                                 
inline Byte* VRAM::Lock(Offset offset, Size bytes) const {
   void* raw;
   vkMapMemory(mDevice.Get(), mMemory.Get(), offset, bytes, 0, &raw);
   return static_cast<Byte*>(raw);
}

/// Unlock VRAM                                                               
inline void VRAM::Unlock() const {
   vkUnmapMemory(mDevice.Get(), mMemory.Get());
}

/// Check if buffer contains valid VRAM allocation                            
///   @return true if VRAM allocation is valid                                
inline bool VulkanBuffer::IsValid() const noexcept {
   return mBuffer && VRAM::IsValid();
}

/// Reset the buffer. This does not deallocate the memory!                    
inline void VulkanBuffer::Reset() noexcept {
   mBuffer.Reset();
   VRAM::Reset();
}

/// Check if buffer contains valid VRAM allocation                            
inline bool VulkanImage::IsValid() const noexcept {
   return mBuffer && VRAM::IsValid();
}

/// Reset the buffer. This does not deallocate the memory                     
inline void VulkanImage::Reset() {
   mView = {};
   mInfo = {};
   mBuffer.Reset();
   VRAM::Reset();
}