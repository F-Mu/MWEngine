﻿set(TARGET_NAME MWEditor)

file(GLOB EDITOR_SOURCES CONFIGURE_DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/source/*.cpp)

source_group(TREE "${CMAKE_CURRENT_SOURCE_DIR}" FILES ${EDITOR_SOURCES})

add_executable(${TARGET_NAME} ${EDITOR_SOURCES} ${Vulkan_LIBRARY})
add_compile_definitions("MW_ROOT_DIR=${BINARY_ROOT_DIR}")

set_target_properties(${TARGET_NAME} PROPERTIES CXX_STANDARD 17 OUTPUT_NAME "MWEditor")
set_target_properties(${TARGET_NAME} PROPERTIES FOLDER "Engine")

target_compile_options(${TARGET_NAME} PUBLIC "$<$<COMPILE_LANG_AND_ID:CXX,MSVC>:/WX->")
set(POST_BUILD_COMMANDS
  COMMAND ${CMAKE_COMMAND} -E make_directory "${BINARY_ROOT_DIR}"
  COMMAND ${CMAKE_COMMAND} -E copy_directory "$<TARGET_FILE_DIR:${TARGET_NAME}>/" "${BINARY_ROOT_DIR}"
)

add_custom_command(TARGET ${TARGET_NAME} ${POST_BUILD_COMMANDS})

target_link_libraries(${TARGET_NAME} MWRuntime)