project "Engine"
    kind "StaticLib"
    language "C++"
    cppdialect "C++20"
    staticruntime "off"
    linkoptions { "-IGNORE:4098", "-IGNORE:4006","-IGNORE:4099" }

    targetdir ("%{wks.location}/bin/" .. outputdir .. "/%{prj.name}")
    objdir ("%{wks.location}/bin-int/" .. outputdir .. "/%{prj.name}")

    pchheader "kgpch.h"
    pchsource "src/kgpch.cpp"
    
    files 
    {
		"src/Kargono.h",
		"src/kgpch.h",
		"src/kgpch.cpp",
		"src/Kargono/**.h",
		"src/Kargono/**.cpp",
        "dependencies/stb_image/**.cpp",
        "dependencies/stb_image/**.h",
        "dependencies/ImGuizmo/ImGuizmo.h",
        "dependencies/ImGuizmo/ImGuizmo.cpp",
        "dependencies/hash_library/sha256.cpp",
        "dependencies/hash_library/sha256.h",
        "dependencies/hash_library/crc32.cpp",
        "dependencies/hash_library/crc32.h",
        "dependencies/optick/src/**.cpp",
        "dependencies/optick/src/**.h",
        "src/API/**.h",
        "src/API/**.cpp"
    }

    -- prebuildcommands 
        -- {
        --     "{COPYDIR} \"%{wks.location}Engine/dependencies/dynamic_libraries\" \"%{cfg.buildtarget.directory}\""
        -- } 
        -- postbuildcommands 
        -- {
        --     "{COPYDIR} \"%{cfg.buildtarget.directory}\" \"%{cfg.buildtarget.directory}../Breakout\"",
        --     "{COPYDIR} \"%{cfg.buildtarget.directory}\" \"%{cfg.buildtarget.directory}../Sandbox3D\"",
        --     "{COPYDIR} \"%{cfg.buildtarget.directory}\" \"%{cfg.buildtarget.directory}../Sandbox\""
        -- }

    defines
    {
        "_CRT_SECURE_NO_WARNINGS",
		"GLFW_INCLUDE_NONE",
        "KG_EXPORT=0"
        
    }
    includedirs 
    {
        "src",
        "%{IncludeDir.GLFW}",
        "%{IncludeDir.GLAD}",
        "%{IncludeDir.filewatch}",
        "%{IncludeDir.Box2D}",
        "%{IncludeDir.spdlog}",
        "%{IncludeDir.imGui}",
        "%{IncludeDir.glm}",
        "%{IncludeDir.stb_image}",
        "%{IncludeDir.entt}",
        "%{IncludeDir.mono}",
        "%{IncludeDir.yaml_cpp}",
        --"%{IncludeDir.free_type}",
        "%{IncludeDir.ImGuizmo}",
        "%{IncludeDir.VulkanSDK}",
        "%{IncludeDir.OpenALSoft}",
        "%{IncludeDir.dr_wav}",
        "%{IncludeDir.hash_library}",
        "%{IncludeDir.msdf_atlas_gen}",
        "%{IncludeDir.msdfgen}",
        "%{IncludeDir.optick}",
        "%{IncludeDir.asio}"
    }

    libdirs
    {
    }

    links 
    { 
        "GLFW",
        "Box2D",
        "GLAD",
        "opengl32.lib",
        "imGui",
        "dwmapi.lib",
        "yaml-cpp",
        "msdf-atlas-gen",
        "%{Library.mono}",
        --"%{Library.free_type}",
        "ScriptEngine"
    }

    filter "files:dependencies/ImGuizmo/**.cpp"
        flags{ "NoPCH" }

    filter "files:dependencies/hash_library/**.cpp"
        flags{ "NoPCH" }

    filter "files:dependencies/optick/src/**.cpp"
        flags{ "NoPCH" }

    filter "system:windows"
        systemversion "latest"

        defines 
        {
            "KG_PLATFORM_WINDOWS"
        }

        links 
        {
            "%{Library.WinSock}",
            "%{Library.WinMM}",
            "%{Library.WinVersion}",
            "%{Library.BCrypt}"
        }
    filter "configurations:Debug"
        defines "KG_DEBUG"
        runtime "Debug"
        symbols "on"
        -- FIXME: No other platform support for moving .dll files.
        filter { "system:windows", "configurations:Debug" }
            postbuildcommands { "xcopy \"%{DynamicLibrary.OpenALSoft_Debug}\" \"%{cfg.buildtarget.directory}\" /y" }
        links
		{
			"%{Library.ShaderC_Debug}",
			"%{Library.SPIRV_Cross_Debug}",
			"%{Library.SPIRV_Cross_GLSL_Debug}",
			"%{Library.OpenALSoft_Debug}",
			-- "%{Library.msdf_gen_Debug}",
			-- "%{Library.msdf_atlas_gen_Debug}",
			-- "%{Library.msdf_gen_ext_Debug}"
		}


    filter "configurations:Release"
        defines "KG_RELEASE"
        runtime "Release"
        optimize "on"
        -- FIXME: No other platform support for moving .dll files.
        filter { "system:windows", "configurations:Release" }
            postbuildcommands { "xcopy \"%{DynamicLibrary.OpenALSoft_Release}\" \"%{cfg.buildtarget.directory}\" /y" }

        links
		{
			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}",
			"%{Library.OpenALSoft_Release}",
            -- "%{Library.msdf_gen_Release}",
            -- "%{Library.msdf_atlas_gen_Release}",
			-- "%{Library.msdf_gen_ext_Release}"
		}

    filter "configurations:Dist"
        defines "KG_DIST"
        runtime "Release"
        optimize "on"
        symbols "off"

        -- FIXME: No other platform support for moving .dll files.
        filter { "system:windows", "configurations:Dist" }
            postbuildcommands { "xcopy \"%{DynamicLibrary.OpenALSoft_Dist}\" \"%{cfg.buildtarget.directory}\" /y" }
        links
		{
			"%{Library.ShaderC_Release}",
			"%{Library.SPIRV_Cross_Release}",
			"%{Library.SPIRV_Cross_GLSL_Release}",
			"%{Library.OpenALSoft_Dist}",
            -- "%{Library.msdf_gen_Dist}",
            -- "%{Library.msdf_atlas_gen_Dist}",
			-- "%{Library.msdf_gen_ext_Dist}"
		}