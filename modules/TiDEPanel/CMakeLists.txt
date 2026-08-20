cmake_minimum_required(VERSION 3.30)

project(TiDEPanel)

gmpi_plugin(
	PROJECT_NAME ${PROJECT_NAME}
	HAS_GUI
	SOURCE_FILES
	  TiDEPanelGui.cpp
	  ${GMPI_UI_SDK}/helpers/Timer.cpp
)

# The faceplate is path traced. tide_render brings its own include directory.
target_link_libraries(${PROJECT_NAME} PRIVATE tide_render)

# helpers/Timer.cpp's mac path is CFRunLoopTimer, and the loadable-bundle link
# line gets no frameworks by default -- without this the first mac build of
# this bundle failed with undefined _kCFRunLoopCommonModes / _kCFAllocatorDefault
# (E15, 2026-08-20). TIDE's static build never sees it because the wrappers
# already link CoreFoundation.
if(APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE "-framework CoreFoundation")
endif()

# Match the tracer's floating point model. modules/CMakeLists.txt turns on
# /fp:fast globally, and the SDF lambda in TiDEPanelGui.cpp is part of the
# distance function the sphere tracer marches: compiling half the field under
# one float model and half under another is how a ray steps through a surface.
tide_render_strict_fp(${PROJECT_NAME})
