cmake_minimum_required(VERSION 3.30)

project(TideRender)

# The path tracer is a plain static library with NO dependency on GMPI, the
# SynthEdit SDK, or anything else. That is deliberate: it means the preview tool
# and the regression test below build and run without a plugin host, and it
# means the renderer can be lifted out of this repo wholesale — which is the
# plan, since it is meant to end up in gmpi_ui/helpers where every GMPI module
# can use it, not just TIDE's.
add_library(tide_render STATIC
    TidePathTracer.h
    TidePathTracer.cpp
)

target_include_directories(tide_render PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_compile_features(tide_render PUBLIC cxx_std_20)

# OPT OUT of the fast-math that modules/CMakeLists.txt sets globally.
#
# /fp:fast and -ffast-math let the compiler reassociate float expressions and
# assume no NaN or infinity ever occurs. Both assumptions are hostile here.
# Monte Carlo integration divides by sampled probability densities that can be
# arbitrarily small, and sphere tracing is a subtractive loop where reassociated
# rounding decides whether a ray stops at a surface or steps through it. The
# code is written not to DEPEND on NaN semantics, so it is correct either way —
# but reproducibility is not, and the regression test below pins actual images,
# which means the optimiser must not be free to move them.
#
# The cost is nil: this library runs once and its result is cached.
#
# It is applied as a FUNCTION rather than to this one target because the demo
# scenes are header-only: every SDF lambda is compiled into whichever executable
# includes DemoScenes.h, so leaving those on fast-math would put half the
# renderer under one floating point model and half under another, and the
# reference images would depend on which.
# ONE definition of what "strict" means, used by both helpers below. If these
# ever diverge, a TU compiled by the source-level helper stops matching the
# library it is marching against, which is precisely the bug the comment above
# describes.
if(MSVC)
    set(TIDE_RENDER_STRICT_FP_FLAGS /fp:precise)
else()
    set(TIDE_RENDER_STRICT_FP_FLAGS -fno-fast-math)
endif()

function(tide_render_strict_fp target)
    target_compile_options(${target} PRIVATE ${TIDE_RENDER_STRICT_FP_FLAGS})
endfunction()

# BACKLOG E14. Same treatment, applied to SOURCE FILES rather than a target.
#
# TIDE links the tracer into a plugin whose OTHER translation units — the whole
# DSP engine — are deliberately on fast math (TideSynth/CMakeLists.txt sets
# /fp:fast on MSVC and an explicit reassociating subset on Apple). Putting
# `tide_render_strict_fp(TIDE)` on the plugin target would quietly change the
# floating point model of the audio path to fix a renderer, which is a much
# larger change than it looks and is not E14's to make.
#
# So the opt-out travels with the ONE file that compiles the distance field.
# Source-level COMPILE_OPTIONS land after the directory-level flags on the
# command line, so the later flag wins — which is what makes this work on Apple,
# where the reassociating options are set individually rather than via
# -ffast-math.
#
# Note the flags are set on the FILE, so they apply in every target that
# compiles it. That is the point: TIDE_Rack, TIDE_Rack_VST3 and TIDE_Rack_STANDALONE all
# compile TiDEPanelGui.cpp, and all three must agree with the library.
function(tide_render_strict_fp_sources)
    set_source_files_properties(${ARGV}
        DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        PROPERTIES COMPILE_OPTIONS "${TIDE_RENDER_STRICT_FP_FLAGS}")
endfunction()

tide_render_strict_fp(tide_render)

# A module links this with:
#     target_link_libraries(<module> PRIVATE tide_render)
# and includes "TidePathTracer.h".

# --- developer preview tool -------------------------------------------------
#
# Renders a demo scene to a PNG. Not shipped, not linked into any plugin; it
# exists so a material can be judged by looking at it. Built by default because
# it costs one small translation unit and is the only way to iterate on a look
# without a DAW in the loop.
option(TIDE_RENDER_PREVIEW "Build the path tracer's PNG preview tool" ON)

if(TIDE_RENDER_PREVIEW)
    add_executable(tide_render_preview
        preview/PreviewMain.cpp
        DemoScenes.h
        PngIo.h
    )
    target_link_libraries(tide_render_preview PRIVATE tide_render)
    tide_render_strict_fp(tide_render_preview)
    set_target_properties(tide_render_preview PROPERTIES FOLDER "tools")

    # --- image regression test ----------------------------------------------
    #
    # Renders every demo scene small and compares it against a committed
    # reference PNG. This is the guard that a change to a BSDF, a light or an
    # SDF did not quietly alter how everything looks — the class of bug that no
    # unit test notices and every pair of eyes does.
    #
    # It shares the preview tool's option because it shares its scenes and its
    # PNG code; turning the preview off turns the test off with it.
    add_executable(tide_render_regression
        tests/RenderRegression.cpp
        DemoScenes.h
        PngIo.h
    )
    target_link_libraries(tide_render_regression PRIVATE tide_render)
    tide_render_strict_fp(tide_render_regression)
    set_target_properties(tide_render_regression PROPERTIES FOLDER "tests")

    # The ROOT of the reference tree, deliberately -- the test descends into
    # tests/references/macos or tests/references/windows-linux for itself. Doing
    # the selection here would fix it for ctest and leave CI and a developer's
    # own shell to repeat it, which is how the split first shipped with only the
    # CI caller updated (S44).
    add_test(NAME render_regression
        COMMAND tide_render_regression
            "${CMAKE_CURRENT_SOURCE_DIR}/tests/references"
            "${CMAKE_CURRENT_BINARY_DIR}"
    )

    # --- numeric unit test ---------------------------------------------------
    #
    # The other half of the suite. The image test above pins how things LOOK;
    # this pins the properties no picture can show — that progressive batches
    # compose exactly, that bloom carries its own alpha, that the quality rungs
    # are ordered, that Fast and Full agree on coverage. Each of those would
    # break with every reference image still matching.
    #
    # It needs no reference data and no PNG code, so unlike the image test it
    # does not depend on the preview tool's option — but it does share the demo
    # scenes, which is why it lives inside the same guard for now.
    add_executable(tide_render_unit
        tests/RenderUnit.cpp
        DemoScenes.h
    )
    target_link_libraries(tide_render_unit PRIVATE tide_render)
    tide_render_strict_fp(tide_render_unit)
    set_target_properties(tide_render_unit PROPERTIES FOLDER "tests")

    add_test(NAME render_unit COMMAND tide_render_unit)
endif()
