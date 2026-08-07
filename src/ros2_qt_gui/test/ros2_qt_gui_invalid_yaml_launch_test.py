import os
import unittest

import launch
import launch_ros.actions
import launch_testing
import launch_testing.actions
import launch_testing.asserts
import launch_testing.util


def generate_test_description():
    params_file = os.path.join(
        os.path.dirname(__file__),
        "config",
        "invalid_ros2_qt_gui.yaml",
    )
    empty_component_id = launch_ros.actions.Node(
        package="ros2_qt_gui",
        executable="ros2_qt_gui_configuration_probe",
        name="invalid_empty_component_id",
        output="screen",
        parameters=[params_file],
    )
    invalid_publish_interval = launch_ros.actions.Node(
        package="ros2_qt_gui",
        executable="ros2_qt_gui_configuration_probe",
        name="invalid_publish_interval",
        output="screen",
        parameters=[params_file],
    )
    duplicate_topic = launch_ros.actions.Node(
        package="ros2_qt_gui",
        executable="ros2_qt_gui_configuration_probe",
        name="invalid_duplicate_topic",
        output="screen",
        parameters=[params_file],
    )

    return (
        launch.LaunchDescription(
            [
                empty_component_id,
                invalid_publish_interval,
                duplicate_topic,
                launch_testing.util.KeepAliveProc(),
                launch_testing.actions.ReadyToTest(),
            ]
        ),
        {
            "empty_component_id": empty_component_id,
            "invalid_publish_interval": invalid_publish_interval,
            "duplicate_topic": duplicate_topic,
        },
    )


class TestInvalidYamlStopsStartup(unittest.TestCase):
    def test_all_invalid_configurations_stop(
        self,
        proc_info,
        empty_component_id,
        invalid_publish_interval,
        duplicate_topic,
    ):
        proc_info.assertWaitForShutdown(empty_component_id, timeout=10.0)
        proc_info.assertWaitForShutdown(invalid_publish_interval, timeout=10.0)
        proc_info.assertWaitForShutdown(duplicate_topic, timeout=10.0)


@launch_testing.post_shutdown_test()
class TestInvalidYamlErrors(unittest.TestCase):
    def test_empty_component_id_is_rejected(
        self, proc_info, proc_output, empty_component_id
    ):
        launch_testing.asserts.assertExitCodes(
            proc_info,
            allowable_exit_codes=[1],
            process=empty_component_id,
        )
        launch_testing.asserts.assertInStderr(
            proc_output,
            "component_status.component_id must not be empty",
            process=empty_component_id,
        )

    def test_invalid_publish_interval_is_rejected(
        self, proc_info, proc_output, invalid_publish_interval
    ):
        launch_testing.asserts.assertExitCodes(
            proc_info,
            allowable_exit_codes=[1],
            process=invalid_publish_interval,
        )
        launch_testing.asserts.assertInStderr(
            proc_output,
            "component_status.publish_interval_ms must be between 100 and 600000",
            process=invalid_publish_interval,
        )

    def test_duplicate_topic_is_rejected(
        self, proc_info, proc_output, duplicate_topic
    ):
        launch_testing.asserts.assertExitCodes(
            proc_info,
            allowable_exit_codes=[1],
            process=duplicate_topic,
        )
        launch_testing.asserts.assertInStderr(
            proc_output,
            "component_monitors.plc.status_topic resolves to '/camera/status', "
            "already used by component_monitors.camera.status_topic",
            process=duplicate_topic,
        )
