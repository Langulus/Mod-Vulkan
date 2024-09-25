///                                                                           
/// Langulus::Module::Vulkan                                                  
/// Copyright (c) 2020 Dimo Markov <team@langulus.com>                        
/// Part of the Langulus framework, see https://langulus.com                  
///                                                                           
/// SPDX-License-Identifier: GPL-3.0-or-later                                 
///                                                                           
#include "Main.hpp"
#include <Flow/Time.hpp>
#include <Flow/Verbs/Interpret.hpp>
#include <Flow/Verbs/Compare.hpp>
#include <Langulus/Platform.hpp>
#include <Langulus/Graphics.hpp>
#include <Langulus/Physical.hpp>
#include <Langulus/Mesh.hpp>
#include <Langulus/Image.hpp>
#include <catch2/catch.hpp>


/// See https://github.com/catchorg/Catch2/blob/devel/docs/tostring.md        
CATCH_TRANSLATE_EXCEPTION(::Langulus::Exception const& ex) {
   const Text serialized {ex};
   return ::std::string {Token {serialized}};
}

namespace Catch
{
   template<CT::Stringifiable T>
   struct StringMaker<T> {
      static std::string convert(T const& value) {
         return ::std::string {Token {static_cast<Text>(value)}};
      }
   };

   /// Save catch2 from doing infinite recursions with Block types            
   template<CT::Block T>
   struct is_range<T> {
      static const bool value = false;
   };

}

/*SCENARIO("Renderer creation inside a window", "[renderer]") {
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
}*/

SCENARIO("Drawing solid polygons", "[renderer]") {
   static Allocator::State memoryState;

   GIVEN("A window with a renderer") {
      // Create the scene                                               
      auto root = Thing::Root<false>(
         "GLFW",
         "Vulkan",
         "FileSystem",
         "AssetsImages",
         "AssetsGeometry",
         "AssetsMaterials",
         "Physics"
      );

      root.CreateUnit<A::Window>(Traits::Size(640, 480));
      root.CreateUnit<A::Renderer>();
      root.CreateUnit<A::Layer>();
      root.CreateUnit<A::World>();

      auto rect = root.CreateChild(Traits::Size {100}, "Rectangles");
      auto renderable = rect->CreateUnit<A::Renderable>();
      auto mesh = rect->CreateUnit<A::Mesh>(Math::Box2 {});
      auto topLeft  = rect->CreateUnit<A::Instance>(Traits::Place(100, 100), Colors::Black);
      auto topRight = rect->CreateUnit<A::Instance>(Traits::Place(540, 100), Colors::Green);
      auto botLeft  = rect->CreateUnit<A::Instance>(Traits::Place(100, 380), Colors::Blue);
      auto botRight = rect->CreateUnit<A::Instance>(Traits::Place(540, 380), Colors::White);
      root.DumpHierarchy();

      static Allocator::State memoryState2;

      for (int repeat = 0; repeat != 10; ++repeat) {
         WHEN(std::string("Update cycle #") + std::to_string(repeat)) {
            // Update the scene                                         
            root.Update(16ms);

            // And interpret the scene as an image, i.e. taking a       
            // screenshot                                               
            Verbs::InterpretAs<A::Image*> interpret;
            root.Run(interpret);

            REQUIRE(root.GetUnits().GetCount() == 4);
            REQUIRE(rect->GetUnits().GetCount() == 6);
            REQUIRE(root.GetChildren().GetCount() == 1);
            REQUIRE_FALSE(root.HasUnits<A::Image>());
            REQUIRE(interpret.IsDone());
            REQUIRE(interpret->GetCount() == 1);
            REQUIRE(interpret->IsSparse());
            REQUIRE(interpret->template CastsTo<A::Image>());

            Verbs::Compare compare {"polygons.png"};
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

