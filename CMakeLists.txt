cmake_minimum_required(VERSION 3.14) 
project(5568ke LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Find required packages
find_package(OpenGL REQUIRED)

# Compiler-specific options
if(WIN32)
  if(MSVC)
    message(STATUS "Using MSVC compiler")
    add_compile_options(/wd4819 /wd4244 /wd4267 /wd4305 "/Zc:__cplusplus")
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL")
    set(CMAKE_CXX_FLAGS_DEBUG "/MDd /O2")
    set(CMAKE_CXX_FLAGS_RELEASE "/MD /O2")
  else()
    message(STATUS "Using MinGW compiler")
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

set(PROJECT_MODULES
    Core
    Material
    ModelLoader
    ImGui
)

add_executable(${PROJECT_NAME})

# Automatically find source files for all modules
foreach(MODULE ${PROJECT_MODULES})
    file(GLOB_RECURSE MODULE_SOURCES 
         "${PROJECT_SOURCE_DIR}/5568ke/${MODULE}/*.cpp"
         "${PROJECT_SOURCE_DIR}/5568ke/${MODULE}/src/*.cpp")
    
    target_sources(${PROJECT_NAME} PRIVATE ${MODULE_SOURCES})
    target_include_directories(${PROJECT_NAME} PRIVATE
                              "${PROJECT_SOURCE_DIR}/5568ke/${MODULE}"
                              "${PROJECT_SOURCE_DIR}/5568ke/${MODULE}/include")
endforeach()

# third-party include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${THIRD_DIR}/imgui
    ${THIRD_DIR}/implot
    ${THIRD_DIR}/glfw/include
    ${THIRD_DIR}/glad/include
    ${THIRD_DIR}/glm
    ${THIRD_DIR}/stb_image
    ${THIRD_DIR}/tinygltf
    ${THIRD_DIR}/assimp/include
    ${OPENGL_INCLUDE_DIRS}
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    IMGUI_LIB
    IMPLOT_LIB
    ImGuiFileDialog
    glfw
    glad
    assimp
    ${OPENGL_LIBRARIES}
)

# Copy assets to build directory
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets
)