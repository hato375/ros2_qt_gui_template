#pragma once

#include <QMainWindow>

#include "application_event.h"

class QLabel;
class QPlainTextEdit;
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

	/// @brief アプリケーションイベントをログ表示へ追加する
	/// @param event 表示するイベント
	void appendApplicationEvent(const ApplicationEvent& event) noexcept;

private:
	void updateRosStatus() noexcept;

	QLabel* statusLabel_;
	QLabel* heartbeatLabel_;
	QPlainTextEdit* eventLog_;
	QTimer* statusTimer_;
};

}  // namespace ros2qtgui
