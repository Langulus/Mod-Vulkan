#pragma once
#include "Common.hpp"

LANGULUS_EXCEPTION(GLSL);

///                                                                           
///   GLSL code container & tools                                             
///                                                                           
class GLSL : public Anyness::Text {
public:
   using Text::Text;

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

   GLSL(const Text&);
   GLSL(Text&&) noexcept;

   explicit GLSL(const Block&);
   explicit GLSL(const DataID&);
   explicit GLSL(const TraitID&);
   explicit GLSL(DMeta);
   explicit GLSL(TMeta);

   template<class T>
   explicit GLSL(const TCol<T>&);
   template<class T, pcptr S>
   explicit GLSL(const TVec<T, S>&);
   template<class T, pcptr S1, pcptr S2>
   explicit GLSL(const TMat<T, S1, S2>&);
   template<RealNumber T>
   explicit GLSL(const TQuat<T>&);

   NOD() GLSL Clone() const;
   NOD() static GLSL From(ShaderStage::Enum);
   NOD() Text::Selection Select(ShaderToken::Enum);
   NOD() Text::Selection Select(const Text&);
   NOD() bool IsDefined(const Text&);
   NOD() Text Pretty() const;
   NOD() static GLSL Type(DMeta);

   GLSL& Define(const Text&);
   GLSL& SetVersion(const Text&);

   template<class ANYTHING>
   GLSL& operator += (const ANYTHING&);

   template<RTTI::ReflectedData T>
   NOD() static GLSL Type();
};

NOD() GLSL operator + (const GLSL&, const GLSL&);
NOD() GLSL operator + (const GLSL&, const Text&);
NOD() GLSL operator + (const Text&, const GLSL&);

template<class T>
NOD() GLSL operator + (const T&, const GLSL&) requires (!IsText<T>);

template<class T>
NOD() GLSL operator + (const GLSL&, const T&) requires (!IsText<T>);

inline GLSL operator "" _glsl(const char* text, std::size_t size) {
   return GLSL{ text, size };
}

#include "GLSL.inl"