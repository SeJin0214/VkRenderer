
#define GLFW_INCLUDE_VULKAN
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE

#include <vec4.hpp>
#include <mat4x4.hpp>

#include <iostream>
#include <filesystem> 

#include "Renderer.h"



int main() 
{

    Renderer renderer;
	renderer.Run();

    return EXIT_SUCCESS;
}

