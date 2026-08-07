#include <chrono>
#include <memory>

#include <gtest/gtest.h>

#include <rclcpp/rclcpp.hpp>

#include <sample_processor/sample_lifecycle_processor_error_codes.h>
#include <sample_processor/sample_lifecycle_processor_node.h>
#include <yds/ros2/component_status_node.h>
#include <yds/ros2/repeated_event_rate_limiter.h>

namespace {

class ExternalLifecycleProcessorNode final
	: public sampleprocessor::SampleLifecycleProcessorNode {
public:
	using SampleLifecycleProcessorNode::SampleLifecycleProcessorNode;

protected:
	bool configureProcessor(QString& errorMessage) override {
		errorMessage = QStringLiteral("External processor configuration failed");
		return false;
	}
};

class ExternalRegularComponentStatusNode final
	: public yds::ros2::ComponentStatusNode {
public:
	ExternalRegularComponentStatusNode()
		: ComponentStatusNode(
			"external_regular_component_status_node",
			QStringLiteral("external-regular-1"),
			QStringLiteral("external_regular/status")) {}
};

TEST(SampleProcessorPublicApiTest, DerivesFromInstalledRegularStatusNode) {
	auto node = std::make_shared<ExternalRegularComponentStatusNode>();

	EXPECT_TRUE(node->setComponentStatus(
		yds::ros2::ComponentState::kReady,
		0,
		QStringLiteral("External regular node ready")));

	EXPECT_EQ(node->componentStatus().state, yds::ros2::ComponentState::kReady);
	EXPECT_EQ(node->componentStatus().errorCode, 0);
	EXPECT_EQ(
		node->componentStatus().message,
		QStringLiteral("External regular node ready"));
	EXPECT_EQ(node->componentId(), QStringLiteral("external-regular-1"));
}

TEST(SampleProcessorPublicApiTest, DerivesFromInstalledLifecycleNode) {
	auto node = std::make_shared<ExternalLifecycleProcessorNode>();

	node->configure();

	EXPECT_EQ(node->get_current_state().label(), "unconfigured");
	EXPECT_EQ(node->componentStatus().state, yds::ros2::ComponentState::kError);
	EXPECT_EQ(
		node->componentStatus().errorCode,
		sampleprocessor::lifecycle_error_code::kConfiguration);
	EXPECT_EQ(
		node->componentStatus().message,
		QStringLiteral("External processor configuration failed"));
}

TEST(SampleProcessorPublicApiTest, UsesInstalledRepeatedEventRateLimiter) {
	using namespace std::chrono_literals;
	yds::ros2::RepeatedEventRateLimiter limiter(10s);
	const auto start = yds::ros2::RepeatedEventRateLimiter::Clock::time_point(100s);

	EXPECT_TRUE(limiter.record(start).shouldReport);
	EXPECT_FALSE(limiter.record(start + 1s).shouldReport);
}

}  // namespace

int main(int argc, char* argv[]) {
	testing::InitGoogleTest(&argc, argv);

	int rosArgumentCount = 0;
	char** rosArguments = nullptr;
	rclcpp::init(rosArgumentCount, rosArguments);
	const int result = RUN_ALL_TESTS();
	rclcpp::shutdown();
	return result;
}
