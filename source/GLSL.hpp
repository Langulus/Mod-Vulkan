#pragma once
#include "Common.hpp"
#include <Anyness/Text.hpp>

LANGULUS_EXCEPTION(GLSL);


///                                                                           
///   GLSL code container & tools                                             
///                                                                           
struct GLSL : Text {
   LANGULUS_BASES(Text);
private:
   /// GLSL template for each programmable shader stage                       
   static constexpr Token Templates[ShaderStage::Counter] = {
      // ShaderStage::Vertex                                            
      "//#VERSION\n\n"
      "//#DEFINES\n\n"
      "//#INPUT\n\n"
      "//#OUTPUT\n\n"
      "//#UNIFORM\n\n"
      "//#FUNCTIONS\n\n"
      "void main () {\n"
      "   //#TEXTURIZE\n\n"
      "   //#COLORIZE\n\n"
      "   //#TRANSFORM\n\n"
      "   //#POSITION\n\n"
      "}",

      // ShaderStage::Geometry                                          
      "//#VERSION\n\n"
      "//#DEFINES\n\n"
      "//#INPUT\n\n"
      "//#OUTPUT\n\n"
      "//#UNIFORM\n\n"
      "//#FUNCTIONS\n\n"
      "void main () {\n"
      "   //#FOREACHSTART\n\n"
      "   //#FOREACHEND\n\n"
      "}\n",

      // ShaderStage::TessCtrl                                          
      "",

      // ShaderStage::TessEval                                          
      "",

      // ShaderStage::Pixel                                             
      "//#VERSION\n\n"
      "//#DEFINES\n\n"
      "//#INPUT\n\n"
      "//#OUTPUT\n\n"
      "//#UNIFORM\n\n"
      "//#FUNCTIONS\n\n"
      "void main () {\n"
      "   //#TRANSFORM\n\n"
      "   //#TEXTURIZE\n\n"
      "   //#COLORIZE\n\n"
      "}",

      // ShaderStage::Compute                                           
      ""
   };

public:
   using Text::Text;

   GLSL(const Text&);
   GLSL(Text&&);

   explicit GLSL(const CT::Deep auto&);
   explicit GLSL(DMeta);
   explicit GLSL(TMeta);
   explicit GLSL(CMeta);

   template<CT::DenseNumber T, Count C>
   explicit GLSL(const TVector<T, C>&);
   template<CT::DenseNumber T, Count C, Count R>
   explicit GLSL(const TMatrix<T, C, R>&);
   template<CT::DenseNumber T>
   explicit GLSL(const TQuaternion<T>&);

   NOD() static GLSL From(ShaderStage::Enum);
   NOD() bool IsDefined(const Text&);
   NOD() Text Pretty() const;
   NOD() static GLSL Type(DMeta);

   GLSL& Define(const Text&);
   GLSL& SetVersion(const Text&);

   template<class ANYTHING>
   GLSL& operator += (const ANYTHING&);

   template<CT::Data T>
   NOD() static GLSL Type();
};

LANGULUS(INLINED)
GLSL operator "" _glsl(const char* text, std::size_t size) {
   return GLSL {text, size};
}

#include "GLSL.inl"