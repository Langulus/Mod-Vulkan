#include "GLSL.hpp"

#define GLSL_VERBOSE(a)

/// Clone the GLSL container, retaining type                                  
///   @return the cloned GLSL container                                       
GLSL GLSL::Clone() const {
   return Text::Clone();
}

/// Create GLSL from shader template                                          
///   @param stage - the shader stage to use as template                      
///   @return the GLSL code                                                   
GLSL GLSL::From(ShaderStage::Enum stage) {
   return Templates[stage];
}

/// Select shader token                                                       
///   @param token - the token to select                                      
///   @return the text selection                                              
/*Text::Selection GLSL::Select(ShaderToken::Enum token) {
   return Text::Select(ShaderToken::Names[token]);
}

/// Standard select, relayed
///   @param token - pattern to select
///   @return the text selection
Text::Selection GLSL::Select(const Text& token) {
   return Text::Select(token);
}*/

/// Check if a #define exists for a symbol                                    
///   @param symbol - the definition to search for                            
///   @return true if definition exists                                       
bool GLSL::IsDefined(const Text& symbol) {
   return Text::FindWild("*#define " + symbol + "*");
}

/// Generate a log-friendly pretty version of the code, with line             
/// numbers, colors, highlights                                               
///   @return the pretty GLSL code                                            
Text GLSL::Pretty() const {
   if (IsEmpty())
      return {};

   const auto linescount = Text::GetLineCount();
   const auto linedigits = pcNumDigits(linescount);
   Text result = "\n";
   pcptr line = 1, lstart = 0, lend = 0;
   for (pcptr i = 0; i <= mCount; ++i) {
      if (i == mCount || (*this)[i] == '\n') {
         const auto size = lend - lstart;

         // Insert line number                                          
         auto segment = result.Extend(size + linedigits + 3);
         pcptr lt = 0;
         for (; lt < linedigits - pcNumDigits(line); ++lt)
            segment.GetRaw()[lt] = ' ';
         std::to_chars(segment.GetRaw() + lt, segment.GetRaw() + linedigits, line);
         segment.GetRaw()[linedigits] = '|';
         segment.GetRaw()[linedigits + 1] = ' ';

         // Insert line text                                            
         if (size)
            pcCopyMemory(GetRaw() + lstart, segment.GetRaw() + linedigits + 2, size);

         // Add new-line character at the end                           
         segment.GetRaw()[size + linedigits + 2] = '\n';

         ++line;
         lstart = lend = i + 1;
      }
      else if ((*this)[i] == '\0')
         break;
      else ++lend;
   }

   result.last() = '\0';
   return result;
}

/// Add a definition to code                                                  
///   @param definition - the definition to add                               
///   @return a reference to this code                                        
GLSL& GLSL::Define(const Text& definition) {
   const Text defined {"#define " + definition};
   if (Text::Find(defined))
      return *this;

   Select(ShaderToken::Defines) >> defined >> "\n";
   return *this;
}

/// Set GLSL version for the code                                             
///   @param version - the version string                                     
///   @return a reference to this code                                        
GLSL& GLSL::SetVersion(const Text& version) {
   if (Text::Find("#version"))
      return *this;

   Select(ShaderToken::Version) >> "#version " >> version >> "\n";
   return *this;
}

/// GLSL type string conversion                                               
///   @param meta - the type                                                  
///   @return the GLSL string                                                 
GLSL GLSL::Type(DMeta meta) {
   meta = meta->GetConcreteMeta();

   if (meta->InterpretsAs<bool>(1))
      return "bool";

   if (meta->InterpretsAs<ANumber>(1)) {
      if (meta->InterpretsAs<pcr32>())
         return "float";
      if (meta->InterpretsAs<pcu32>())
         return "uint";
      if (meta->InterpretsAs<pcr64>())
         return "double";
      if (meta->InterpretsAs<pci32>())
         return "int";
   }
   else if (meta->InterpretsAs<AVector>()) {
      auto count = 0;
      if (meta->InterpretsAs<TSizedVector<4>>())
         count = 4;
      else if (meta->InterpretsAs<TSizedVector<3>>())
         count = 3;
      else if (meta->InterpretsAs<TSizedVector<2>>())
         count = 2;
      else if (meta->InterpretsAs<TSizedVector<1>>())
         count = 1;
      else
         throw Except::GLSL(pcLogFuncError
            << "Unsupported vector size: " << meta->GetToken());

      if (meta->InterpretsAs<TTypedVector<pcr32>>()
         || meta->InterpretsAs<TTypedVector<pcu8>>())
         return Text("vec") + count;

      if (meta->InterpretsAs<TTypedVector<pcr64>>())
         return Text("dvec") + count;

      if (meta->InterpretsAs<TTypedVector<pci32>>())
         return Text("ivec") + count;

      //if (meta->InterpretsAs<TTypedVector<bool>>())
      //   return Text("bvec") + count;
   }
   else if (meta->InterpretsAs<AMatrix>()) {
      if (meta->InterpretsAs<mat2f>())
         return "mat2";
      if (meta->InterpretsAs<mat2d>())
         return "dmat2";
      if (meta->InterpretsAs<mat3f>())
         return "mat3";
      if (meta->InterpretsAs<mat3d>())
         return "dmat3";
      if (meta->InterpretsAs<mat4f>())
         return "mat4";
      if (meta->InterpretsAs<mat4d>())
         return "dmat4";
   }
   else if (meta->InterpretsAs<ATexture>()) {
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

   throw Except::GLSL(pcLogFuncError
      << "Unsupported GLSL type: " << meta->GetToken());
}