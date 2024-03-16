<div align="center">

  # PhotonDust renderer
</div>

![Example Render produced by PhotonDust renderer](./images/cover_image.png)

### About
PhotonDust is a spectral physically based GPU renderer written in C++ with Vulkan. It uses a simple custom json format to load scenes, which was primarily added to allow easy adjustments of the scene without needing to recompile/restart the program. Models can be loaded from a gltf binary file and the json files allows to set materials and transformations for each model. It is also possible to just define vertices in the json file. The spectral dispersion is based on the [Sellmeyer equation](https://en.wikipedia.org/wiki/Sellmeier_equation) which uses coefficients B and C to determine the refractiveness. The existing json files should demonstrate overall usage quite well.

### Features
* physically based lighting with spectral dispersion at translucent surfaces
* show histogram of currently rendered image
* use Cook-Torrance BRDF for metallic and Oren-Nayar BRDF for diffuse
* next event estimation that considers all emissive meshes

### Dependencies
#### external
* glm
* SDL2
* Vulkan
* spdlog
* Boost
* glslc (shaderc)
#### included
* Dear ImGui
* ImPlot
* VulkanMemoryAllocator
* tinygltf

