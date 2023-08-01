///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright(C) 2020 Dimo Markov <langulusteam@gmail.com>                    
///                                                                           
/// Distributed under GNU General Public License v3+                          
/// See LICENSE file, or https://www.gnu.org/licenses                         
///                                                                           
#include "Main.hpp"
#include <Flow/Time.hpp>
#include <Flow/Verbs/Interpret.hpp>
#include <Flow/Verbs/Compare.hpp>
#include "Math/Colors.hpp"
#include <catch2/catch.hpp>


/// See https://github.com/catchorg/Catch2/blob/devel/docs/tostring.md        
CATCH_TRANSLATE_EXCEPTION(::Langulus::Exception const& ex) {
   const Text serialized {ex};
   return ::std::string {Token {serialized}};
}

SCENARIO("Renderer creation inside a window", "[renderer]") {
   Allocator::State memoryState;

   for (int repeat = 0; repeat != 10; ++repeat) {
      GIVEN(std::string("Init and shutdown cycle #") + std::to_string(repeat)) {
         // Create root entity                                          
         Thing root;
         root.SetName("ROOT");

         // Create runtime at the root                                  
         root.CreateRuntime();

         // Load vulkan module                                          
         root.LoadMod("GLFW");
         root.LoadMod("Vulkan");

         WHEN("The renderer is created") {
            auto window = root.CreateUnitToken("Window", Traits::Size(640, 480));
            auto renderer = root.CreateUnitToken("Renderer");

            THEN("Various traits change") {
               root.DumpHierarchy();
               
               REQUIRE(window);
               REQUIRE(window.IsSparse());
               REQUIRE(window.CastsTo<A::Window>());

               REQUIRE(renderer);
               REQUIRE(renderer.IsSparse());
               REQUIRE(renderer.CastsTo<A::Renderer>());
            }
         }
         
         // Check for memory leaks after each cycle                     
         REQUIRE(memoryState.Assert());
      }
   }
}

SCENARIO("Drawing an empty window", "[renderer]") {
   GIVEN("A window with a renderer") {
      // Create the scene                                               
      Thing root;
      root.SetName("ROOT");
      root.CreateRuntime();
      root.LoadMod("GLFW");
      root.LoadMod("Vulkan");
      root.LoadMod("AssetsImages");
      root.CreateUnitToken("Window", Traits::Size(640, 480));
      root.CreateUnitToken("Renderer");

      for (int repeat = 0; repeat != 10; ++repeat) {
         Allocator::State memoryState;

         WHEN(std::string("Update cycle #") + std::to_string(repeat)) {
            // Update the scene                                         
            root.Update(16ms);

            // And interpret the scene as an image, i.e. taking a       
            // screenshot                                               
            Verbs::InterpretAs<A::Image> interpret;
            root.Do(interpret);

            REQUIRE_FALSE(root.HasUnits<A::Image>());
            REQUIRE(interpret.IsDone());
            REQUIRE(interpret->GetCount() == 1);
            REQUIRE(interpret->IsSparse());
            REQUIRE(interpret->template CastsTo<A::Image>());

            THEN("The window should be filled with a uniform color") {
               Verbs::Compare compare {Math::Colors::Red};
               interpret->Run(compare);

               REQUIRE(compare.IsDone());
               REQUIRE(compare->GetCount() == 1);
               REQUIRE(compare->IsDense());
               REQUIRE(compare.GetOutput() == Compared::Equal);

               root.DumpHierarchy();
            }
         }

         // Check for memory leaks after each cycle                     
         REQUIRE(memoryState.Assert());
      }
   }
}

