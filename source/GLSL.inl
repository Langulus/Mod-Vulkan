#pragma once
#include "GLSL.hpp"


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
      Verbs::Interpret interpreter {MetaData::Of<GLSL>()};
      auto element = block.GetElementResolved(index);
      if (Verb::DispatchFlat(element, interpreter, false)) {
         if (!interpreter.GetOutput().template Is<GLSL>()) {
            throw Except::BadSerialization(
               GLSL_VERBOSE(
                  pcLogError << "Can't serialize " << element.GetToken()
                  << " to GLSL (produced " << interpreter.GetOutput().GetToken()
                  << " instead"
               )
            );
         }

         // Interpretation was a success                                
         *this += interpreter.GetOutput().Get<GLSL>();
      }
      else {
         // Failed, but try doing GASM interpretation                   
         // It works most of the time :)                                
         auto gasm = pcSerialize<GASM>(element);
         GLSL_VERBOSE(pcLogFuncWarning
            << "Inserting GASM code as GLSL: " << gasm);
         *this += gasm;
      }

      if (index < block.GetCount() - 1)
         *this += ", ";
   }
}

/// Meta data -> GLSL serializer                                              
///   @param type - type to serialize                                         
LANGULUS(ALWAYSINLINE)
GLSL::GLSL(DMeta meta)
   : GLSL {GLSL::Type(meta)} {}

/// Meta trait -> GLSL serializer                                             
///   @param trait - trait to serialize                                       
LANGULUS(ALWAYSINLINE)
GLSL::GLSL(TMeta meta)
   : Text {meta->mToken} {}

/// Meta constant -> GLSL serializer                                          
///   @param trait - trait to serialize                                       
LANGULUS(ALWAYSINLINE)
GLSL::GLSL(CMeta meta)
   : Text {meta->mToken} {}

/// Vector -> GLSL serializer                                                 
///   @tparam T - vector type (deducible)                                     
///   @tparam S - vector size (deducible)                                     
///   @param vector - vector to serialize                                     
template<CT::DenseNumber T, Count C>
LANGULUS(ALWAYSINLINE)
GLSL::GLSL(const TVector<T, C>& vector) {
   if constexpr (C == 1)
      *this += vector[0];
   else {
      *this += "vec";
      *this += C;
      *this += "(";
      for (Count i = 0; i < C; ++i) {
         *this += vector[i];
         if (i < C - 1)
            *this += ", ";
      }
      *this += ")";
   }
}

/// Matrix -> GLSL serializer                                                 
///   @tparam T - matrix type (deducible)                                     
///   @tparam S1 - matrix columns (deducible)                                 
///   @tparam S2 - matrix rows (deducible)                                    
///   @param matrix - matrix to serialize                                     
template<CT::DenseNumber T, Count C, Count R>
LANGULUS(ALWAYSINLINE)
GLSL::GLSL(const TMatrix<T, C, R>& matrix) {
   *this += "mat";
   if constexpr (C == R)
      *this += C;
   else {
      *this += C;
      *this += "x";
      *this += R;
   }
   *this += "(";
   for (Count i = 0; i < C * R; ++i) {
      *this += matrix[i];
      if (i < (C * R) - 1)
         *this += ", ";
   }
   *this += ")";
}

/// Quaternion -> GLSL serializer                                             
///   @tparam T - quaternion type (deducible)                                 
///   @param quaternion - quaternion to serialize                             
template<CT::DenseNumber T>
LANGULUS(ALWAYSINLINE)
GLSL::GLSL(const TQuaternion<T>& quaternion) {
   *this += "vec4(";
   *this += quaternion[0];
   *this += ", ";
   *this += quaternion[1];
   *this += ", ";
   *this += quaternion[2];
   *this += ", ";
   *this += quaternion[3];
   *this += ")";
}

/// GLSL static type string conversion                                        
///   @return the GLSL string                                                 
template<CT::Data T>
LANGULUS(ALWAYSINLINE)
GLSL GLSL::Type() {
   return Type(MetaData::Of<Decay<T>>());
}

/// Concatenate GLSL with GLSL                                                
///   @param lhs - left operand                                               
///   @param rhs - right operand                                              
///   @return the concatenated operands                                       
LANGULUS(ALWAYSINLINE)
GLSL operator + (const GLSL& lhs, const GLSL& rhs) {
   // It's essentially the same, as concatenating Text with Text        
   // with the only difference being, that it retains GLSL type         
   return static_cast<const Text&>(lhs) + static_cast<const Text&>(rhs);
}

/// Concatenate any Text with GLSL                                            
///   @param lhs - left operand                                               
///   @param rhs - right operand                                              
///   @return the concatenated operands as GLSL                               
LANGULUS(ALWAYSINLINE)
GLSL operator + (const Text& lhs, const GLSL& rhs) {
   // It's essentially the same, as concatenating Text with Text        
   // with the only difference being, that it retains GLSL type         
   return lhs + static_cast<const Text&>(rhs);
}

/// Concatenate GLSL with any Text                                            
///   @param lhs - left operand                                               
///   @param rhs - right operand                                              
///   @return the concatenated operands as GLSL                               
LANGULUS(ALWAYSINLINE)
GLSL operator + (const GLSL& lhs, const Text& rhs) {
   // It's essentially the same, as concatenating Text with Text        
   // with the only difference being, that it retains GLSL type         
   return static_cast<const Text&>(lhs) + rhs;
}

/// Destructive concatenation of GLSL with anything                           
/// Attempts to serialize right operand to GLSL, or GASM as an alternative    
///   @param rhs - right operand                                              
///   @return a reference to lhs                                              
template<class ANYTHING>
LANGULUS(ALWAYSINLINE)
GLSL& GLSL::operator += (const ANYTHING& rhs) {
   if constexpr (CT::Text<ANYTHING>) {
      Text::operator += (rhs);
   }
   else if constexpr (CT::Convertible<ANYTHING, GLSL>) {
      // First attempt conversion to GLSL                               
      operator += (GLSL {rhs});
   }
   else if constexpr (CT::Convertible<ANYTHING, Code>) {
      // Otherwise rely on GASM converters                              
      operator += (Code {rhs});
   }
   else LANGULUS_ERROR("GLSL converter not implemented");
   return *this;
}

/// Concatenate anything with GLSL                                            
/// Attempts to serialize left operand to GLSL, or GASM as an alternative     
///   @param lhs - left operand                                               
///   @param rhs - right operand                                              
///   @return a reference to lhs                                              
template<class T>
LANGULUS(ALWAYSINLINE)
GLSL operator + (const CT::NotText auto& lhs, const GLSL& rhs) {
   GLSL converted;
   converted += lhs;
   converted += rhs;
   return converted;
}

/// Concatenate anything with GLSL                                            
/// Attempts to serialize right operand to GLSL, or GASM as an alternative    
///   @param lhs - left operand                                               
///   @param rhs - right operand                                              
///   @return a reference to lhs                                              
LANGULUS(ALWAYSINLINE)
GLSL operator + (const GLSL& lhs, const CT::NotText auto& rhs) {
   GLSL converted;
   converted += lhs;
   converted += rhs;
   return converted;
}