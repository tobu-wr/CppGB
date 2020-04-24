cmake_minimum_required(VERSION 3.5)

set(CMAKE_CXX_STANDARD 14)

set(PROJECT_NAME CppGB)
project(${PROJECT_NAME} VERSION 1.0.0)

find_package(SDL2 REQUIRED)
if(NOT SDL2_FOUND)
	message(FATAL_ERROR "SDL2 not found")
endif()
include_directories(${SDL2_INCLUDE_DIRS})

add_executable(${PROJECT_NAME}
	Source/Cpu.cpp
	Source/Cpu.h
	Source/DisplayController.cpp
	Source/DisplayController.h
	Source/Error.h
	Source/EventHandler.cpp
	Source/EventHandler.h
	Source/Main.cpp
	Source/Memory.cpp
	Source/Memory.h
	Source/SoundController.cpp
	Source/SoundController.h
	Source/Types.h
)

string(STRIP ${SDL2_LIBRARIES} SDL2_LIBRARIES)
target_link_libraries(${PROJECT_NAME} ${SDL2_LIBRARIES})

