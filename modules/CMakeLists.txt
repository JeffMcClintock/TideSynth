cmake_minimum_required(VERSION 3.30)

set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "") 
set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version")
# TIDE Rack is arm64-only on macOS, ruled by Jeff 2026-08-21: *"lets change
# macOS to ARM-only, for faster building. Any straggler who wants intel can
# build it themselves."* Roughly halves the macOS build -- the universal
# build compiled every one of these translation units twice.
#
# This only takes effect because SynthEditLib and GMPI_Wrappers stopped
# FORCEing the variable on the same day; with the FORCE in place this line
# was silently overwritten, and so was a command-line -D. SynthEdit itself
# is unaffected and still ships universal.
set(CMAKE_OSX_ARCHITECTURES "arm64" CACHE STRING "Build architectures for MacOS")
set(SE_LOCAL_BUILD FALSE CACHE BOOL "Execute extra build steps for developers machine")

# This is for macOS commandline only, because it uses a single-target generator. for other targets, ref: target_compile_definitions
# this point of this is to ensure NDEBUG macro is set
if(NOT GENERATOR_IS_MULTI_CONFIG)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
endif()

include(FetchContent)

# Download the SynthEdit SDKs
#note: SOURCE_SUBDIR is a subfolder with NO cmake file (so we don't needlessly include the GMPI examples)
FetchContent_Declare(
  gmpi
  GIT_REPOSITORY https://github.com/JeffMcClintock/GMPI
  GIT_TAG origin/main
  SOURCE_SUBDIR Core
)

FetchContent_MakeAvailable(gmpi)

FetchContent_Declare(
  gmpi_ui
  GIT_REPOSITORY https://github.com/JeffMcClintock/gmpi_ui
  GIT_TAG origin/main
  SOURCE_SUBDIR helpers
)
FetchContent_MakeAvailable(gmpi_ui)
set(GMPI_UI_SDK ${gmpi_ui_SOURCE_DIR})

FetchContent_Declare(
  syntheditsdk
  GIT_REPOSITORY https://github.com/JeffMcClintock/SynthEdit_SDK
  GIT_TAG origin/master
  SOURCE_SUBDIR se_sdk3
)

FetchContent_MakeAvailable(syntheditsdk)

project(TiDEModules)

enable_testing()

set(se_sdk_folder
	${syntheditsdk_SOURCE_DIR}/se_sdk3
    )
set(se_shared_folder
    ${syntheditsdk_SOURCE_DIR}/shared
    )

set(sdk_folder ${se_sdk_folder})

set(GMPI_SDK
	${gmpi_SOURCE_DIR}
    )
	
include_directories(
    ${se_sdk_folder}
    ${se_shared_folder}
    )

set(CMAKE_CXX_STANDARD 20)

add_definitions(-D_UNICODE)
add_definitions(-DSE_TARGET_SEM)

if (MSVC)
    # Floating Point Model: Fast (/fp:fast)
    # Buffer Security Check: No (/GS-)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast /GS-")
endif()

if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffast-math")
endif()

include(${syntheditsdk_SOURCE_DIR}/plugin_helper.cmake)
include(${gmpi_SOURCE_DIR}/gmpi_plugin.cmake)

# The shared path tracer that generates the cached knob/faceplate bitmaps.
# Listed FIRST because the module folders below may link it.
add_subdirectory("common")

# Add you own modules here. One folder per module.
add_subdirectory("TiDEknob")
add_subdirectory("TiDEPanel")

