cmake_minimum_required(VERSION 3.5)

project(ImGuiFileDialog)

add_library(
  ImGuiFileDialog STATIC
    ImGuiFileDialog.cpp
    ImGuiFileDialog.h
    ImGuiFileDialogConfig.h
)

target_include_directories(
  ImGuiFileDialog PUBLIC 
  ${IMGUI_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${OPENGL_INCLUDE_DIRS}
)

target_link_libraries(
  ImGuiFileDialog
  IMGUI_LIB
  glfw
  glad
  ${OPENGL_LIBRARIES}
)

if(UNIX)
    target_compile_options(ImGuiFileDialog PUBLIC -Wno-unknown-pragmas)
endif()
