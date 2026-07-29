#pragma once

#include <cstdint>

#include <QObject>

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

signals:
	/// @brief ハートビートが更新されたときに送出される
	/// @param count ハートビート回数
	void heartbeatUpdated(quint64 count);
};

}  // namespace ros2qtgui
