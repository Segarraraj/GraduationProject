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
		"ole32"
	}

    configuration "Debug"
        defines {
      	    "DEBUG",
	    	"VERBOSE",
		}

		flags {
	    	"Symbols", 
		}

    configuration "Release"
		defines {
	    	"RELEASE",
	    	"VERBOSE",
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
	    	"source/**.cc",
	    	"include/**.h",
			"include/**.hpp",
			"shaders/**.hlsl"
		}

		includedirs {
	    	"include",
		}

	configuration "Debug"
	    targetdir "bin/project/debug"

	configuration "Release"
	    targetdir "bin/project/release"

	configuration "Shipping"
	    targetdir "bin/project/shipping"
 	    kind "WindowedApp"
