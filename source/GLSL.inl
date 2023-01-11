namespace PCFW
{

   inline GLSL::GLSL(const Text& other)
      : Text{ other } {}

   inline GLSL::GLSL(Text&& other) noexcept
      : Text{ pcForward<Text>(other) } {}

   /// DataID -> GLSL serializer                                                
   ///   @param type - type to serialize                                       
   inline GLSL::GLSL(const DataID& type)
      : GLSL{ type.GetMeta() } {}

   /// TraitID -> GLSL serializer                                             
   ///   @param trait - trait to serialize                                    
   inline GLSL::GLSL(const TraitID& trait)
      : GLSL{ trait.GetMeta() } {}

   /// Meta data -> GLSL serializer                                             
   ///   @param type - type to serialize                                       
   inline GLSL::GLSL(DMeta meta)
      : GLSL{ GLSL::Type(meta) } {}

   /// Meta trait -> GLSL serializer                                          
   ///   @param trait - trait to serialize                                    
   inline GLSL::GLSL(TMeta meta)
      : Text{ Text {TraitID::Prefix} + Text {meta->GetToken()} } {}

   /// Color -> GLSL serializer - normalizes integer vectors to range [0;1]   
   ///   @tparam T - color type (deducible)                                    
   ///   @param color - color to serialize                                    
   template<class T>
   GLSL::GLSL(const TCol<T>& color)
      : GLSL{ color.Real() } {}

   /// Vector -> GLSL serializer                                                
   ///   @tparam T - vector type (deducible)                                    
   ///   @tparam S - vector size (deducible)                                    
   ///   @param vector - vector to serialize                                    
   template<class T, pcptr S>
   GLSL::GLSL(const TVec<T, S>& vector) {
      if constexpr (S == 1)
         *this += vector[0];
      else {
         *this += "vec";
         *this += S;
         *this += "(";
         for (pcptr i = 0; i < S; ++i) {
            *this += vector[i];
            if (i < S - 1)
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
   template<class T, pcptr S1, pcptr S2>
   GLSL::GLSL(const TMat<T, S1, S2>& matrix) {
      *this += "mat";
      if constexpr (S1 == S2)
         *this += S1;
      else {
         *this += S1;
         *this += "x";
         *this += S2;
      }
      *this += "(";
      for (pcptr i = 0; i < S1 * S2; ++i) {
         *this += matrix[i];
         if (i < (S1 * S2) - 1)
            *this += ", ";
      }
      *this += ")";
   }

   /// Quaternion -> GLSL serializer                                          
   ///   @tparam T - quaternion type (deducible)                              
   ///   @param quaternion - quaternion to serialize                           
   template<RealNumber T>
   GLSL::GLSL(const TQuat<T>& quaternion) {
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
   template<RTTI::ReflectedData T>
   GLSL GLSL::Type() {
      return Type(DataID::Reflect<T>());
   }

   /// Concatenate GLSL with GLSL                                             
   ///   @param lhs - left operand                                             
   ///   @param rhs - right operand                                             
   ///   @return the concatenated operands                                    
   inline GLSL operator + (const GLSL& lhs, const GLSL& rhs) {
      // It's essentially the same, as concatenating Text with Text      
      // with the only difference being, that it retains GLSL type      
      return static_cast<const Text&>(lhs) + static_cast<const Text&>(rhs);
   }

   /// Concatenate any Text with GLSL                                          
   ///   @param lhs - left operand                                             
   ///   @param rhs - right operand                                             
   ///   @return the concatenated operands as GLSL                              
   inline GLSL operator + (const Text& lhs, const GLSL& rhs) {
      // It's essentially the same, as concatenating Text with Text      
      // with the only difference being, that it retains GLSL type      
      return lhs + static_cast<const Text&>(rhs);
   }

   /// Concatenate GLSL with any Text                                          
   ///   @param lhs - left operand                                             
   ///   @param rhs - right operand                                             
   ///   @return the concatenated operands as GLSL                              
   inline GLSL operator + (const GLSL& lhs, const Text& rhs) {
      // It's essentially the same, as concatenating Text with Text      
      // with the only difference being, that it retains GLSL type      
      return static_cast<const Text&>(lhs) + rhs;
   }

   /// Destructive concatenation of GLSL with anything                        
   /// Attempts to serialize right operand to GLSL, or GASM as an alternative   
   ///   @param rhs - right operand                                             
   ///   @return a reference to lhs                                             
   template<class ANYTHING>
   GLSL& GLSL::operator += (const ANYTHING& rhs) {
      if constexpr (IsText<ANYTHING>) {
         Text::template operator += <Text>(static_cast<const Text&>(rhs));
      }
      else {
         if constexpr (Convertible<ANYTHING, GLSL>) {
            // First attempt conversion to GLSL                           
            GLSL converted;
            TConverter<ANYTHING, GLSL>::Convert(rhs, converted);
            operator += (converted);
         }
         else if constexpr (Convertible<ANYTHING, GASM>) {
            // Otherwise rely on GASM converters                        
            GASM converted;
            TConverter<ANYTHING, GASM>::Convert(rhs, converted);
            operator += (converted);
         }
         else LANGULUS_ASSERT("GLSL converter not implemented");
      }
      return *this;
   }

   /// Concatenate anything with GLSL                                          
   /// Attempts to serialize left operand to GLSL, or GASM as an alternative   
   ///   @param lhs - left operand                                             
   ///   @param rhs - right operand                                             
   ///   @return a reference to lhs                                             
   template<class T>
   NOD() GLSL operator + (const T& lhs, const GLSL& rhs) requires (!IsText<T>) {
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
   template<class T>
   NOD() GLSL operator + (const GLSL& lhs, const T& rhs) requires (!IsText<T>) {
      GLSL converted;
      converted += lhs;
      converted += rhs;
      return converted;
   }

} // namespace PCFW