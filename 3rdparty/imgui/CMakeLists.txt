set(CMAKE_POSITION_INDEPENDENT_CODE ON)

file(GLOB IMGUI_INCLUDE CONFIGURE_DEPENDS 
  ${IMGUI_DIR}/*.h
  ${IMGUI_DIR}/*.cpp
)

add_library(
  IMGUI_LIB STATIC
  ${IMGUI_INCLUDE}
)

target_link_libraries(
  IMGUI_LIB
  ${OPENGL_LIBRARIES}
  glfw
)

target_include_directories(
  IMGUI_LIB PUBLIC
  ${THIRD_DIR}/imgui
  ${OPENGL_INCLUDE_DIRS}
  ${IMGUI_DIR}
)