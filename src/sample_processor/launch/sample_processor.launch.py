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
                    [FindPackageShare("sample_processor"), "config", "sample_processor.yaml"]
                ),
                description="Path to the sample processor parameter YAML file.",
            ),
            Node(
                package="sample_processor",
                executable="sample_processor",
                name="sample_processor_node",
                output="screen",
                parameters=[params_file],
            ),
        ]
    )
