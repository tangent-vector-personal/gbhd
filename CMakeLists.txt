# CMakeLists.txt
#
# Top-level build configuration file for the project.
#

cmake_minimum_required(VERSION 3.10)
project(gbhd LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

set(SDL3_DIR "external/SDL/")

add_subdirectory("${SDL3_DIR}" EXCLUDE_FROM_ALL)

#find_package(SDL3 REQUIRED "${SDL3_DIR}")

file(GLOB SOURCES CONFIGURE_DEPENDS "src/*.cpp")
file(GLOB HEADERS CONFIGURE_DEPENDS "src/*.h")
add_executable(gbhd)
set_target_properties(gbhd PROPERTIES LINKER_LANGUAGE CXX)
target_sources(gbhd PRIVATE ${SOURCES})
target_sources(gbhd PRIVATE ${HEADERS})

target_link_libraries(gbhd PRIVATE SDL3::SDL3)

if(WIN32)
    file(GLOB_RECURSE SDL3_DLLS "${SDL3_BINARY_DIR}/*.dll")
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy ${SDL3_DLLS} $<TARGET_FILE_DIR:gbhd>
        COMMENT "Copying SDL3 DLLs to build directory"
    )
endif()