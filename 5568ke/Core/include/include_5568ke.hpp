#pragma once

// clang-format off
// Include stb_image.h before tiny_gltf.h
#include <stb_image.h>
#include <stb_image_write.h>

// Now configure TinyGLTF to use stb_image
#define TINYGLTF_USE_STB_IMAGE        // Use stb_image for image loading
#define TINYGLTF_USE_STB_IMAGE_WRITE  // Use stb_image_write for image writing

// Include TinyGLTF (without implementation)
#include <tiny_gltf.h>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#define GLM_ENABLE_EXPERIMENTAL
// clang-format on
