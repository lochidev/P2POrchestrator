# CMakeList.txt : CMake project for P2POrchestrator, include source and define
# project specific logic here.
#
cmake_minimum_required (VERSION 3.8)

# Add source to this project's executable.
add_executable (P2POrchestrator "main.cpp" "Network.h" "Worker.h" "MasterControl.h" "Task.h" "Lobby.h")

if (CMAKE_VERSION VERSION_GREATER 3.12)
  set_property(TARGET P2POrchestrator PROPERTY CXX_STANDARD 20)
endif()

# TODO: Add tests and install targets if needed.
