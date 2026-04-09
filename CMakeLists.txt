cmake_minimum_required(VERSION 3.20)

project(sdr-renderer)

find_package(Git QUIET)
if(GIT_FOUND AND EXISTS "${PROJECT_SOURCE_DIR}/.git")
# Update submodules as needed
    option(GIT_SUBMODULE "Check submodules during build" ON)
    if(GIT_SUBMODULE)
        message(STATUS "Submodule update")
        execute_process(COMMAND ${GIT_EXECUTABLE} submodule update --init --recursive
                        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
                        RESULT_VARIABLE GIT_SUBMOD_RESULT)
        if(NOT GIT_SUBMOD_RESULT EQUAL "0")
            message(FATAL_ERROR "git submodule update --init --recursive failed with ${GIT_SUBMOD_RESULT}, please checkout submodules")
        endif()
    endif()
endif()

add_executable("${CMAKE_PROJECT_NAME}")

set_property(TARGET "${CMAKE_PROJECT_NAME}" PROPERTY CXX_STANDARD 20)

add_subdirectory(extern/glad)	#opengl loader
add_subdirectory(extern/glfw)
add_subdirectory(extern/glm)	#math library

target_link_libraries("${CMAKE_PROJECT_NAME}" PUBLIC glfw glad glm)

# Define MY_SOURCES to be a list of all the source files
file(GLOB_RECURSE MY_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp")
file(GLOB_RECURSE DEP CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui/*.cpp")

target_sources("${CMAKE_PROJECT_NAME}" PRIVATE ${MY_SOURCES} )
target_sources("${CMAKE_PROJECT_NAME}" PRIVATE ${DEP} )

include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/extern/imgui")
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/extern/stb")

target_compile_definitions("${CMAKE_PROJECT_NAME}" PRIVATE IMGUI_IMPL_OPENGL_LOADER_GLAD)
target_compile_definitions("${CMAKE_PROJECT_NAME}" PRIVATE RES_DIR="${CMAKE_SOURCE_DIR}/res")

