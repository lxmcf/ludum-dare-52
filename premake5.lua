-- premake5.lua
workspace "Rotten Harvest"
   configurations { "Debug", "Release" }

project "Rotten Harvest"
   kind "WindowedApp"
   language "C"
   targetdir "bin/%{cfg.buildcfg}"

   files { "**.h", "**.c" }

   links { "raylib" }

   filter "configurations:Debug"
      defines { "DEBUG" }
      symbols "On"

   filter "configurations:Release"
      defines { "NDEBUG" }
      optimize "On"
