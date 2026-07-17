cmake_minimum_required(VERSION 3.16)
project(nav_tree LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release)
endif()

# ---------- CPM.cmake bootstrap ----------
set(CPM_DOWNLOAD_VERSION 0.40.2)
set(CPM_DOWNLOAD_LOCATION
    "${CMAKE_BINARY_DIR}/cmake/CPM_${CPM_DOWNLOAD_VERSION}.cmake")
if(NOT EXISTS "${CPM_DOWNLOAD_LOCATION}")
  message(STATUS "Downloading CPM.cmake v${CPM_DOWNLOAD_VERSION}")
  file(DOWNLOAD
    "https://github.com/cpm-cmake/CPM.cmake/releases/download/v${CPM_DOWNLOAD_VERSION}/CPM.cmake"
    "${CPM_DOWNLOAD_LOCATION}")
endif()
include("${CPM_DOWNLOAD_LOCATION}")

# ---------- BehaviorTree.CPP (v3) ----------
# v3 option names differ from v4. Pre-set cache so BT.CPP's option()
# declarations pick up the desired values during its sub-build.
set(BT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(BT_BUILD_TESTS    OFF CACHE BOOL "" FORCE)
set(BT_BUILD_TOOLS    OFF CACHE BOOL "" FORCE)
set(BT_BUILD_PYTHON   OFF CACHE BOOL "" FORCE)
set(BT_BUILD_ZMQ      OFF CACHE BOOL "" FORCE)

CPMAddPackage(
  NAME BehaviorTree.CPP
  GIT_REPOSITORY https://github.com/BehaviorTree/BehaviorTree.CPP.git
  GIT_TAG        3.8.8
)

# ---------- nav_tree target ----------
add_executable(nav_tree
  src/main.cpp
  src/nav_nodes.cpp
  src/condition_nodes.cpp
)

target_include_directories(nav_tree PRIVATE src)
target_link_libraries(nav_tree PRIVATE behaviortree_cpp_v3)

# Copy demo trees to build dir so `./build/nav_tree` works out of the box.
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/trees/nav_tree.xml
  ${CMAKE_CURRENT_BINARY_DIR}/nav_tree.xml
  COPYONLY
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/trees/chase_tree.xml
  ${CMAKE_CURRENT_BINARY_DIR}/chase_tree.xml
  COPYONLY
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/trees/nav_subtree.xml
  ${CMAKE_CURRENT_BINARY_DIR}/nav_subtree.xml
  COPYONLY
)
configure_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/trees/composite_tree.xml
  ${CMAKE_CURRENT_BINARY_DIR}/composite_tree.xml
  COPYONLY
)

message(STATUS "")
message(STATUS "nav_tree configured. Build with: cmake --build build -j")
message(STATUS "Run with:                     ./build/nav_tree")
message(STATUS "")