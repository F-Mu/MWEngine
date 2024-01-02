set(ktx_SOURCE_DIR_ ${CMAKE_CURRENT_SOURCE_DIR}/ktx)

include_directories("${ktx_SOURCE_DIR_}/other_include")
file(GLOB ktx_sources CONFIGURE_DEPENDS "${ktx_SOURCE_DIR_}/*.h")
file(GLOB ktx_impl CONFIGURE_DEPENDS
        "${ktx_SOURCE_DIR_}/lib/texture.c"
        "${ktx_SOURCE_DIR_}/lib/hashlist.c"
        "${ktx_SOURCE_DIR_}/lib/checkheader.c"
        "${ktx_SOURCE_DIR_}/lib/swap.c"
        "${ktx_SOURCE_DIR_}/lib/memstream.c"
        "${ktx_SOURCE_DIR_}/lib/filestream.c")
add_library(ktx STATIC ${ktx_sources} ${ktx_impl})
target_include_directories(ktx PUBLIC $<BUILD_INTERFACE:${ktx_SOURCE_DIR_}>)
target_include_directories(ktx PUBLIC $<BUILD_INTERFACE:${vulkan_include}>)
target_link_libraries(ktx PUBLIC glfw ${vulkan_lib})