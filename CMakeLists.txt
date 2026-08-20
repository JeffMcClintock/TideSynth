cmake_minimum_required(VERSION 3.30)

# TIDE Rack -- the superproject, BACKLOG C7d.
#
# Until now TIDE could only be built from inside the private SE16 repo, whose
# root CMakeLists.txt supplied every SDK path. `SynthEditLib/CMakeLists.txt`
# consumes ${GMPI_SDK}, ${GMPI_UI_SDK} and ${VST3_SDK} and never sets them, so a
# stranger cloning this repo had nothing to configure. This file plays SE16's
# role for TIDE's subset only: fetch the SDKs, then add SynthEditLib, EditorLib
# and TIDE.
#
# Modelled on SE16/CMakeLists.txt deliberately, and kept recognisably similar so
# a change there is easy to mirror here. What is NOT here, and why:
#
#   se_vst3 / se_gmpi / se_au   SynthEdit's own plugin engine, not TIDE's.
#   EditorScreenshot            dropped from TIDE by U3 with the breadcrumb bar.
#   SynthEditCL, SynthEditWayland, tests, cancellation utils
#                               SynthEdit's applications and test harness.
#   SynthEdit2, SynthEditMac    the desktop apps.
#
# The SDK fetches and the CPM bootstrap are copied close to verbatim rather than
# reworked: they are fiddly, they are not TIDE-specific, and divergence between
# the two files is the thing most likely to break a build here without breaking
# one there.

if(MSVC)
    set(CMAKE_GENERATOR_TOOLSET "v143" CACHE STRING "Platform Toolset" FORCE)
endif()

set(CMAKE_CONFIGURATION_TYPES "Debug;Release;RelWithDebInfo" CACHE STRING "" FORCE)
set(CMAKE_OSX_DEPLOYMENT_TARGET "13.3" CACHE STRING "Minimum MacOS deployment version")
set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64" CACHE STRING "Build architectures for MacOS")
set(GMPI_SDK_FOLDER_OVERRIDE "" CACHE PATH "path of a local GMPI repo, or blank to fetch it automatically.")
set(GMPI_UI_FOLDER_OVERRIDE "" CACHE PATH "path of a local GMPI UI repo, or blank to fetch it automatically.")
set(SYNTHEDITLIB_FOLDER_OVERRIDE "" CACHE PATH "path of a local SynthEditLib repo, or blank to fetch it automatically.")
set(VST3_SDK_USE_MIDI_UMP "" CACHE BOOL "Support MIDI UMP in VST3.")
set(SE_LOCAL_BUILD FALSE CACHE BOOL "Execute extra build steps for developers machine")

include(FetchContent)

# CPM bootstrap (shared cache across builds)
set(CPM_DOWNLOAD_VERSION 0.38.6)
set(CPM_USE_NAMED_CACHE_DIRECTORIES ON)
set(CPM_GIT_SHALLOW ON) # shallow clone for speed

if(NOT DEFINED CPM_SOURCE_CACHE)
  if(WIN32)
    set(CPM_SOURCE_CACHE "$ENV{USERPROFILE}/.cpm" CACHE STRING "CPM cache dir")
  else()
    set(CPM_SOURCE_CACHE "$ENV{HOME}/.cache/CPM" CACHE STRING "CPM cache dir")
  endif()
endif()

# Pin the exact release so a corrupt/partial download is detectable.
set(CPM_HASH_SUM "11c3fa5f1ba14f15d31c2fb63dbc8628ee133d81c8d764caad9a8db9e0bacb07")

# Keep the bootstrap script under the build dir, NOT under CPM_SOURCE_CACHE.
# CPM.cmake records the directory it was included from in the cached INTERNAL
# var CPM_DIRECTORY, and on a reconfigure it return()s early WITHOUT defining
# CPMAddPackage if its current location differs from that cached value. Anchoring
# to ${CMAKE_BINARY_DIR} keeps the include path stable across reconfigures of an
# existing build tree (e.g. CMake GUI), so the guard never misfires. Heavy CPM
# *packages* (the VST3 SDK etc.) are still shared via CPM_SOURCE_CACHE.
set(CPM_DOWNLOAD_LOCATION "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")

# Download CPM.cmake with hash verification + retries. EXPECTED_HASH makes this
# self-healing: file(DOWNLOAD) skips the fetch when the cached file already
# matches, re-downloads when it is missing/empty/corrupt (e.g. a prior run wrote
# a truncated file or an HTTP 403/429 error page), and reports a bad fetch via
# STATUS. Without it a silently-failed download leaves CPMAddPackage undefined,
# and configure dies further down with "Unknown CMake command CPMAddPackage".
set(_cpm_download_ok FALSE)
foreach(_cpm_attempt RANGE 1 3)
  file(DOWNLOAD
    "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake"
    "${CPM_DOWNLOAD_LOCATION}"
    EXPECTED_HASH SHA256=${CPM_HASH_SUM}
    TLS_VERIFY ON
    STATUS _cpm_dl_status
  )
  list(GET _cpm_dl_status 0 _cpm_dl_code)
  if(_cpm_dl_code EQUAL 0)
    set(_cpm_download_ok TRUE)
    break()
  endif()
  list(GET _cpm_dl_status 1 _cpm_dl_msg)
  message(WARNING "CPM.cmake download attempt ${_cpm_attempt}/3 failed: ${_cpm_dl_msg}")
  file(REMOVE "${CPM_DOWNLOAD_LOCATION}") # don't poison the next run with a partial file
  if(_cpm_attempt LESS 3)
    execute_process(COMMAND "${CMAKE_COMMAND}" -E sleep 3)
  endif()
endforeach()
if(NOT _cpm_download_ok)
  message(FATAL_ERROR "Could not download CPM.cmake v${CPM_DOWNLOAD_VERSION} after 3 attempts. See warnings above.")
endif()

include("${CPM_DOWNLOAD_LOCATION}")

# BACKLOG S17 -- say which folder each dependency ACTUALLY resolved to.
#
# The provenance machinery (tide_report_dependency / tide_check_not_shadowed)
# lives in cmake/S17DependencyProvenance.cmake, with the reasoning, so that
# SynthEditSem can include it too: SE16 add_subdirectory()s SynthEditSem
# WITHOUT this root file, and defining the commands only here broke every
# SE16-hosted configure (issue #222).
include(${CMAKE_CURRENT_LIST_DIR}/cmake/S17DependencyProvenance.cmake)


if("${VST3_SDK_FOLDER_OVERRIDE}" STREQUAL "")
# Fetch VST3 via CPM
CPMAddPackage(
NAME vst3_sdk
GITHUB_REPOSITORY steinbergmedia/vst3sdk
GIT_TAG v3.8.0_build_66       # or a specific commit SHA
DOWNLOAD_ONLY YES              # we only need sources; actual add_subdirectory happens later
)
set(VST3_SDK ${vst3_sdk_SOURCE_DIR})
tide_report_dependency(vst3_sdk "${VST3_SDK}" "CPM cache")
else()
message(STATUS "Using local VST3 SDK")
set(VST3_SDK ${VST3_SDK_FOLDER_OVERRIDE})
tide_report_dependency(vst3_sdk "${VST3_SDK}" "local override")
endif()

# fetch VST MIDI UMP support
if(VST3_SDK_USE_MIDI_UMP)
CPMAddPackage(
    NAME vst3_midi_ump
    GITHUB_REPOSITORY JeffMcClintock/VST3_MIDI_extension
    GIT_TAG origin/main       # or a specific commit SHA
    DOWNLOAD_ONLY YES         # we only need sources; actual add_subdirectory happens later
)
set(VST3_MIDI_UMP_SDK ${vst3_midi_ump_SOURCE_DIR})
endif()

# testing set(VST3_MIDI_UMP_SDK C://SE/VST3_MIDI_extension)

# This is for macOS commandline only, because it uses a single-target generator. for other targets, ref: target_compile_definitions
# this point of this is to ensure NDEBUG macro is set
if(NOT GENERATOR_IS_MULTI_CONFIG)
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()
endif()


# SynthEditLib -- the DSP engine and, since C6, EditorLib's CMakeLists too.
if("${SYNTHEDITLIB_FOLDER_OVERRIDE}" STREQUAL "")
message(STATUS "Fetching SynthEditLib from github")
FetchContent_Declare(
  syntheditlib
  GIT_REPOSITORY https://github.com/JeffMcClintock/SynthEditLib
  GIT_TAG origin/main
  SOURCE_SUBDIR Shared
)
FetchContent_MakeAvailable(syntheditlib)
set(syntheditlib_folder ${syntheditlib_SOURCE_DIR})
tide_report_dependency(SynthEditLib "${syntheditlib_folder}" "fetched")
else()
message(STATUS "Using local SynthEditLib folder")
set(syntheditlib_folder ${SYNTHEDITLIB_FOLDER_OVERRIDE})
tide_check_not_shadowed(syntheditlib "${SYNTHEDITLIB_FOLDER_OVERRIDE}")
tide_report_dependency(SynthEditLib "${syntheditlib_folder}" "local override")
endif()

if("${GMPI_SDK_FOLDER_OVERRIDE}" STREQUAL "")
message(STATUS "Fetching GMPI from github")
FetchContent_Declare(
  gmpi
  GIT_REPOSITORY https://github.com/JeffMcClintock/GMPI
  GIT_TAG origin/main
  SOURCE_SUBDIR Core
)
FetchContent_MakeAvailable(gmpi)
set(GMPI_SDK ${gmpi_SOURCE_DIR})
tide_report_dependency(GMPI "${GMPI_SDK}" "fetched")
else()
message(STATUS "Using local GMPI folder")
set(GMPI_SDK ${GMPI_SDK_FOLDER_OVERRIDE})
tide_check_not_shadowed(gmpi "${GMPI_SDK_FOLDER_OVERRIDE}")
tide_report_dependency(GMPI "${GMPI_SDK}" "local override")
endif()

if("${GMPI_UI_FOLDER_OVERRIDE}" STREQUAL "")
message(STATUS "Fetching gmpi_ui from github")
FetchContent_Declare(
  gmpi_ui
  GIT_REPOSITORY https://github.com/JeffMcClintock/gmpi_ui
  GIT_TAG origin/main
  SOURCE_SUBDIR helpers
)
FetchContent_MakeAvailable(gmpi_ui)
set(GMPI_UI_SDK ${gmpi_ui_SOURCE_DIR})
tide_report_dependency(gmpi_ui "${GMPI_UI_SDK}" "fetched")
else()
message(STATUS "Using local gmpi_ui folder")
set(GMPI_UI_SDK ${GMPI_UI_FOLDER_OVERRIDE})
tide_check_not_shadowed(gmpi_ui "${GMPI_UI_FOLDER_OVERRIDE}")
tide_report_dependency(gmpi_ui "${GMPI_UI_SDK}" "local override")
endif()

# AudioUnit and CLAP SDKs. TIDE's own FORMATS_LIST is `GMPI VST3 STANDALONE`,
# so it ships neither -- but GMPI_Wrappers (fetched by SynthEditSem) configures
# its AU2 and CLAP wrappers unconditionally, and CMake needs the sources to
# exist even for a target nothing links. Copied from SE16 verbatim.
if(APPLE)

message(STATUS "Fetching AudioUnit SDK from github")
#note: SOURCE_SUBDIR is a subfolder with NO cmake file (so we don't needlessly generate example projects)
FetchContent_Declare(
  au2
  GIT_REPOSITORY https://github.com/apple/AudioUnitSDK
  GIT_TAG origin/main
  SOURCE_SUBDIR hooks
)
FetchContent_MakeAvailable(au2)
set(AU_SDK ${au2_SOURCE_DIR})
tide_report_dependency(AudioUnit "${AU_SDK}" "fetched")
set(AU_SDK_SRC ${au2_SOURCE_DIR}/src)
set(AU_SDK_H ${au2_SOURCE_DIR}/include)

endif() # APPLE

# fetch CLAP SDK
message(STATUS "Fetching CLAP SDK from github")
#note: SOURCE_SUBDIR is a subfolder with NO cmake file (so we don't needlessly generate example projects)
FetchContent_Declare(
  clap
  GIT_REPOSITORY https://github.com/free-audio/clap
  GIT_TAG origin/main
  SOURCE_SUBDIR conventions
)
FetchContent_MakeAvailable(clap)
set(CLAP_SDK ${clap_SOURCE_DIR})
tide_report_dependency(CLAP "${CLAP_SDK}" "fetched")

FetchContent_Declare(
  clap_helpers
  GIT_REPOSITORY https://github.com/free-audio/clap-helpers
  GIT_TAG origin/main
  SOURCE_SUBDIR tests
)
FetchContent_MakeAvailable(clap_helpers)
set(CLAP_HELPERS_SDK ${clap_helpers_SOURCE_DIR})
tide_report_dependency(clap_helpers "${CLAP_HELPERS_SDK}" "fetched")


project(TideSynth)

if(APPLE)
  enable_language(OBJC)
  enable_language(OBJCXX)
endif()

set(DSP_CORE ${syntheditlib_folder})
set(se_sdk_folder ${DSP_CORE}/modules/se_sdk3)
set(se_shared_folder ${DSP_CORE}/modules/shared)
set(se_sdk_hosting_folder ${DSP_CORE}/modules/se_sdk3_hosting)

include_directories(
    ${DSP_CORE}
    ${se_sdk_folder}
    ${DSP_CORE}/modules/shared
    ${DSP_CORE}/modules/se_sdk3_hosting
    ${GMPI_SDK}
    ${GMPI_SDK}/Core
    ${GMPI_UI_SDK}
    )

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)

add_definitions(-DUNICODE)
add_definitions(-D_UNICODE)
add_definitions(-DNOMINMAX)

if (MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /fp:fast /GS- /MP")
endif()

if(APPLE)
    # Same explicit subset SE16 uses, and for the same two reasons: -ffast-math
    # would imply -ffinite-math-only and break gmpi_ui's isinf() auto_size
    # sentinel, and -ffp-contract=off (S19) keeps macOS renders bit-comparable
    # with the x86 references the regression suite is baked against.
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-math-errno -fno-trapping-math -fno-signed-zeros -fassociative-math -freciprocal-math")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -ffp-contract=off")
endif()

option(SE_SOFTWARE_RENDERER "Compile the CPU software renderer into ALL configs (default: Debug only)" OFF)
if(SE_SOFTWARE_RENDERER)
    add_compile_definitions(SE_SOFTWARE_RENDERER_OPTION=1)
endif()

# HarfBuzz, vendored as one amalgamation source. SynthEditLib links it, so it
# must be defined BEFORE add_subdirectory(SynthEditLib).
CPMAddPackage(
    NAME harfbuzz
    GITHUB_REPOSITORY harfbuzz/harfbuzz
    GIT_TAG 14.2.1
    DOWNLOAD_ONLY YES
)
tide_report_dependency(harfbuzz "${harfbuzz_SOURCE_DIR}" "CPM cache")
add_library(gmpi_harfbuzz STATIC ${harfbuzz_SOURCE_DIR}/src/harfbuzz.cc)
target_include_directories(gmpi_harfbuzz PUBLIC ${harfbuzz_SOURCE_DIR}/src)
target_compile_definitions(gmpi_harfbuzz PUBLIC HB_NO_MT)
set_target_properties(gmpi_harfbuzz PROPERTIES CXX_STANDARD 17)
if(MSVC)
    target_compile_options(gmpi_harfbuzz PRIVATE /wd4244 /wd4267 /wd4996 /bigobj)
endif()

# BACKLOG E14 -- the path tracer behind TIDE's own panels (E17's ruling: panels
# are path traced into a cached bitmap, moving parts drawn over in vector).
# A plain static library with no GMPI or SDK dependency, so this costs one small
# target. Its tools and image-regression test are NOT wanted inside the plugin
# build -- they are the modules/ superproject's business -- so they are off here.
set(TIDE_RENDER_PREVIEW OFF CACHE BOOL "" FORCE)
add_subdirectory(modules/common TideRender)

add_subdirectory(${syntheditlib_folder} SynthEditLib)
add_subdirectory(${syntheditlib_folder}/EditorLib EditorLib)
add_subdirectory(SynthEditSem)

# S17 -- one block, after every resolution, so the provenance survives a long
# configure log. `cmake -B build 2>&1 | grep "dep "` is the whole diagnostic.
string(REPLACE "\n" ";" _tide_deps "${TIDE_DEP_REPORT}")
list(REMOVE_ITEM _tide_deps "")
list(LENGTH _tide_deps _tide_dep_count)
message(STATUS "TIDE dependency provenance -- ${_tide_dep_count} resolved:")
foreach(_row ${_tide_deps})
  string(REPLACE "|" ";" _f "${_row}")
  list(GET _f 0 _n)
  list(GET _f 1 _p)
  list(GET _f 2 _o)
  message(STATUS "  ${_n} <- ${_p} [${_o}]")
endforeach()
