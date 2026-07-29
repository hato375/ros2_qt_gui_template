from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    params_file = LaunchConfiguration("params_file")

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "params_file",
                default_value=PathJoinSubstitution(
                    [
                        FindPackageShare("ros2_qt_gui"),
                        "config",
                        "ros2_qt_gui.yaml",
                    ]
                ),
                description="Path to the ROS 2 parameter YAML file.",
            ),
            Node(
                package="ros2_qt_gui",
                executable="ros2_qt_gui",
                name="ros2_qt_gui_node",
                output="screen",
                parameters=[params_file],
            )
        ]
    )
