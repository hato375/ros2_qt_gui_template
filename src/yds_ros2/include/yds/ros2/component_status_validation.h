#pragma once

#include <QtGlobal>
#include <QString>

#include <yds/ros2/component_status.h>

namespace yds::ros2 {

/// @brief コンポーネント状態値の検証結果
struct ComponentStatusValidationResult {
	/// @brief 状態値が定義済みの列挙値であるか
	bool validState;
	/// @brief 正常系状態に非ゼロのエラーコードが設定されているか
	bool unexpectedErrorCode;
	/// @brief 異常系状態にエラーコード0が設定されているか
	bool missingErrorCode;
	/// @brief 異常系状態に空のメッセージが設定されているか
	bool missingMessage;

	/// @brief 意味的な不整合があるかを取得する
	bool hasConsistencyWarning() const noexcept;
};

/// @brief ComponentStatusへ設定する状態、エラーコード、メッセージを検証する
ComponentStatusValidationResult validateComponentStatus(
	ComponentState state,
	qint32 errorCode,
	const QString& message);

}  // namespace yds::ros2
