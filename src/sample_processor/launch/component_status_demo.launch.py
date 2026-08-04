from launch import LaunchDescription
from launch.substitutions import PathJoinSubstitution
from launch_ros.actions import Node
from launch_ros.substitutions import FindPackageShare


def generate_launch_description():
    sample_package = FindPackageShare("sample_processor")

    return LaunchDescription(
        [
            Node(
                package="sample_processor",
                executable="sample_processor",
                name="sample_processor_node",
                output="screen",
                parameters=[
                    PathJoinSubstitution(
                        [sample_package, "config", "sample_processor.yaml"]
                    )
                ],
            ),
            Node(
                package="ros2_qt_gui",
                executable="ros2_qt_gui",
                name="ros2_qt_gui_node",
                output="screen",
                parameters=[
                    PathJoinSubstitution(
                        [sample_package, "config", "component_status_demo_gui.yaml"]
                    )
                ],
            ),
        ]
    )
