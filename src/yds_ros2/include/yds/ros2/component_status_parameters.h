#pragma once

#include <rclcpp/node_interfaces/node_parameters_interface.hpp>

#include <yds/ros2/component_status_publisher.h>

namespace yds::ros2 {

/// @brief コンポーネント状態通知のROSパラメータを宣言して設定を取得する
/// @param nodeParameters パラメータを所有するNode Interface
/// @param defaults パラメータの既定値
/// @return ROSパラメータを反映したPublisher設定
ComponentStatusPublisherConfiguration declareComponentStatusPublisherParameters(
	const rclcpp::node_interfaces::NodeParametersInterface::SharedPtr& nodeParameters,
	const ComponentStatusPublisherConfiguration& defaults);

/// @brief Nodeからコンポーネント状態通知のROSパラメータを宣言して設定を取得する
/// @tparam NodeT `rclcpp::Node`または互換Node Interfaceを提供する型
/// @param node パラメータを所有するノード
/// @param defaults パラメータの既定値
/// @return ROSパラメータを反映したPublisher設定
template<typename NodeT>
ComponentStatusPublisherConfiguration declareComponentStatusPublisherParameters(
	NodeT& node,
	const ComponentStatusPublisherConfiguration& defaults) {
	return declareComponentStatusPublisherParameters(
		node.get_node_parameters_interface(),
		defaults);
}

}  // namespace yds::ros2
