import unittest

import launch
import launch_ros.actions
import launch_ros.substitutions
import launch_testing
import launch_testing.actions
import launch_testing.asserts
import rclpy
from rcl_interfaces.srv import GetParameters
from rclpy.parameter import parameter_value_to_python
from launch.substitutions import PathJoinSubstitution


NODE_NAME = "ros2_qt_gui_node"


def generate_test_description():
    params_file = PathJoinSubstitution(
        [
            launch_ros.substitutions.FindPackageShare("ros2_qt_gui"),
            "config",
            "ros2_qt_gui.yaml",
        ]
    )
    gui_node = launch_ros.actions.Node(
        package="ros2_qt_gui",
        executable="ros2_qt_gui",
        name=NODE_NAME,
        output="screen",
        parameters=[params_file],
        additional_env={"QT_QPA_PLATFORM": "offscreen"},
    )

    return (
        launch.LaunchDescription(
            [
                gui_node,
                launch_testing.actions.ReadyToTest(),
            ]
        ),
        {"gui_node": gui_node},
    )


class TestRos2QtGuiYamlConfiguration(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        rclpy.init()
        cls.node = rclpy.create_node("ros2_qt_gui_yaml_launch_test")
        cls.parameter_client = cls.node.create_client(
            GetParameters,
            f"/{NODE_NAME}/get_parameters",
        )

    @classmethod
    def tearDownClass(cls):
        cls.node.destroy_client(cls.parameter_client)
        cls.node.destroy_node()
        rclpy.shutdown()

    def test_installed_yaml_values_are_applied(self):
        self.assertTrue(self.parameter_client.wait_for_service(timeout_sec=10.0))
        parameter_names = [
            "component_status.component_id",
            "component_status.status_topic",
            "component_status.publish_interval_ms",
            "heartbeat_interval_ms",
            "gui_status_check_interval_ms",
            "repeated_error_report_interval_ms",
            "component_monitor_names",
            "component_monitors.camera.expected_component_id",
            "component_monitors.camera.timeout_ms",
            "component_monitors.plc.expected_component_id",
            "component_monitors.plc.timeout_ms",
        ]
        request = GetParameters.Request()
        request.names = parameter_names
        future = self.parameter_client.call_async(request)
        rclpy.spin_until_future_complete(self.node, future, timeout_sec=10.0)

        self.assertTrue(future.done())
        self.assertIsNotNone(future.result())
        values = {
            name: parameter_value_to_python(value)
            for name, value in zip(parameter_names, future.result().values)
        }
        self.assertEqual(
            values["component_status.component_id"],
            "ros2-qt-gui-supervisor-1",
        )
        self.assertEqual(
            values["component_status.status_topic"],
            "ros2_qt_gui/status",
        )
        self.assertEqual(values["component_status.publish_interval_ms"], 1000)
        self.assertEqual(values["heartbeat_interval_ms"], 1000)
        self.assertEqual(values["gui_status_check_interval_ms"], 200)
        self.assertEqual(values["repeated_error_report_interval_ms"], 10000)
        self.assertEqual(values["component_monitor_names"], ["camera", "plc"])
        self.assertEqual(
            values["component_monitors.camera.expected_component_id"],
            "camera-1",
        )
        self.assertEqual(values["component_monitors.camera.timeout_ms"], 3000)
        self.assertEqual(
            values["component_monitors.plc.expected_component_id"],
            "plc-1",
        )
        self.assertEqual(values["component_monitors.plc.timeout_ms"], 5000)


@launch_testing.post_shutdown_test()
class TestRos2QtGuiShutdown(unittest.TestCase):
    def test_process_exits_successfully(self, proc_info, gui_node):
        launch_testing.asserts.assertExitCodes(proc_info, process=gui_node)
