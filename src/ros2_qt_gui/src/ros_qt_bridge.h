#pragma once

#include <cstdint>

#include <QObject>

#include <yds/ros2/application_event.h>
#include <yds/ros2/component_status.h>
#include <yds/ros2/topic_reception_status.h>

namespace ros2qtgui {

/// @brief ROS 2スレッドからQt GUIスレッドへ通知するブリッジ
class RosQtBridge final : public QObject {
	Q_OBJECT

public:
	/// @brief ブリッジを生成する
	/// @param parent 親Qtオブジェクト
	explicit RosQtBridge(QObject* parent = nullptr);

	/// @brief ROS 2のハートビート更新を通知する
	/// @param count ハートビート回数
	void notifyHeartbeat(std::uint64_t count) noexcept;

	/// @brief アプリケーションイベントを通知する
	/// @param event 通知するイベント
	void notifyApplicationEvent(const yds::ros2::ApplicationEvent& event) noexcept;

	/// @brief ROSトピックの受信状況を通知する
	/// @param status 通知する受信状況
	void notifyTopicReceptionStatus(
		const yds::ros2::TopicReceptionStatus& status) noexcept;

	/// @brief コンポーネント状態を通知する
	/// @param status 通知するコンポーネント状態
	void notifyComponentStatus(const yds::ros2::ComponentStatus& status) noexcept;

signals:
	/// @brief ハートビートが更新されたときに送出される
	/// @param count ハートビート回数
	void heartbeatUpdated(quint64 count);

	/// @brief アプリケーションイベントが発生したときに送出される
	/// @param event 発生したイベント
	void applicationEventOccurred(const yds::ros2::ApplicationEvent& event);

	/// @brief ROSトピックの受信状況が更新されたときに送出される
	/// @param status 更新された受信状況
	void topicReceptionStatusUpdated(const yds::ros2::TopicReceptionStatus& status);

	/// @brief コンポーネント状態が更新されたときに送出される
	/// @param status 更新されたコンポーネント状態
	void componentStatusUpdated(const yds::ros2::ComponentStatus& status);
};

}  // namespace ros2qtgui
