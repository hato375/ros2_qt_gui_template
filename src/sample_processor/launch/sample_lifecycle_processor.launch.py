from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument
from launch.substitutions import LaunchConfiguration, PathJoinSubstitution
from launch_ros.actions import LifecycleNode
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    params_file = LaunchConfiguration("params_file")

    return LaunchDescription(
        [
            DeclareLaunchArgument(
                "params_file",
                default_value=PathJoinSubstitution(
                    [
                        FindPackageShare("sample_processor"),
                        "config",
                        "sample_lifecycle_processor.yaml",
                    ]
                ),
                description="Path to the lifecycle processor parameter YAML file.",
            ),
            LifecycleNode(
                package="sample_processor",
                executable="sample_lifecycle_processor",
                name="sample_lifecycle_processor_node",
                namespace="",
                output="screen",
                parameters=[params_file],
            ),
        ]
    )
