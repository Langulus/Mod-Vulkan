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
/// VRAM buffer helper                                                        
///                                                                           
class VRAMBuffer {
public:
   // Meta                                                              
   DMeta mMeta = nullptr;
   // Buffer                                                            
   Own<VkBuffer> mBuffer;
   // Memory associated with buffer                                     
   Own<VkDeviceMemory> mMemory;
   // Device associated with the buffer                                 
   Own<VkDevice> mDevice;

public:
   constexpr VRAMBuffer() noexcept {}
   constexpr VRAMBuffer(const VRAMBuffer&) noexcept = default;
   constexpr VRAMBuffer(VRAMBuffer&&) noexcept = default;

   constexpr VRAMBuffer& operator = (const VRAMBuffer&) noexcept = default;
   constexpr VRAMBuffer& operator = (VRAMBuffer&&) noexcept = default;

   /// Check if buffer contains valid VRAM allocation                         
   ///   @return true if VRAM allocation is valid                             
   inline bool IsValid() const noexcept { 
      return mBuffer && mMemory && mDevice; 
   }

   /// Reset the buffer. This does not deallocate the memory!                 
   inline void Reset() noexcept {
      mBuffer.Reset();
      mMemory.Reset();
      mDevice.Reset();
   }

   /// Upload data to VRAM                                                    
   inline bool Upload(const pcptr offset, const pcptr bytes, const void* data) const {
      auto raw = Lock(offset, bytes);
      pcCopyMemory(data, raw, bytes);
      Unlock();
      return true;
   }

   /// Lock VRAM                                                              
   inline pcbyte* Lock(const pcptr offset, const pcptr bytes) const {
      void* raw;
      vkMapMemory(mDevice, mMemory, offset, bytes, 0, &raw);
      return static_cast<pcbyte*>(raw);
   }

   /// Unlock VRAM                                                            
   inline void Unlock() const {
      vkUnmapMemory(mDevice, mMemory);
   }
};

///                                                                           
/// VRAM image helper                                                         
///                                                                           
class VRAMImage {
public:
   // Meta                                                              
   PixelView mView;
   // Buffer                                                            
   Own<VkImage> mBuffer;
   // VRAM image info                                                   
   VkImageCreateInfo mInfo = {};
   // Memory associated with buffer                                     
   Own<VkDeviceMemory> mMemory;
   // Device associated with the buffer                                 
   Own<VkDevice> mDevice;

public:
   constexpr VRAMImage() noexcept {}
   constexpr VRAMImage(const VRAMImage&) noexcept = default;
   constexpr VRAMImage(VRAMImage&&) noexcept = default;

   constexpr VRAMImage& operator = (const VRAMImage&) noexcept = default;
   constexpr VRAMImage& operator = (VRAMImage&&) noexcept = default;

   /// Check if buffer contains valid VRAM allocation                         
   inline bool IsValid() const noexcept {
      return mBuffer && mMemory && mDevice;
   }

   /// Reset the buffer. This does not deallocate the memory                  
   inline void Reset() {
      mView = {};
      mInfo = {};
      mBuffer.Reset();
      mMemory.Reset();
      mDevice.Reset();
   }

   /// Upload data to VRAM                                                    
   inline bool Upload(const pcptr offset, const pcptr bytes, const void* data) const {
      auto raw = Lock(offset, bytes);
      pcCopyMemory(data, raw, bytes);
      Unlock();
      return true;
   }

   /// Lock VRAM                                                              
   inline pcbyte* Lock(const pcptr offset, const pcptr bytes) const {
      void* raw;
      vkMapMemory(mDevice, mMemory, offset, bytes, 0, &raw);
      return static_cast<pcbyte*>(raw);
   }

   /// Unlock VRAM                                                            
   inline void Unlock() const {
      vkUnmapMemory(mDevice, mMemory);
   }
};
