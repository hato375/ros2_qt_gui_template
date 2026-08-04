#pragma once

#include <QMainWindow>

#include <yds/ros2/application_event.h>
#include <yds/ros2/widgets/component_monitor_dialog.h>

class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTimer;

namespace ros2qtgui {

/// @brief ROS 2ノードの概要状態を表示するメインウィンドウ
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
	void appendApplicationEvent(const yds::ros2::ApplicationEvent& event) noexcept;

	/// @brief コンポーネント監視ダイアログを取得する
	/// @return このウィンドウが所有する監視ダイアログ
	yds::ros2::widgets::ComponentMonitorDialog& componentMonitorDialog() noexcept;

	/// @brief 監視トピックに対応するコンポーネント表示名を設定する
	void setComponentDisplayName(
		const QString& topicName,
		const QString& displayName) noexcept;

	/// @brief ROSトピックの受信状況を監視ダイアログへ反映する
	void setTopicReceptionStatus(const yds::ros2::TopicReceptionStatus& status) noexcept;

	/// @brief コンポーネント状態を監視ダイアログへ反映する
	void setComponentStatus(const yds::ros2::ComponentStatus& status) noexcept;

	/// @brief 全監視対象の集約状態を更新する
	/// @param status 集約状態
	/// @param receivingCount 受信中の監視対象数
	/// @param totalCount 全監視対象数
	void setOverallStatus(
		yds::ros2::widgets::ComponentMonitorDialog::OverallStatus status,
		int receivingCount,
		int totalCount) noexcept;

private:
	void updateRosStatus() noexcept;

	QLabel* statusLabel_;
	QLabel* overallStatusLabel_;
	QLabel* heartbeatLabel_;
	QPushButton* showComponentMonitorButton_;
	QPlainTextEdit* eventLog_;
	QTimer* statusTimer_;
	yds::ros2::widgets::ComponentMonitorDialog* componentMonitorDialog_;
};

}  // namespace ros2qtgui
