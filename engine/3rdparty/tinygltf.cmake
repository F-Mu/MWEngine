file(GLOB tinygltf_sources CONFIGURE_DEPENDS  "${CMAKE_CURRENT_SOURCE_DIR}/tinygltf/*.h")
add_library(tinygltf INTERFACE ${tinygltf_sources})
target_include_directories(tinygltf INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/tinygltf)