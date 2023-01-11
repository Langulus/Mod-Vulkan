#pragma once
#include "Common.hpp"

LANGULUS_EXCEPTION(GLSL);


///                                                                           
///   GLSL code container & tools                                             
///                                                                           
class GLSL : public Anyness::Text {
   /// GLSL templated for each programmable pipeline                          
   static constexpr const char* Templates[ShaderStage::Counter] = {
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
   GLSL(Text&&) noexcept;

   explicit GLSL(const Block&);
   explicit GLSL(DMeta);
   explicit GLSL(TMeta);

   template<class T, Count S>
   explicit GLSL(const TVector<T, S>&);
   template<class T, Count S1, Count S2>
   explicit GLSL(const TMatrix<T, S1, S2>&);
   template<class T>
   explicit GLSL(const TQuaternion<T>&);

   NOD() GLSL Clone() const;
   NOD() static GLSL From(ShaderStage::Enum);
   /*NOD() Text::Selection Select(ShaderToken::Enum);
   NOD() Text::Selection Select(const Text&);*/
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

NOD() GLSL operator + (const GLSL&, const GLSL&);
NOD() GLSL operator + (const GLSL&, const Text&);
NOD() GLSL operator + (const Text&, const GLSL&);

template<class T>
NOD() GLSL operator + (const T&, const GLSL&) requires (!CT::Text<T>);

template<class T>
NOD() GLSL operator + (const GLSL&, const T&) requires (!CT::Text<T>);

inline GLSL operator "" _glsl(const char* text, std::size_t size) {
   return GLSL{ text, size };
}

#include "GLSL.inl"