## Simple Vulkan Renderer

The architecture mimic the Piccolo engine, https://github.com/BoomingTech/Piccolo

Part of the code is from my project VulkanThreadPool, https://github.com/F-Mu/VulkanThreadPool

Part of the code is from this project,https://github.com/SaschaWillems/Vulkan

**NOW:**

+ Simple rasterization

+ Simple ray-tracing (1 SPP)

+ gltf model loading

+ Deferred+CSM+PCF+SSAO+IBL+ToneMapping

+ Simple Mesh Shader Base Pass(Without Task Shader)
```
  G-Buffer-Pass 
  /     |     \
CSM   SSAO    IBL
  \     |     /
  Shading-Pass
```
**TODO:**

+ Provide scene and object loading

+ Provide UI

+ Separate normal Vulkan device and ray-tracing Vulkan device

+ Implement different types of cameras 

+ More render passes after learning more graphics knowledge (continued)

+ ~~Become a game engine,having editor mode and game mode(give up)~~

+ Clean up

+ Camera postprocess

+ Separate lighting,shadow,ao pass

+ Use global resources

+ Use work graph(?)