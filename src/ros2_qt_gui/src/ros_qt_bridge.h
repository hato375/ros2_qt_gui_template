#pragma once

#include <cstdint>

#include <QObject>

#include "application_event.h"

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
	void notifyApplicationEvent(const ApplicationEvent& event) noexcept;

signals:
	/// @brief ハートビートが更新されたときに送出される
	/// @param count ハートビート回数
	void heartbeatUpdated(quint64 count);

	/// @brief アプリケーションイベントが発生したときに送出される
	/// @param event 発生したイベント
	void applicationEventOccurred(const ApplicationEvent& event);
};

}  // namespace ros2qtgui
