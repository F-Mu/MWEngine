file(GLOB ktx_sources CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/ktx/include/*.h")
add_library(ktx INTERFACE ${ktx_sources})
target_include_directories(ktx INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/ktx/include)
