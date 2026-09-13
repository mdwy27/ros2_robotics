import os
from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch_ros.actions import Node

def generate_launch_description():
    pkg_path = get_package_share_directory('gigi_description')
    slam_params = os.path.join(pkg_path, 'config', 'mapper_params_online_async.yaml')

    slam = Node(
        package='slam_toolbox',
        executable='async_slam_toolbox_node',
        #name='slam_toolbox',
        output='screen',
        parameters=[slam_params, {'use_sim_time': True}], #it is important to use sim time so clocks across gz, ros, slam are synched up; otherwise can cause problems sharing data
    )

    return LaunchDescription([slam])