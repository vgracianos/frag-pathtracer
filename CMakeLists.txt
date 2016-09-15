cmake_minimum_required(VERSION 3.0.2)
project(Pathtracer)

# Compilation directives.
if ( NOT CMAKE_BUILD_TYPE )
   set(CMAKE_BUILD_TYPE RELEASE)
endif()
set(CMAKE_C_FLAGS "-O2 -march=native")
set(CMAKE_C_FLAGS_DEBUG "-g -Wall")
set(CMAKE_C_FLAGS_RELEASE "-O2 -march=native")
set(CMAKE_CXX_FLAGS "-O2 -march=native -std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-g -Wall -std=c++11")
set(CMAKE_CXX_FLAGS_RELEASE "-O2 -march=native -std=c++11")

# GLEW
find_package(GLEW REQUIRED)
include_directories(GLEW_INCLUDE_DIRS)

# GLFW
add_subdirectory(libraries/glfw-3.1.2)
include_directories(libraries/glfw-3.1.2/include)

# Source files
include_directories(include)
file(GLOB SOURCES "src/*.cpp")

# Executables
add_executable(pathtracer ${SOURCES})
target_link_libraries(pathtracer ${GLEW_LIBRARIES} glfw ${GLFW_LIBRARIES} Xrandr rt)

# Copy some necessary folders
file(COPY ${CMAKE_SOURCE_DIR}/shaders DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
file(COPY ${CMAKE_SOURCE_DIR}/scenes  DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
