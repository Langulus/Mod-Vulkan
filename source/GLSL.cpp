#include "GLSL.hpp"
#include <Anyness/Edit.hpp>
#include <Flow/Verbs/Catenate.hpp>

#define GLSL_VERBOSE(a)


/// Create GLSL from shader template                                          
///   @param stage - the shader stage to use as template                      
///   @return the GLSL code                                                   
GLSL GLSL::From(ShaderStage::Enum stage) {
   return Templates[stage];
}

/// Check if a #define exists for a symbol                                    
///   @param symbol - the definition to search for                            
///   @return true if definition exists                                       
bool GLSL::IsDefined(const Text& symbol) {
   return Text::FindWild("*#define "_text + symbol + "*"_text);
}

/// Generate a log-friendly pretty version of the code, with line             
/// numbers, colors, highlights                                               
///   @return the pretty GLSL code                                            
Text GLSL::Pretty() const {
   if (IsEmpty())
      return {};

   const auto linescount = GetLineCount();
   const auto linedigits = CountDigits(linescount);
   Text result {"\n"};
   Count line {1};
   Offset lstart {};
   Offset lend {};

   for (Offset i = 0; i <= mCount; ++i) {
      if (i == mCount || (*this)[i] == '\n') {
         const auto size = lend - lstart;

         // Insert line number                                          
         auto segment = result.Extend(size + linedigits + 3);

         Offset lt = 0;
         for (; lt < linedigits - CountDigits(line); ++lt)
            segment.GetRaw()[lt] = ' ';

         ::std::to_chars(segment.GetRaw() + lt, segment.GetRaw() + linedigits, line);
         segment.GetRaw()[linedigits] = '|';
         segment.GetRaw()[linedigits + 1] = ' ';

         // Insert line text                                            
         if (size)
            ::std::memcpy(segment.GetRaw() + linedigits + 2, GetRaw() + lstart, size);

         // Add new-line character at the end                           
         segment.GetRaw()[size + linedigits + 2] = '\n';

         ++line;
         lstart = lend = i + 1;
      }
      else if ((*this)[i] == '\0')
         break;
      else
         ++lend;
   }

   result.Last() = '\0';
   return result;
}

/// Add a definition to code                                                  
///   @param definition - the definition to add                               
///   @return a reference to this code                                        
GLSL& GLSL::Define(const Text& definition) {
   const Text defined {"#define "_text + definition};
   if (Text::Find(defined))
      return *this;

   Edit(this).Select(ShaderToken::Defines) >> defined >> "\n";
   return *this;
}

/// Set GLSL version for the code                                             
///   @param version - the version string                                     
///   @return a reference to this code                                        
GLSL& GLSL::SetVersion(const Text& version) {
   if (Text::Find("#version"))
      return *this;

   Edit(this).Select(ShaderToken::Version) >> "#version " >> version >> "\n";
   return *this;
}
