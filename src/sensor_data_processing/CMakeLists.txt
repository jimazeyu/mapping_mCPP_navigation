cmake_minimum_required(VERSION 3.0.2)
project(sensor_data_processing)

## Compile as C++11, supported in ROS Kinetic and newer
add_compile_options(-std=c++11)

## Find catkin macros and libraries
## if COMPONENTS list like find_package(catkin REQUIRED COMPONENTS xyz)
## is used, also find other catkin packages
find_package(catkin REQUIRED COMPONENTS
  roscpp
  rospy
)

## DEPENDS: system dependencies of this project that dependent projects also need
catkin_package(
#  INCLUDE_DIRS include
#  LIBRARIES sensor_data_processing
#  CATKIN_DEPENDS roscpp rospy
#  DEPENDS system_lib
)

include_directories(
# include
  ${catkin_INCLUDE_DIRS}
)

add_executable(add_noise src/add_noise.cpp)
add_executable(publish_path src/publish_path.cpp)

target_link_libraries(add_noise
  ${catkin_LIBRARIES}
)

target_link_libraries(publish_path
  ${catkin_LIBRARIES}
)