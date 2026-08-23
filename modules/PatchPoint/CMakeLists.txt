cmake_minimum_required(VERSION 3.30)

project(PatchPoint)

gmpi_plugin(
	PROJECT_NAME ${PROJECT_NAME}
	HAS_DSP
	HAS_GUI
	SOURCE_FILES
	  PatchPoint.cpp
	  PatchPointGui.cpp
)
