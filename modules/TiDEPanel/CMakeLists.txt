cmake_minimum_required(VERSION 3.30)

project(TiDEPanel)

gmpi_plugin(
	PROJECT_NAME ${PROJECT_NAME}
	HAS_GUI
	SOURCE_FILES
	  TiDEPanelGui.cpp
)
