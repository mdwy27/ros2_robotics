#the purpose of this code is to start gazebo and then spawn in GIGI from robot_state_publisher topic
import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import IncludeLaunchDescription
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import Command
from launch_ros.actions import Node
from launch_ros.parameter_descriptions import ParameterValue


def generate_launch_description():
    pkg_path = get_package_share_directory('gigi_description')
    world_path = os.path.join(pkg_path, 'worlds', 'four_wall_world.sdf') #path to use later to launch world with 4 walls for GIGI to drive around in simulation
    xacro_file = os.path.join(pkg_path, 'urdf', 'gigi.urdf.xacro')
    robot_description = ParameterValue(Command(['xacro ', xacro_file]), value_type=str)

    robot_state_publisher = Node( #launching brings up robot_state_publisher
        package='robot_state_publisher',
        executable='robot_state_publisher',
        parameters=[{'robot_description': robot_description}],
    )

    gazebo = IncludeLaunchDescription( #launching brings up Gazebo
        PythonLaunchDescriptionSource(
            os.path.join(get_package_share_directory('ros_gz_sim'), 'launch', 'gz_sim.launch.py')
        ),
        launch_arguments={'gz_args': f'-r {world_path}'}.items(), #when gazebo launches, it uses world path to launch the four_wall_world.sdf
        #these arguments tell gazebo what to open
    )

    spawn = Node( #launching spawns GIGI
        package='ros_gz_sim',
        executable='create',
        arguments=['-topic', 'robot_description', '-name', 'gigi', '-z', '0.1'],
        output='screen',
    )

    bridge = Node( #launching will bring up the gz_ros_bridge
        package='ros_gz_bridge',
        executable='parameter_bridge',
        arguments=[
            '--ros-args', '-p',
            f'config_file:={os.path.join(pkg_path, "config", "gz_ros_bridge.yaml")}'
        ],
        output='screen',
    )

    scan_frame_bridge = Node(
        package='tf2_ros',
        executable='static_transform_publisher',
        arguments=['0', '0', '0', '0', '0', '0', 'laser_frame', 'gigi/laser_frame/laser'],
    )

    return LaunchDescription([robot_state_publisher, gazebo, spawn, bridge, scan_frame_bridge])