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
#include <Math/Color.hpp>
#include <Math/Primitives/Box.hpp>
#include <catch2/catch.hpp>


/// See https://github.com/catchorg/Catch2/blob/devel/docs/tostring.md        
CATCH_TRANSLATE_EXCEPTION(::Langulus::Exception const& ex) {
   const Text serialized {ex};
   return ::std::string {Token {serialized}};
}

SCENARIO("Renderer creation inside a window", "[renderer]") {
   static Allocator::State memoryState;

   for (int repeat = 0; repeat != 10; ++repeat) {
      GIVEN(std::string("Init and shutdown cycle #") + std::to_string(repeat)) {
         // Create root entity                                          
         Thing root;
         root.SetName("ROOT");
         root.CreateRuntime();
         root.LoadMod("GLFW");
         root.LoadMod("Vulkan");
         
         WHEN("A renderer is created via abstractions") {
            auto window = root.CreateUnit<A::Window>(Traits::Size(640, 480));
            auto renderer = root.CreateUnit<A::Renderer>();
            root.DumpHierarchy();
               
            REQUIRE(window);
            REQUIRE(window.IsSparse());
            REQUIRE(window.CastsTo<A::Window>());

            REQUIRE(renderer);
            REQUIRE(renderer.IsSparse());
            REQUIRE(renderer.CastsTo<A::Renderer>());

            REQUIRE(root.GetUnits().GetCount() == 2);
         }

      #if LANGULUS_FEATURE(MANAGED_REFLECTION)
         WHEN("A renderer is created via tokens") {
            auto window = root.CreateUnitToken("A::Window", Traits::Size(640, 480));
            auto renderer = root.CreateUnitToken("Renderer");
            root.DumpHierarchy();
               
            REQUIRE(window);
            REQUIRE(window.IsSparse());
            REQUIRE(window.CastsTo<A::Window>());

            REQUIRE(renderer);
            REQUIRE(renderer.IsSparse());
            REQUIRE(renderer.CastsTo<A::Renderer>());

            REQUIRE(root.GetUnits().GetCount() == 2);
         }
      #endif

         // Check for memory leaks after each cycle                     
         REQUIRE(memoryState.Assert());
      }
   }
}

SCENARIO("Drawing an empty window", "[renderer]") {
   static Allocator::State memoryState;

   GIVEN("A window with a renderer") {
      // Create the scene                                               
      Thing root;
      root.SetName("ROOT");
      root.CreateRuntime();
      root.LoadMod("GLFW");
      root.LoadMod("Vulkan");
      root.LoadMod("FileSystem");
      root.LoadMod("AssetsImages");
      root.CreateUnit<A::Window>(Traits::Size(640, 480));
      root.CreateUnit<A::Renderer>();

      static Allocator::State memoryState2;

      for (int repeat = 0; repeat != 10; ++repeat) {
         WHEN(std::string("Update cycle #") + std::to_string(repeat)) {
            // Update the scene                                         
            root.Update(16ms);

            // And interpret the scene as an image, i.e. taking a       
            // screenshot                                               
            Verbs::InterpretAs<A::Image> interpret;
            root.Run(interpret);

            REQUIRE(root.GetUnits().GetCount() == 2);
            REQUIRE_FALSE(root.HasUnits<A::Image>());
            REQUIRE(interpret.IsDone());
            REQUIRE(interpret->GetCount() == 1);
            REQUIRE(interpret->IsSparse());
            REQUIRE(interpret->template CastsTo<A::Image>());

            Verbs::Compare compare {Colors::Red};
            interpret.Then(compare);

            REQUIRE(compare.IsDone());
            REQUIRE(compare->GetCount() == 1);
            REQUIRE(compare->IsDense());
            REQUIRE(compare.GetOutput() == Compared::Equal);

            root.DumpHierarchy();

            // Check for memory leaks after each update cycle           
            REQUIRE(memoryState2.Assert());
         }
      }
   }

   // Check for memory leaks after each initialization cycle            
   REQUIRE(memoryState.Assert());
}

SCENARIO("Drawing solid polygons", "[renderer]") {
   static Allocator::State memoryState;

   GIVEN("A window with a renderer") {
      // Create the scene                                               
      Thing root;
      root.SetName("ROOT");
      root.CreateRuntime();
      root.LoadMod("GLFW");
      root.LoadMod("Vulkan");
      root.LoadMod("FileSystem");
      root.LoadMod("AssetsImages");
      root.CreateUnit<A::Window>(Traits::Size(640, 480));
      root.CreateUnit<A::Renderer>();
      root.CreateUnit<A::Layer>();

      auto rect = root.CreateChild();
      rect->AddTrait(Traits::Size {100});
      auto renderable = rect->CreateUnit<A::Renderable>();
      auto mesh = rect->CreateUnit<A::Mesh>(Math::Box2 {});
      auto topLeft  = rect->CreateUnit<A::Instance>(Traits::Place(100, 100), Colors::Black);
      auto topRight = rect->CreateUnit<A::Instance>(Traits::Place(540, 100), Colors::Green);
      auto botLeft  = rect->CreateUnit<A::Instance>(Traits::Place(100, 380), Colors::Blue);
      auto botRight = rect->CreateUnit<A::Instance>(Traits::Place(540, 380), Colors::White);
      root.DumpHierarchy();

      for (int repeat = 0; repeat != 10; ++repeat) {
         WHEN(std::string("Update cycle #") + std::to_string(repeat)) {
            // Update the scene                                         
            root.Update(16ms);

            // And interpret the scene as an image, i.e. taking a       
            // screenshot                                               
            Verbs::InterpretAs<A::Image> interpret;
            root.Do(interpret);

            /*Verbs::Compare compare {Colors::Red};
            interpret->Run(compare);

            REQUIRE(compare.GetOutput() == Compared::Equal);*/

            root.DumpHierarchy();
         }

         // Check for memory leaks after each cycle                     
         REQUIRE(memoryState.Assert());
      }
   }
}

