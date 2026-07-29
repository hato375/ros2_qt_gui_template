#pragma once

#include <QMainWindow>

class QLabel;
class QTimer;

namespace ros2qtgui {

/// @brief ROS 2ノードの状態を表示するメインウィンドウ
class MainWindow final : public QMainWindow {
public:
	/// @brief メインウィンドウを生成する
	/// @param statusCheckIntervalMs ROS 2状態を確認する周期（ミリ秒）
	explicit MainWindow(int statusCheckIntervalMs);

	/// @brief ROS 2のハートビート表示を更新する
	/// @param count ハートビート回数
	void setHeartbeatCount(quint64 count) noexcept;

private:
	void updateRosStatus() noexcept;

	QLabel* statusLabel_;
	QLabel* heartbeatLabel_;
	QTimer* statusTimer_;
};

}  // namespace ros2qtgui
