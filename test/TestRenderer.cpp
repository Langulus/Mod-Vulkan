///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Main.hpp"
#include <catch2/catch.hpp>

/// See https://github.com/catchorg/Catch2/blob/devel/docs/tostring.md        
CATCH_TRANSLATE_EXCEPTION(::Langulus::Exception const& ex) {
   const Text serialized {ex};
   return ::std::string {Token {serialized}};
}

SCENARIO("Renderer creation", "[renderer]") {
   GIVEN("A root entity") {
      // Create root entity                                             
      Thing root;
      root.SetName("ROOT"_text);

      // Create runtime at the root                                     
      root.CreateRuntime();

      // Load ImGui module                                              
      root.LoadMod("GLFW");
      root.LoadMod("Vulkan");

      WHEN("The renderer is created") {
         root.CreateUnitToken("Window");
         root.CreateUnitToken("Renderer");

         // Update once                                                 
         root.Update(Time::zero());

         THEN("Various traits change") {
            root.DumpHierarchy();

            REQUIRE(true);
         }
      }
   }
}

