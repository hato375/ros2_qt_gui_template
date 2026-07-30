#pragma once

#include <QDateTime>
#include <QMetaType>
#include <QString>
#include <QtGlobal>

namespace yds::ros2 {

/// @brief ROSトピックの受信状態
enum class TopicReceptionState {
	kWaiting,
	kReceiving,
	kTimedOut,
};

/// @brief ROSトピックの受信状況を表すデータ
struct TopicReceptionStatus {
	QString topicName;
	TopicReceptionState state;
	QDateTime lastReceivedAt;
	quint64 receivedCount;
	QString lastMessage;
};

/// @brief ROSトピックの受信状態を文字列へ変換する
/// @param state 変換する受信状態
/// @return 受信状態を表す文字列
inline QString topicReceptionStateText(TopicReceptionState state) noexcept {
	switch (state) {
	case TopicReceptionState::kWaiting:
		return QStringLiteral("WAITING");
	case TopicReceptionState::kReceiving:
		return QStringLiteral("RECEIVING");
	case TopicReceptionState::kTimedOut:
		return QStringLiteral("TIMED OUT");
	}
	return QStringLiteral("UNKNOWN");
}

}  // namespace yds::ros2

Q_DECLARE_METATYPE(yds::ros2::TopicReceptionStatus)
