#include <exception>
#include <memory>

#include <rclcpp/rclcpp.hpp>

#include <sample_processor/sample_processor_node.h>

int main(int argc, char* argv[]) {
	rclcpp::init(argc, argv);

	try {
		auto node = std::make_shared<sampleprocessor::SampleProcessorNode>();
		rclcpp::spin(node);
		rclcpp::shutdown();
		return 0;
	} catch (const std::exception& exception) {
		RCLCPP_FATAL(rclcpp::get_logger("sample_processor"), "%s", exception.what());
	} catch (...) {
		RCLCPP_FATAL(rclcpp::get_logger("sample_processor"), "Unknown exception");
	}

	if (rclcpp::ok()) {
		rclcpp::shutdown();
	}
	return 1;
}
