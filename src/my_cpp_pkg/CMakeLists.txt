cmake_minimum_required(VERSION 3.8)
project(my_cpp_pkg)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_COMPILER_IS_GNUCXX OR CMAKE_CXX_COMPILER_ID MATCHES "Clang")
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

# find dependencies
find_package(ament_cmake REQUIRED)
find_package(rclcpp REQUIRED)
find_package(example_interfaces REQUIRED) #required to link dependencies in package.xml to node executable
#this is an additonal c++-specific step (when writing nodes in python, this isn't necessary)

add_executable(cpp_node src/my_first_node.cpp)
ament_target_dependencies(cpp_node rclcpp)

add_executable(robot_news_station src/robot_news_station.cpp) #use same name for executable as you used for node name -- good practice
ament_target_dependencies(robot_news_station rclcpp example_interfaces) #list all dependencies here

install(TARGETS
  cpp_node
  robot_news_station #add addl. executables here
  DESTINATION lib/${PROJECT_NAME}
)

ament_package()
