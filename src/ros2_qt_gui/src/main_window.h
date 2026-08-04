#pragma once

#include <QMainWindow>

#include <yds/ros2/application_event.h>
#include <yds/ros2/component_status.h>
#include <yds/ros2/topic_reception_status.h>

class QLabel;
class QPlainTextEdit;
class QTableWidget;
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
	void appendApplicationEvent(const yds::ros2::ApplicationEvent& event) noexcept;

	/// @brief 監視トピックに対応するコンポーネント表示名を設定する
	/// @param topicName 監視するROSトピック名
	/// @param displayName GUIに表示するコンポーネント名
	void setComponentDisplayName(
		const QString& topicName,
		const QString& displayName) noexcept;

	/// @brief ROSトピックの受信状況を更新する
	/// @param status 表示する受信状況
	void setTopicReceptionStatus(
		const yds::ros2::TopicReceptionStatus& status) noexcept;

	/// @brief コンポーネント状態を更新する
	/// @param status 表示するコンポーネント状態
	void setComponentStatus(const yds::ros2::ComponentStatus& status) noexcept;

private:
	int findOrCreateTopicRow(const QString& topicName) noexcept;
	void updateRosStatus() noexcept;

	QLabel* statusLabel_;
	QLabel* heartbeatLabel_;
	QTableWidget* topicStatusTable_;
	QPlainTextEdit* eventLog_;
	QTimer* statusTimer_;
};

}  // namespace ros2qtgui
