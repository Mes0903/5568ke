cmake_minimum_required(VERSION 3.11)
project(5568ke LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(OpenGL REQUIRED)
if(OPENGL_FOUND)
  message(STATUS "OPENGL_FOUND-is-true")
else()
  message(FATAL_ERROR "OPENGL_FOUND-is-false")
endif()

if(WIN32)
  if (CMAKE_CXX_COMPILER_ID MATCHES "MSVC")
    MESSAGE("==================== USING MSVC TO COMILE ====================")
    add_compile_options(/wd4819 /wd4244 /wd4267 /wd4305 "/Zc:__cplusplus")
    # Use dynamic runtime
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")

    set(CMAKE_CXX_FLAGS_DEBUG "/MDd /O2")
    set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2")
  else()
    MESSAGE("==================== USING MINGW TO COMILE ====================")
    set(CMAKE_CXX_FLAGS "-Wall -Wextra")
    set(CMAKE_CXX_FLAGS_DEBUG "-g -O3")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3")
  endif()
else()
  set(CMAKE_CXX_FLAGS "-Wall -Wextra")
  set(CMAKE_CXX_FLAGS_DEBUG "-g -O3")
  set(CMAKE_CXX_FLAGS_RELEASE "-O3")
endif()

set(THIRD_DIR ${PROJECT_SOURCE_DIR}/3rdparty)
add_subdirectory(${THIRD_DIR})

add_executable(${PROJECT_NAME})

file(GLOB_RECURSE 5568ke_core LIST_DIRECTORIES false "${PROJECT_SOURCE_DIR}/5568ke/Core/src/*.cpp")
file(GLOB_RECURSE 5568ke_material LIST_DIRECTORIES false "${PROJECT_SOURCE_DIR}/5568ke/Material/src/*.cpp")
target_sources(${PROJECT_NAME} PRIVATE
  ${5568ke_core}
  ${5568ke_material}
)

target_include_directories(
  ${PROJECT_NAME}
  PRIVATE
    ${PROJECT_SOURCE_DIR}/5568ke/core/include
    ${PROJECT_SOURCE_DIR}/5568ke/Material/include
    ${THIRD_DIR}/imgui
    ${THIRD_DIR}/implot
    ${THIRD_DIR}/glfw/include
    ${THIRD_DIR}/glad/include
    ${THIRD_DIR}/glm
    ${THIRD_DIR}/stb_image
    ${THIRD_DIR}/assimp/include
    ${OPENGL_INCLUDE_DIRS}
)

target_link_libraries(
  ${PROJECT_NAME}
  PRIVATE
    IMGUI_LIB
    IMPLOT_LIB
    glfw
    glad
    assimp
    ${OPENGL_LIBRARIES}
)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)