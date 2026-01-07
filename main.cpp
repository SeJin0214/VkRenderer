
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
   /* HelloTriangleApplication app;
    try 
    {
        app.run();
    }
    catch (const std::exception& e) 
    {
        std::cerr << e.what() << std::endl;

        return EXIT_FAILURE;
    }*/

    return EXIT_SUCCESS;
}

