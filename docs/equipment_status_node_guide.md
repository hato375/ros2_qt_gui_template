# 設備ステータスPublish共通基底ノード 利用ガイド

## 1. 目的

`yds::ros2::EquipmentStatusNode`は、camera、PLCなどの設備ノードから
`yds_interfaces/msg/EquipmentStatus`を共通の方法で通知するための基底クラスです。

このクラスは次の処理を提供します。

- 状態変更時の即時Publish
- 最新状態の定期Publish
- 初期状態`INITIALIZING`のPublish
- 設備ID、トピック名、通知周期のROSパラメータ対応
- 複数スレッドからの状態更新に対する排他制御
- 最新状態を保持するReliable、Transient Local QoS

設備固有の初期化、復旧処理、安全停止判断は派生ノードの責務です。

## 2. 派生ノードの実装例

```cpp
#include <chrono>

#include <QString>

#include <yds/ros2/equipment_status_node.h>

class CameraNode final : public yds::ros2::EquipmentStatusNode {
public:
	explicit CameraNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions())
		: EquipmentStatusNode(
			"camera_node",
			QStringLiteral("camera-1"),
			QStringLiteral("camera/status"),
			std::chrono::milliseconds(1000),
			options) {}

	void notifyReady() {
		setEquipmentStatus(yds::ros2::EquipmentState::kReady);
	}

	void notifyError(qint32 errorCode, const QString& message) {
		setEquipmentStatus(yds::ros2::EquipmentState::kError, errorCode, message);
	}
};
```

`setEquipmentStatus()`は内部状態を更新した直後にPublishします。戻り値が`false`の場合はPublishに
失敗しています。設備の安全状態を確認し、必要な停止処理や上位通知を派生ノード側で実行してください。

状態が変わらない間も最新状態を設定周期で再Publishします。この定期通知をSupervisor側の
`TopicReceptionMonitor`で監視すると、設備状態と通信タイムアウトを別々に判定できます。

## 3. ROSパラメータ

次のパラメータはノード起動時に確定し、実行中は変更できません。

| パラメータ | 範囲 | 用途 |
|---|---:|---|
| `equipment_status.equipment_id` | 空文字不可 | メッセージへ設定する設備ID |
| `equipment_status.topic_name` | 空文字不可 | Publish先のトピック名 |
| `equipment_status.publish_interval_ms` | 100～600000 | 最新状態の定期通知周期 |

YAMLによる設定例です。

```yaml
camera_node:
  ros__parameters:
    equipment_status:
      equipment_id: camera-1
      topic_name: camera/status
      publish_interval_ms: 1000
```

Supervisorの受信タイムアウトは、ネットワーク遅延や処理遅延を考慮し、Publish周期より十分長く
設定してください。例えばPublish周期が1000ミリ秒の場合、3000ミリ秒程度から調整します。

## 4. 状態とエラーコード

- 正常な状態では`errorCode`を`0`にします。
- `WARNING`、`ERROR`、`CRITICAL`では、設備ごとに定義したエラーコードと運用者向けメッセージを設定します。
- 復旧時は状態だけでなく、古いエラーコードとメッセージも正常値へ戻します。
- `CRITICAL`を通知するだけでは設備は停止しません。安全停止は設備固有処理として必ず実装します。

```cpp
setEquipmentStatus(yds::ros2::EquipmentState::kCritical, 2001, QStringLiteral("Emergency stop"));

// 復旧後
setEquipmentStatus(yds::ros2::EquipmentState::kReady, 0, QString());
```

## 5. QoS

Publisherは`KeepLast(1)`、`Reliable`、`Transient Local`を使用します。監視ノードが同じQoSで
Subscribeすると、設備ノードより後に起動した場合も、保持された最新状態を受信できます。
定期Publishも行うため、一時的な切断から復旧した後は最新状態を再受信できます。
