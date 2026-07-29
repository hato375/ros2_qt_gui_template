from launch import LaunchDescription
from launch_ros.actions import Node


def generate_launch_description():
    return LaunchDescription(
        [
            Node(
                package="ros2_qt_gui",
                executable="ros2_qt_gui",
                name="ros2_qt_gui_node",
                output="screen",
            )
        ]
    )
