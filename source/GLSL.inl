#pragma once
#include "GLSL.hpp"
#include "Vulkan.hpp"


/// Construct by copying text                                                 
///   @param other - text container to shallow-copy                           
LANGULUS(INLINED)
GLSL::GLSL(const Text& other)
   : Text {other} {}

/// Construct by moving text                                                  
///   @param other - text container to move                                   
LANGULUS(INLINED)
GLSL::GLSL(Text&& other)
   : Text {Forward<Text>(other)} {}

/// Convert a block to GLSL code                                              
///   @param block - the block to convert to GLSL                             
GLSL::GLSL(const CT::Deep auto& block) : Text {} {
   if (block.IsOr())
      LANGULUS_THROW(GLSL, "GLSL doesn't support OR blocks");

   const auto count = block.GetCount();
   if (block.IsDeep()) {
      // Nest                                                           
      for (Count i = 0; i < count; ++i) {
         *this += GLSL {block.template As<Block>(i)};
         if (i < count - 1)
            *this += ", ";
      }
      return;
   }

   if (block.template CastsTo<Text>()) {
      // Contained type is code - just concatenate everything           
      for (Count i = 0; i < count; ++i) {
         *this += block.template As<Text>(i);
         if (i < count - 1)
            *this += ", ";
      }
      return;
   }

   // Finally, attempt to interpret each element as GLSL and concat     
   for (Count index = 0; index < count; ++index) {
      *this = Verbs::Interpret::To<GLSL>(block);

      // Failed, but try doing GASM interpretation                      
      // It works most of the time :)                                   
      //TODO
   }
}

/// Meta data -> GLSL serializer                                              
///   @param type - type to serialize                                         
LANGULUS(INLINED)
GLSL::GLSL(DMeta meta)
   : GLSL {GLSL::Type(meta)} {}

/// Meta trait -> GLSL serializer                                             
///   @param trait - trait to serialize                                       
LANGULUS(INLINED)
GLSL::GLSL(TMeta meta)
   : Text {meta->mToken} {}

/// Meta constant -> GLSL serializer                                          
///   @param trait - trait to serialize                                       
LANGULUS(INLINED)
GLSL::GLSL(CMeta meta)
   : Text {meta->mToken} {}

/// Vector -> GLSL serializer                                                 
///   @tparam T - vector type (deducible)                                     
///   @tparam S - vector size (deducible)                                     
///   @param vector - vector to serialize                                     
template<CT::DenseNumber T, Count C>
LANGULUS(INLINED)
GLSL::GLSL(const TVector<T, C>& vector) {
   if constexpr (C == 1)
      *this += vector[0];
   else {
      *this += GLSL::template Type<TVector<T, C>>();
      *this += '(';
      for (Count i = 0; i < C; ++i) {
         *this += vector[i];
         if (i < C - 1)
            *this += ", ";
      }
      *this += ')';
   }
}

/// Matrix -> GLSL serializer                                                 
///   @tparam T - matrix type (deducible)                                     
///   @tparam S1 - matrix columns (deducible)                                 
///   @tparam S2 - matrix rows (deducible)                                    
///   @param matrix - matrix to serialize                                     
template<CT::DenseNumber T, Count C, Count R>
LANGULUS(INLINED)
GLSL::GLSL(const TMatrix<T, C, R>& matrix) {
   *this += GLSL::template Type<TMatrix<T, C, R>>();
   *this += '(';
   for (Count i = 0; i < C * R; ++i) {
      *this += matrix[i];
      if (i < (C * R) - 1)
         *this += ", ";
   }
   *this += ')';
}

/// Quaternion -> GLSL serializer                                             
///   @tparam T - quaternion type (deducible)                                 
///   @param quaternion - quaternion to serialize                             
template<CT::DenseNumber T>
LANGULUS(INLINED)
GLSL::GLSL(const TQuaternion<T>& quaternion) {
   *this += GLSL::template Type<TQuaternion<T>>();
   *this += '(';
   *this += quaternion[0];
   *this += ", ";
   *this += quaternion[1];
   *this += ", ";
   *this += quaternion[2];
   *this += ", ";
   *this += quaternion[3];
   *this += ')';
}

/// GLSL static type string conversion                                        
///   @return the GLSL string                                                 
template<CT::Data T>
LANGULUS(INLINED)
GLSL GLSL::Type() {
   return Type(MetaData::Of<Decay<T>>());
}

/// GLSL type string conversion                                               
///   @param meta - the type                                                  
///   @return the GLSL string                                                 
inline GLSL GLSL::Type(DMeta meta) {
   meta = meta->GetMostConcrete();

   if (meta->CastsTo<A::Number>()) {
      // Scalars, vectors, matrices, quaternions, all based on numbers  
      Letter prefix {};
      RTTI::Base base;
      if (meta->GetBase<Float>(0, base)) {
         if (base.mCount == 1)
            return "float";   // Scalar                                 
      }
      else if (meta->GetBase<Double>(0, base)) {
         if (base.mCount == 1)
            return "double";  // Scalar                                 
         prefix = 'd';
      }
      else if (meta->GetBase<uint32_t>(0, base)) {
         if (base.mCount == 1)
            return "uint";    // Scalar                                 
         prefix = 'u';
      }
      else if (meta->GetBase<int32_t>(0, base)) {
         if (base.mCount == 1)
            return "int";     // Scalar                                 
         prefix = 'i';
      }
      else if (meta->GetBase<bool>(0, base)) {
         if (base.mCount == 1)
            return "bool";    // Scalar                                 
         prefix = 'b';
      }
      else LANGULUS_THROW(GLSL, "Unsupported GLSL type");

      // If this is reached, then we have a vector/matrix/quaternion    
      Text token {prefix};
      if (meta->CastsTo<A::Matrix>()) {
         // Find the number of columns in the matrix                    
         RTTI::Base matbase;
         Count columns {};
         Count rows {};

         if (meta->GetBase<A::Vector>(0, matbase)) {
            if (matbase.mCount == 1) {
               // Single column, so return as vector                    
               token += "vec";
               token += Text {base.mCount};
               return token;
            }

            columns = matbase.mCount;
            rows = base.mCount / columns;
         }

         // If reached, then we have a matrix                           
         token += "mat";
         if (columns == rows) {
            // Square matrix, so only one suffix number                 
            token += Text {columns};
         }
         else {
            // Non-square matrix, so two suffix numbers                 
            token += Text {columns};
            token += 'x';
            token += Text {rows};
         }
      }
      else if (meta->CastsTo<A::Vector>()) {
         token += "vec";
         token += Text {base.mCount};
      }
      else LANGULUS_THROW(GLSL, "Unsupported GLSL type");

      return token;
   }
   else if (meta->CastsTo<VulkanTexture>()) {
      return "sampler2D";
      //TODO distinguish these:
      //gsampler1D   GL_TEXTURE_1D   1D texture
      //gsampler2D   GL_TEXTURE_2D   2D texture
      //gsampler3D   GL_TEXTURE_3D   3D texture
      //gsamplerCube   GL_TEXTURE_CUBE_MAP   Cubemap Texture
      //gsampler2DRect   GL_TEXTURE_RECTANGLE   Rectangle Texture
      //gsampler1DArray   GL_TEXTURE_1D_ARRAY   1D Array Texture
      //gsampler2DArray   GL_TEXTURE_2D_ARRAY   2D Array Texture
      //gsamplerCubeArray   GL_TEXTURE_CUBE_MAP_ARRAY   Cubemap Array Texture (requires GL 4.0 or ARB_texture_cube_map_array)
      //gsamplerBuffer   GL_TEXTURE_BUFFER   Buffer Texture
      //gsampler2DMS   GL_TEXTURE_2D_MULTISAMPLE   Multisample Texture
      //gsampler2DMSArray   GL_TEXTURE_2D_MULTISAMPLE_ARRAY   Multisample Array Texture
   }

   LANGULUS_THROW(GLSL, "Unsupported GLSL type");
}

/// Concatenate GLSL with GLSL                                                
///   @param lhs - left operand                                               
///   @param rhs - right operand                                              
///   @return the concatenated operands                                       
/*LANGULUS(INLINED)
GLSL operator + (const GLSL& lhs, const GLSL& rhs) {
   // It's essentially the same, as concatenating Text with Text        
   // with the only difference being, that it retains GLSL type         
   return static_cast<const Text&>(lhs) + static_cast<const Text&>(rhs);
}

/// Concatenate any Text with GLSL                                            
///   @param lhs - left operand                                               
///   @param rhs - right operand                                              
///   @return the concatenated operands as GLSL                               
LANGULUS(INLINED)
GLSL operator + (const Text& lhs, const GLSL& rhs) {
   // It's essentially the same, as concatenating Text with Text        
   // with the only difference being, that it retains GLSL type         
   return lhs + static_cast<const Text&>(rhs);
}

/// Concatenate GLSL with any Text                                            
///   @param lhs - left operand                                               
///   @param rhs - right operand                                              
///   @return the concatenated operands as GLSL                               
LANGULUS(INLINED)
GLSL operator + (const GLSL& lhs, const Text& rhs) {
   // It's essentially the same, as concatenating Text with Text        
   // with the only difference being, that it retains GLSL type         
   return static_cast<const Text&>(lhs) + rhs;
}*/

/// Destructive concatenation of GLSL with anything                           
/// Attempts to serialize right operand to GLSL, or GASM as an alternative    
///   @param rhs - right operand                                              
///   @return a reference to lhs                                              
template<class T>
LANGULUS(INLINED)
GLSL& GLSL::operator += (const T& rhs) {
   if constexpr (CT::Text<T>) {
      Text::operator += (rhs);
   }
   else if constexpr (CT::Convertible<T, GLSL>) {
      // First attempt conversion to GLSL                               
      operator += (GLSL {rhs});
   }
   else if constexpr (CT::Convertible<T, Code>) {
      // Otherwise rely on GASM converters                              
      operator += (Code {rhs});
   }
   else LANGULUS_ERROR("GLSL converter not implemented");
   return *this;
}

/// Concatenate anything with GLSL                                            
/// Attempts to serialize left operand to GLSL, or Flow::Code                 
///   @param lhs - left operand                                               
///   @param rhs - right operand                                              
///   @return a reference to lhs                                              
/*LANGULUS(INLINED)
GLSL operator + (const CT::NotText auto& lhs, const GLSL& rhs) {
   GLSL converted;
   converted += lhs;
   converted += rhs;
   return converted;
}

/// Concatenate anything with GLSL                                            
/// Attempts to serialize right operand to GLSL, or Flow::Code                
///   @param lhs - left operand                                               
///   @param rhs - right operand                                              
///   @return a reference to lhs                                              
LANGULUS(INLINED)
GLSL operator + (const GLSL& lhs, const CT::NotText auto& rhs) {
   GLSL converted;
   converted += lhs;
   converted += rhs;
   return converted;
}*/