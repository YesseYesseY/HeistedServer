workspace "Heisted"
    configurations { "Release" }

project "HeistedClient"
    kind "SharedLib"
    language "C++"
    cppdialect "C++23"
    location "Client"
    targetdir "bin"
    objdir "obj/client"
    files {
        "Client/*.cpp",

        "minhook/src/**.c",

        "SDK/SDK/Basic.cpp",
        "SDK/SDK/CoreUObject_functions.cpp",
        "SDK/SDK/Engine_functions.cpp",
        "SDK/SDK/FortniteGame_functions.cpp",
    }
    includedirs {
        "SDK",
        "Shared",
        "minhook/include"
    }
    buildoptions { "/wd4369", "/wd4309" }
    architecture "x64"

project "HeistedServer"
    kind "SharedLib"
    language "C++"
    cppdialect "C++23"
    location "Server"
    targetdir "bin"
    objdir "obj/server"
    files {
        "Server/*.cpp",

        "minhook/src/**.c",

        "SDK/SDK/Basic.cpp",
        "SDK/SDK/CoreUObject_functions.cpp",
        "SDK/SDK/Engine_functions.cpp",
        "SDK/SDK/FortniteGame_functions.cpp",
        "SDK/SDK/GameplayAbilities_functions.cpp",
    }
    includedirs {
        "SDK",
        "Shared",
        "minhook/include"
    }
    buildoptions { "/wd4369", "/wd4309" }
    architecture "x64"
