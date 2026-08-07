import time
import unittest

import launch
from launch.actions import IncludeLaunchDescription, SetEnvironmentVariable
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import PathJoinSubstitution
import launch_ros.substitutions
import launch_testing
import launch_testing.actions
import launch_testing.asserts
import rclpy
from rcl_interfaces.srv import GetParameters
from rclpy.parameter import parameter_value_to_python
from rclpy.qos import DurabilityPolicy, QoSProfile, ReliabilityPolicy
from yds_interfaces.msg import ComponentStatus


GUI_NODE_NAME = "ros2_qt_gui_node"
STATUS_TOPIC = "/sample_lifecycle_processor/status"


def generate_test_description():
    demo_launch = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            PathJoinSubstitution(
                [
                    launch_ros.substitutions.FindPackageShare("sample_processor"),
                    "launch",
                    "lifecycle_component_status_demo.launch.py",
                ]
            )
        )
    )
    return launch.LaunchDescription(
        [
            SetEnvironmentVariable("QT_QPA_PLATFORM", "offscreen"),
            demo_launch,
            launch_testing.actions.ReadyToTest(),
        ]
    )


class TestLifecycleComponentStatusDemo(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        rclpy.init()
        cls.node = rclpy.create_node("lifecycle_component_status_demo_launch_test")
        cls.latest_status = None
        status_qos = QoSProfile(
            depth=1,
            reliability=ReliabilityPolicy.RELIABLE,
            durability=DurabilityPolicy.TRANSIENT_LOCAL,
        )
        cls.status_subscription = cls.node.create_subscription(
            ComponentStatus,
            STATUS_TOPIC,
            cls._on_status,
            status_qos,
        )
        cls.parameter_client = cls.node.create_client(
            GetParameters,
            f"/{GUI_NODE_NAME}/get_parameters",
        )

    @classmethod
    def tearDownClass(cls):
        cls.node.destroy_subscription(cls.status_subscription)
        cls.node.destroy_client(cls.parameter_client)
        cls.node.destroy_node()
        rclpy.shutdown()

    @classmethod
    def _on_status(cls, message):
        cls.latest_status = message

    def test_lifecycle_status_matches_gui_yaml(self):
        deadline = time.monotonic() + 10.0
        while time.monotonic() < deadline and self.latest_status is None:
            rclpy.spin_once(self.node, timeout_sec=0.1)
        self.assertIsNotNone(self.latest_status)
        self.assertEqual(
            self.latest_status.component_id,
            "sample-lifecycle-processor-1",
        )
        self.assertEqual(
            self.latest_status.state,
            ComponentStatus.STATE_INITIALIZING,
        )

        self.assertTrue(self.parameter_client.wait_for_service(timeout_sec=10.0))
        names = [
            "component_monitor_names",
            "component_monitors.sample_lifecycle_processor.status_topic",
            "component_monitors.sample_lifecycle_processor.expected_component_id",
            "repeated_error_report_interval_ms",
        ]
        request = GetParameters.Request()
        request.names = names
        future = self.parameter_client.call_async(request)
        rclpy.spin_until_future_complete(self.node, future, timeout_sec=10.0)
        self.assertTrue(future.done())
        self.assertIsNotNone(future.result())
        values = {
            name: parameter_value_to_python(value)
            for name, value in zip(names, future.result().values)
        }
        self.assertEqual(
            values["component_monitor_names"],
            ["sample_lifecycle_processor"],
        )
        self.assertEqual(
            values["component_monitors.sample_lifecycle_processor.status_topic"],
            "sample_lifecycle_processor/status",
        )
        self.assertEqual(
            values[
                "component_monitors.sample_lifecycle_processor.expected_component_id"
            ],
            self.latest_status.component_id,
        )
        self.assertEqual(values["repeated_error_report_interval_ms"], 10000)


@launch_testing.post_shutdown_test()
class TestLifecycleComponentStatusDemoShutdown(unittest.TestCase):
    def test_processes_exit_successfully(self, proc_info):
        launch_testing.asserts.assertExitCodes(proc_info)
