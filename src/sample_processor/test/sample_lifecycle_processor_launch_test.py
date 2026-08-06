import time
import unittest

import launch
import launch_ros.actions
import launch_testing
import launch_testing.actions
import launch_testing.asserts
import rclpy
from lifecycle_msgs.msg import State, Transition
from lifecycle_msgs.srv import ChangeState, GetState
from rclpy.qos import DurabilityPolicy, QoSProfile, ReliabilityPolicy
from yds_interfaces.msg import ComponentStatus


NODE_NAME = "sample_lifecycle_processor_launch_test_node"
STATUS_TOPIC = "/sample_lifecycle_processor_launch_test/status"


def generate_test_description():
    lifecycle_processor = launch_ros.actions.LifecycleNode(
        package="sample_processor",
        executable="sample_lifecycle_processor",
        name=NODE_NAME,
        namespace="",
        output="screen",
        parameters=[
            {
                "processing_interval_ms": 100,
                "component_status.component_id": "lifecycle-launch-test",
                "component_status.status_topic": STATUS_TOPIC,
                "component_status.publish_interval_ms": 100,
            }
        ],
    )

    return (
        launch.LaunchDescription(
            [
                lifecycle_processor,
                launch_testing.actions.ReadyToTest(),
            ]
        ),
        {"lifecycle_processor": lifecycle_processor},
    )


class TestLifecycleProcessorProcess(unittest.TestCase):
    @classmethod
    def setUpClass(cls):
        rclpy.init()
        cls.node = rclpy.create_node("sample_lifecycle_processor_launch_test")
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
        cls.change_state_client = cls.node.create_client(
            ChangeState,
            f"/{NODE_NAME}/change_state",
        )
        cls.get_state_client = cls.node.create_client(
            GetState,
            f"/{NODE_NAME}/get_state",
        )

    @classmethod
    def tearDownClass(cls):
        cls.node.destroy_subscription(cls.status_subscription)
        cls.node.destroy_client(cls.change_state_client)
        cls.node.destroy_client(cls.get_state_client)
        cls.node.destroy_node()
        rclpy.shutdown()

    @classmethod
    def _on_status(cls, message):
        cls.latest_status = message

    def _wait_for_services(self):
        self.assertTrue(self.change_state_client.wait_for_service(timeout_sec=10.0))
        self.assertTrue(self.get_state_client.wait_for_service(timeout_sec=10.0))

    def _change_state(self, transition_id):
        request = ChangeState.Request()
        request.transition.id = transition_id
        future = self.change_state_client.call_async(request)
        rclpy.spin_until_future_complete(self.node, future, timeout_sec=10.0)
        self.assertTrue(future.done())
        self.assertIsNotNone(future.result())
        self.assertTrue(future.result().success)

    def _current_state(self):
        future = self.get_state_client.call_async(GetState.Request())
        rclpy.spin_until_future_complete(self.node, future, timeout_sec=10.0)
        self.assertTrue(future.done())
        self.assertIsNotNone(future.result())
        return future.result().current_state.id

    def _wait_for_component_state(self, expected_state):
        deadline = time.monotonic() + 10.0
        while time.monotonic() < deadline:
            rclpy.spin_once(self.node, timeout_sec=0.1)
            if (
                self.latest_status is not None
                and self.latest_status.state == expected_state
            ):
                return self.latest_status
        self.fail(f"ComponentStatus state {expected_state} was not received")

    def test_lifecycle_transitions_and_status_topic(self):
        self._wait_for_services()
        self.assertEqual(self._current_state(), State.PRIMARY_STATE_UNCONFIGURED)
        initial_status = self._wait_for_component_state(
            ComponentStatus.STATE_INITIALIZING
        )
        self.assertEqual(initial_status.component_id, "lifecycle-launch-test")

        self._change_state(Transition.TRANSITION_CONFIGURE)
        self.assertEqual(self._current_state(), State.PRIMARY_STATE_INACTIVE)
        ready_status = self._wait_for_component_state(ComponentStatus.STATE_READY)
        self.assertEqual(ready_status.error_code, 0)

        self._change_state(Transition.TRANSITION_ACTIVATE)
        self.assertEqual(self._current_state(), State.PRIMARY_STATE_ACTIVE)
        running_status = self._wait_for_component_state(ComponentStatus.STATE_RUNNING)
        self.assertEqual(running_status.error_code, 0)

        self._change_state(Transition.TRANSITION_ACTIVE_SHUTDOWN)
        self.assertEqual(self._current_state(), State.PRIMARY_STATE_FINALIZED)
        stopped_status = self._wait_for_component_state(ComponentStatus.STATE_STOPPED)
        self.assertEqual(stopped_status.error_code, 0)


@launch_testing.post_shutdown_test()
class TestLifecycleProcessorShutdown(unittest.TestCase):
    def test_process_exits_successfully(self, proc_info, lifecycle_processor):
        launch_testing.asserts.assertExitCodes(
            proc_info,
            process=lifecycle_processor,
        )
