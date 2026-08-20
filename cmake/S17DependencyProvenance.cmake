# S17 -- dependency provenance: name the RESOLVED PATH, not the decision.
#
# Defined here rather than in the root CMakeLists.txt because TWO entry points
# need these commands (issue #222): TideSynth's own root, and SE16, which
# consumes TIDE via `add_subdirectory(${tidesynth_folder}/SynthEditSem ...)`
# WITHOUT ever loading TideSynth's root. When the definitions lived in the
# root, every SE16-hosted configure died at SynthEditSem's first call with
# "Unknown CMake command" -- in both the override and the fetch path -- while
# TideSynth's standalone build, the only mode CI exercises, kept passing.
# Both files now include this module; include_guard makes the second include
# of a standalone configure a no-op, so the report is cleared exactly once
# per configure whichever entry point comes first.
#
# The trap the functions exist for, found on the mac box 2026-08-19 while
# tracing E12: SE16's build printed "Using local GMPI-UI folder" while the
# include path was really `-I.../build/_deps/gmpi_ui-src`, and the two were at
# DIFFERENT COMMITS -- so the class layout being read was not the class layout
# being compiled. A message that names the DECISION ("Using local ...") cannot
# catch that. Only one that names the RESOLVED PATH can, which is what these
# do.
#
# Note there are THREE places a dependency can come from here, and only the
# first is obvious from the log: a local *_FOLDER_OVERRIDE, FetchContent's
# `build/_deps/<name>-src`, and CPM's shared cache under ~/.cache/CPM -- where
# the VST3 SDK and harfbuzz live and where nothing announced them at all.
include_guard(GLOBAL)

# CACHE INTERNAL, not a plain variable: SynthEditSem/ resolves GMPI_Wrappers in
# its own directory scope, and PARENT_SCOPE from there does not reach the root.
# FORCE-cleared here so a reconfigure rebuilds the list instead of doubling it.
set(TIDE_DEP_REPORT "" CACHE INTERNAL "S17 dependency provenance" FORCE)

function(tide_report_dependency name path origin)
  if(NOT EXISTS "${path}")
    message(FATAL_ERROR
      "S17: dependency '${name}' resolved to a path that does not exist:\n"
      "  ${path}\n"
      "Origin: ${origin}. A stale *_FOLDER_OVERRIDE in an existing CMakeCache.txt\n"
      "is the usual cause -- delete the build directory or fix the override.")
  endif()
  message(STATUS "  dep ${name}: ${path}   [${origin}]")
  set(TIDE_DEP_REPORT "${TIDE_DEP_REPORT}${name}|${path}|${origin}\n" CACHE INTERNAL "S17 dependency provenance" FORCE)
endfunction()

# An override is only honoured if nothing ALSO fetched the same name. If both
# exist, the include path decides -- and it is not the one the log named. This
# is the exact SE16 failure, made loud instead of silent.
function(tide_check_not_shadowed name override)
  if("${override}" STREQUAL "")
    return()
  endif()
  set(_shadow "${CMAKE_BINARY_DIR}/_deps/${name}-src")
  if(EXISTS "${_shadow}")
    message(FATAL_ERROR
      "S17: '${name}' has a local override AND a fetched copy; the build would\n"
      "silently compile one of them while the log names the other.\n"
      "  override: ${override}\n"
      "  fetched : ${_shadow}\n"
      "Something else in this build fetched '${name}' anyway. Delete the build\n"
      "directory; if it comes back, that fetch is the bug, not this check.")
  endif()
endfunction()
