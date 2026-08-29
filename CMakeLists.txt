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
# BACKLOG M2 -- this was set unconditionally and the macOS value leaked into
# iOS builds, where 13.3 is meaningless and actively harmful: libc++'s <format>
# floating-point path calls std::to_chars, marked "introduced in iOS 16.3", so
# every translation unit that formats a float failed to compile. Nine errors,
# all the same availability attribute, none of them a code problem.
#
# The iOS figure follows Jeff's ruling (2026-08-22): TIDE targets only the most
# recent couple of iOS versions, deliberately, so it can use the newest C++ and
# STL rather than working around old runtimes.
#
# 18.0 IS THE VERSION BEFORE 26, NOT A TYPO. Apple jumped iOS 18 -> 26 to align
# with the year, so 19 through 25 do not exist -- the toolchain rejects
# -mios-simulator-version-min=25.0 outright as an invalid version number, which
# is how this was found. Probed rather than assumed: 16.3, 17.0, 18.0 and 26.0
# are accepted, 25.0 is not.
#
# RAISE THIS as iOS moves; it is a policy knob, not a compatibility floor
# discovered by trial. It comfortably clears the 16.3 that std::to_chars needs.
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(CMAKE_OSX_DEPLOYMENT_TARGET "18.0" CACHE STRING "Minimum iOS deployment version")
else()
    set(CMAKE_OSX_DEPLOYMENT_TARGET "13.3" CACHE STRING "Minimum MacOS deployment version")
endif()
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

# S38. Give every Objective-C class in this build a TIDE-specific name.
#
# The Objective-C runtime has ONE FLAT, PROCESS-WIDE CLASS NAMESPACE, so two
# GMPI plugins loaded into one host that export the same class name do not get
# one each: whichever loads first wins and every later one silently receives
# the first one's implementation. macOS prints "is implemented in both ... This
# may cause spurious casting failures and mysterious crashes" and carries on.
# The _01/_03/_04 suffixes already on those names separate SDK VERSIONS; they
# can never separate two plugins built from the SAME version, which is the case
# a user hits by installing two GMPI plugins.
#
# Measured on this mac 2026-08-22: of 25 installed AU components exporting ObjC
# classes, 19 carried a unique per-plugin GUID and 6 carried the literal
# UNPATCHED placeholder GUID_GOES_HERE_PLUS_SOME_MORE_CHAR. Loading three of
# those six in one process reproduced the warning for every shared class;
# loading two plugins with distinct GUIDs produced none.
#
# DIRECTORY-SCOPED, NOT TARGET-SCOPED, AND THAT IS DELIBERATE. gmpi_ui's .mm
# sources are compiled into SEVERAL targets per plugin -- SynthEditLib,
# AU3_Wrapper, CLAP_Wrapper, AU_Wrapper and more -- so a define set on the
# plugin target alone would reach almost none of them. One build tree is one
# plugin, so setting it here is both correct and impossible to miss.
#
# The general fix belongs in GMPI's gmpi_plugin.cmake so that EVERY GMPI plugin
# gets a distinct suffix; that repo is PR-GATED and the proposal is a separate
# PR. This line fixes TIDE now and does not wait on it.
if(APPLE)
  string(REGEX REPLACE "[^A-Za-z0-9_]" "_" _tide_objc_suffix "${CMAKE_PROJECT_NAME}")
  add_compile_definitions(GMPI_OBJC_SUFFIX=_${_tide_objc_suffix})
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

# E65. The panel's progression log (REQUEST / PREVIEW / FULL / DRAW with
# timings) used to be armed by editing `#define TIDE_PANEL_TRACE_LOG 0` to 1 in
# TiDEPanelGui.cpp. That is the one instrument E65's row tells a taker to use,
# and a source edit is both a step to forget and a thing to commit by accident.
# The define is unchanged and still defaults to 0; this only gives it a switch.
#
# Applied to the ONE file, in SynthEditSem/CMakeLists.txt beside the strict-fp
# opt-out, for the same reason that one is per-source: nothing else in the
# plugin reads it.
option(TIDE_PANEL_TRACE_LOG "Arm TiDEPanel's REQUEST/PREVIEW/FULL/DRAW progression log" OFF)

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

# BACKLOG S1b -- TIDE ships with only its own modules (Jeff, 2026-08-26), so the
# third-party binary loader is compiled out: no ScanBundle/ScanPluginBinary, no
# SEM cache, no dlopen/dlsym/dlclose imports in the shipped binaries. The prefab
# scanner (ScanFolder's walk) stays -- save-as-prefab depends on it. The full
# cut is documented at the option's declaration in EditorLib/CMakeLists.txt;
# the Accept lives in tests/s1b_no_external_modules_probe.py.
set(SE_NO_EXTERNAL_MODULES ON CACHE BOOL "" FORCE)

add_subdirectory(${syntheditlib_folder} SynthEditLib)
add_subdirectory(${syntheditlib_folder}/EditorLib EditorLib)

# ---------------------------------------------------------------------------
# OPTIONAL, OFF BY DEFAULT, AND THE DEFAULT IS A LICENSING STATEMENT.
#
# TIDE_VCV_FUNDAMENTAL bundles the VCV/Cardinal Fundamental module ports
# (VCV_Fundamental_gmpi) into TIDE's fixed module set, statically, via the
# SynthEdit Rack Adaptor. Both repos are GPL-3.0-or-later - the module
# sources are VCV's, byte-for-byte - so a binary built with this ON is
# GPL-3.0-or-later and may not be distributed under TIDE's ordinary terms.
#
# OFF must mean COMPLETELY ABSENT, not merely disabled: nothing is fetched,
# nothing is compiled, nothing is linked, and TideApp.cpp's registration call
# site compiles out (the TIDE_VCV_FUNDAMENTAL define below is set only here).
# That is why every line of this lives inside the if() rather than being a
# fetched-but-unlinked dependency.
#
# Mechanism, for whoever debugs it later: the modules become OBJECT libraries
# (static_library/ in VCV_Fundamental_gmpi); each module's createModel() line
# registers at static init through the adaptor's RackFactoryStatic.cpp, which
# QUEUES the registration because the XML generator cannot run during static
# init (RACK_DISPLAY_STATE is declared after upstream's .cpp registers);
# TideApp::InitInstance flushes the queue with registerDeferredModules(),
# which lands in ModuleFactory() via SynthEditLib's gmpi::RegisterPluginWithXml.
# Their category is "Rack/VCV Fundamental", which is what the rack browser
# scope (ModuleScope::RackOnly) requires a module to start with to be listed.
option(TIDE_VCV_FUNDAMENTAL "Bundle the GPL VCV/Cardinal Fundamental module ports (makes the binaries GPL-3.0-or-later)" OFF)

# HetrickCV — the PERMISSIVE pack, and the licensing difference is the point.
# HetrickCV's own sources and panel art are CC0-1.0, and the adaptor is now
# dual-licensed (ISC OR GPL-3.0-or-later), so this one does NOT make the
# binaries GPL. It is still OFF by default because a fixed module set is a
# product decision, not a build convenience.
#
# IT IS NOT ATTRIBUTION-FREE, though its headline licence looks it: the port
# vendors Gamma (MIT, (c) 2006 Lance Putnam) and HetrickUtilities.hpp includes
# it, so every module depends on MIT code whose notice must be retained. A
# build that ships this owes that notice -- see BACKLOG E22.
option(TIDE_VCV_HETRICKCV "Bundle the CC0 HetrickCV module ports (permissive; keeps the binaries non-GPL)" OFF)

if(TIDE_VCV_FUNDAMENTAL OR TIDE_VCV_HETRICKCV)
  if(TIDE_VCV_FUNDAMENTAL)
    message(STATUS "TIDE_VCV_FUNDAMENTAL is ON: the resulting binaries are GPL-3.0-or-later")
  endif()
  if(TIDE_VCV_HETRICKCV)
    message(STATUS "TIDE_VCV_HETRICKCV is ON: HetrickCV is CC0, and it carries Gamma (MIT) -- ship the notice")
  endif()

  set(RACK_ADAPTOR_FOLDER_OVERRIDE "" CACHE PATH "path of a local SynthEdit_Rack_Adaptor repo, or blank to fetch it automatically.")
  set(VCV_FUNDAMENTAL_FOLDER_OVERRIDE "" CACHE PATH "path of a local VCV_Fundamental_gmpi repo, or blank to fetch it automatically.")
  set(HETRICKCV_FOLDER_OVERRIDE "" CACHE PATH "path of a local HetrickCV_gmpi repo, or blank to fetch it automatically.")

  # The adaptor's ordinary target serves .gmpi builds; this option makes it
  # ALSO define the static-host pair (SynthEditRackAdaptorStatic +
  # RackAdaptorStaticRegistration) that the module object libraries and the
  # aggregate below expect. CMP0077: a normal variable overrides the option.
  set(RACK_ADAPTOR_STATIC_HOST ON)

  if("${RACK_ADAPTOR_FOLDER_OVERRIDE}" STREQUAL "")
    message(STATUS "Fetching SynthEdit_Rack_Adaptor from github")
    FetchContent_Declare(
      rack_adaptor
      GIT_REPOSITORY https://github.com/JeffMcClintock/SynthEdit_Rack_Adaptor
      GIT_TAG origin/main
    )
    FetchContent_MakeAvailable(rack_adaptor)
    tide_report_dependency(SynthEdit_Rack_Adaptor "${rack_adaptor_SOURCE_DIR}" "fetched")
  else()
    message(STATUS "Using local SynthEdit_Rack_Adaptor folder")
    add_subdirectory(${RACK_ADAPTOR_FOLDER_OVERRIDE} SynthEdit_Rack_Adaptor)
    tide_check_not_shadowed(rack_adaptor "${RACK_ADAPTOR_FOLDER_OVERRIDE}")
    tide_report_dependency(SynthEdit_Rack_Adaptor "${RACK_ADAPTOR_FOLDER_OVERRIDE}" "local override")
  endif()

  # Where "tinyXml2/tinyxml2.h" resolves for the adaptor's SVG parsing. The
  # SYMBOLS come from SynthEditLib's vendored copy at link time; compiling a
  # second tinyxml2 here would be a version-skew ODR hazard for nothing.
  set(RACK_STATIC_TINYXML2_INCLUDE ${syntheditlib_folder}/modules)

  # SOURCE_SUBDIR static_library: the repo root is the .gmpi superproject
  # (own CPM bootstrap, own SDK fetches, wrappers) and must not run here.
  if(TIDE_VCV_FUNDAMENTAL)
  if("${VCV_FUNDAMENTAL_FOLDER_OVERRIDE}" STREQUAL "")
    message(STATUS "Fetching VCV_Fundamental_gmpi from github")
    FetchContent_Declare(
      vcv_fundamental
      GIT_REPOSITORY https://github.com/JeffMcClintock/VCV_Fundamental_gmpi
      GIT_TAG origin/main
      SOURCE_SUBDIR static_library
    )
    FetchContent_MakeAvailable(vcv_fundamental)
    tide_report_dependency(VCV_Fundamental "${vcv_fundamental_SOURCE_DIR}" "fetched")
  else()
    message(STATUS "Using local VCV_Fundamental_gmpi folder")
    add_subdirectory(${VCV_FUNDAMENTAL_FOLDER_OVERRIDE}/static_library VCV_Fundamental)
    tide_check_not_shadowed(vcv_fundamental "${VCV_FUNDAMENTAL_FOLDER_OVERRIDE}")
    tide_report_dependency(VCV_Fundamental "${VCV_FUNDAMENTAL_FOLDER_OVERRIDE}" "local override")
  endif()
  endif()

  # HetrickCV, same shape. Both packs share the adaptor resolved above, and the
  # static_library/ aggregates cooperate over which one contributes the single
  # RackAdaptorStaticRegistration object -- linking two copies of the deferred
  # queue would register everything twice.
  if(TIDE_VCV_HETRICKCV)
  # Both packs at once: a Rack Model is a GLOBAL symbol, so a module name in
  # both collides at link with a duplicate-symbol error naming neither pack.
  # Measured 2026-08-25: the overlap is exactly ONE name, MidSide, out of 66
  # and 38. Fundamental's copy wins because its set is the smaller and more
  # curated one; invert this line to keep HetrickCV's instead.
  if(TIDE_VCV_FUNDAMENTAL)
    set(HETRICKCV_EXCLUDE_MODULES "MidSide" CACHE STRING "" FORCE)
    message(STATUS "Both module packs are ON: excluding HetrickCV's MidSide, which collides with Fundamental's")
  endif()
  if("${HETRICKCV_FOLDER_OVERRIDE}" STREQUAL "")
    message(STATUS "Fetching HetrickCV_gmpi from github")
    FetchContent_Declare(
      hetrickcv
      GIT_REPOSITORY https://github.com/JeffMcClintock/HetrickCV_gmpi
      GIT_TAG origin/main
      SOURCE_SUBDIR static_library
    )
    FetchContent_MakeAvailable(hetrickcv)
    tide_report_dependency(HetrickCV "${hetrickcv_SOURCE_DIR}" "fetched")
  else()
    message(STATUS "Using local HetrickCV_gmpi folder")
    add_subdirectory(${HETRICKCV_FOLDER_OVERRIDE}/static_library HetrickCV)
    tide_check_not_shadowed(hetrickcv "${HETRICKCV_FOLDER_OVERRIDE}")
    tide_report_dependency(HetrickCV "${HETRICKCV_FOLDER_OVERRIDE}" "local override")
  endif()
  endif()
endif()

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
