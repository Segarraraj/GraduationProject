solution "GraduationProject"
    language "C++"
    location "build"

    configurations {
        "Debug",
		"Release",
		"Shipping"
    }

    platforms {
		"x32",
		"x64",
    }

	links {
		"d3d12", 
		"dxgi",
		"dxguid",
		"windowscodecs",
		"ole32",
		"D3DCompiler"
	}

    configuration "Debug"
        defines {
      	    "DEBUG",
	    	"VERBOSE",
			"MTR_ENABLED"
		}

		flags {
	    	"Symbols", 
		}

    configuration "Release"
		defines {
	    	"RELEASE",
	    	"VERBOSE",
			"MTR_ENABLED"
		}

        buildoptions {
	    	"-O3"
		}

    configuration "Shipping"
		defines {
	    	"SHIPPING",
		}

		buildoptions {
	    	"-O3"
		}

    project "Project"
		location "build/project"
		kind "ConsoleApp"
		objdir "build/project/obj"

		files {
			"genie.lua",
	    	"src/**.cc",
	    	"include/**.h",
			"include/**.hpp",
			"shaders/**.hlsl",
			"deps/**.h",
			"deps/**.c",
			"deps/**.cc",
			"deps/**.cpp",
		}

		includedirs {
	    	"include",
			"deps/include"
		}

	configuration "Debug"
	    targetdir "bin/project/debug"

	configuration "Release"
	    targetdir "bin/project/release"

	configuration "Shipping"
	    targetdir "bin/project/shipping"
 	    kind "WindowedApp"
