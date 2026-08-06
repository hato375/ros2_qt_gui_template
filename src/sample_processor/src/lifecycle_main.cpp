#include <exception>
#include <memory>

#include <rclcpp/rclcpp.hpp>

#include <sample_processor/sample_lifecycle_processor_node.h>

int main(int argc, char* argv[]) {
	rclcpp::init(argc, argv);

	try {
		auto node = std::make_shared<sampleprocessor::SampleLifecycleProcessorNode>();
		rclcpp::executors::SingleThreadedExecutor executor;
		executor.add_node(node->get_node_base_interface());
		executor.spin();
		rclcpp::shutdown();
		return 0;
	} catch (const std::exception& exception) {
		RCLCPP_FATAL(
			rclcpp::get_logger("sample_lifecycle_processor"),
			"%s",
			exception.what());
	} catch (...) {
		RCLCPP_FATAL(
			rclcpp::get_logger("sample_lifecycle_processor"),
			"Unknown exception");
	}

	if (rclcpp::ok()) {
		rclcpp::shutdown();
	}
	return 1;
}
