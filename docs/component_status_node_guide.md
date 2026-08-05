# コンポーネントステータスPublish共通基底ノード 利用ガイド

## 1. 目的

`yds::ros2::ComponentStatusPublisher`は、camera、PLCなどの物理設備や、画像処理、点群生成などの
ロジック機能を実装するノードから`yds_interfaces/msg/ComponentStatus`を共通の方法で通知します。
通常の`rclcpp::Node`では、便利な基底クラス`yds::ros2::ComponentStatusNode`を利用できます。

このクラスは次の処理を提供します。

- 状態変更時の即時Publish
- 最新状態の定期Publish
- 初期状態`INITIALIZING`のPublish
- コンポーネントID、トピック名、通知周期のROSパラメータ対応
- 複数スレッドからの状態更新に対する排他制御
- 最新状態を保持するReliable、Transient Local QoS

コンポーネント固有の初期化、復旧処理、安全停止判断は利用するノードの責務です。

状態通知の実装本体は`ComponentStatusPublisher`へ分離されています。Node Interfaceを介してPublisherと
Timerを登録するため、`rclcpp::Node`を継承済みのクラスやLifecycle Nodeを採用するクラスでも
同じ状態管理処理を再利用できます。Lifecycle状態とコンポーネント状態は統合せず、別の軸として扱います。

## 2. 派生ノードの実装例

```cpp
#include <chrono>

#include <QString>

#include <yds/ros2/component_status_node.h>

class CameraNode final : public yds::ros2::ComponentStatusNode {
public:
	explicit CameraNode(const rclcpp::NodeOptions& options = rclcpp::NodeOptions())
		: ComponentStatusNode(
			"camera_node",
			QStringLiteral("camera-1"),
			QStringLiteral("camera/status"),
			std::chrono::milliseconds(1000),
			options) {}

	void notifyReady() {
		setComponentStatus(yds::ros2::ComponentState::kReady);
	}

	void notifyError(qint32 errorCode, const QString& message) {
		setComponentStatus(yds::ros2::ComponentState::kError, errorCode, message);
	}
};
```

`setComponentStatus()`は内部状態を更新した直後にPublishします。戻り値が`false`の場合はPublishに
失敗しています。対象の安全状態を確認し、必要な停止処理や上位通知を派生ノード側で実行してください。

状態が変わらない間も最新状態を設定周期で再Publishします。この定期通知をSupervisor側の
`TopicReceptionMonitor`で監視すると、コンポーネント状態と通信タイムアウトを別々に判定できます。

## 3. ROSパラメータ

次のパラメータはノード起動時に確定し、実行中は変更できません。

| パラメータ | 範囲 | 用途 |
|---|---:|---|
| `component_status.component_id` | 空文字不可 | メッセージへ設定するコンポーネントID |
| `component_status.status_topic` | 空文字不可 | Publish先のトピック名 |
| `component_status.publish_interval_ms` | 100～600000 | 最新状態の定期通知周期 |

YAMLによる設定例です。

```yaml
camera_node:
  ros__parameters:
    component_status:
      component_id: camera-1
      status_topic: camera/status
      publish_interval_ms: 1000
```

Supervisorの受信タイムアウトは、ネットワーク遅延や処理遅延を考慮し、Publish周期より十分長く
設定してください。例えばPublish周期が1000ミリ秒の場合、3000ミリ秒程度から調整します。

## 4. 既存のNodeへ機能を追加する

すでに別の基底クラスを継承している場合は、`ComponentStatusPublisher`をメンバーとして所有します。
この利用方法では、設定値の検証とROSパラメータの宣言は所有するノードの責務です。

```cpp
#include <chrono>
#include <memory>

#include <yds/ros2/component_status_publisher.h>

class CircleDetectorNode final : public rclcpp::Node {
public:
	CircleDetectorNode()
		: rclcpp::Node("circle_detector_node"),
		  statusPublisher_(std::make_unique<yds::ros2::ComponentStatusPublisher>(
			  *this,
			  yds::ros2::ComponentStatusPublisherConfiguration{
				  QStringLiteral("circle-detector-1"),
				  QStringLiteral("circle_detector/status"),
				  std::chrono::milliseconds(1000)})) {}

	void notifyRunning() {
		statusPublisher_->setStatus(yds::ros2::ComponentState::kRunning);
	}

private:
	std::unique_ptr<yds::ros2::ComponentStatusPublisher> statusPublisher_;
};
```

## 5. 状態とエラーコード

- 正常な状態では`errorCode`を`0`にします。
- `WARNING`、`ERROR`、`CRITICAL`では、設備ごとに定義したエラーコードと運用者向けメッセージを設定します。
- 復旧時は状態だけでなく、古いエラーコードとメッセージも正常値へ戻します。
- `CRITICAL`を通知するだけでは設備は停止しません。安全停止は設備固有処理として必ず実装します。

```cpp
setComponentStatus(yds::ros2::ComponentState::kCritical, 2001, QStringLiteral("Emergency stop"));

// 復旧後
setComponentStatus(yds::ros2::ComponentState::kReady, 0, QString());
```

## 6. Lifecycle Nodeへ組み込む

Lifecycle Nodeでは、`ComponentStatusNode`を継承せず、ROS 2標準の
`rclcpp_lifecycle::LifecycleNode`を継承します。`ComponentStatusPublisher`はメンバーとして所有します。

```cpp
#include <chrono>
#include <memory>

#include <rclcpp_lifecycle/lifecycle_node.hpp>
#include <yds/ros2/component_status_parameters.h>
#include <yds/ros2/component_status_publisher.h>

class LifecycleCameraNode final : public rclcpp_lifecycle::LifecycleNode {
public:
	LifecycleCameraNode()
		: rclcpp_lifecycle::LifecycleNode("camera_node"),
		  statusPublisher_(std::make_unique<yds::ros2::ComponentStatusPublisher>(
			  *this,
			  yds::ros2::declareComponentStatusPublisherParameters(
				  *this,
				  yds::ros2::ComponentStatusPublisherConfiguration{
					  QStringLiteral("camera-1"),
					  QStringLiteral("camera/status"),
					  std::chrono::milliseconds(1000)}))) {}

private:
	std::unique_ptr<yds::ros2::ComponentStatusPublisher> statusPublisher_;
};
```

`ComponentStatusPublisher`は通常のPublisherとTimerを使用し、Lifecycle Nodeが`INACTIVE`の間も
最新状態を定期通知します。これにより、Supervisorは正常にInactiveであるノードと、ノード停止や
通信断を区別できます。Inactive時のコンポーネント状態は、設備の実態に応じて`READY`や`STOPPED`などを
Lifecycleコールバックから明示的に設定してください。

Lifecycle状態からComponentStatusへの自動変換は行いません。Lifecycle状態はノードの管理段階、
ComponentStatusは設備またはロジック機能の業務状態であり、必ずしも一対一に対応しないためです。
Lifecycle Nodeを実装するパッケージは、`yds_ros2`に加えて`rclcpp_lifecycle`へ依存してください。

`declareComponentStatusPublisherParameters()`は通常ノードとLifecycle Nodeのどちらでも利用でき、
3章のROSパラメータを読み取り専用として宣言します。戻り値はパラメータoverrideを反映済みの
`ComponentStatusPublisherConfiguration`です。これにより、ノード種別が異なっても同じYAML設定と
入力値検証を使用できます。設定値をコード内で固定する場合は、この関数を使わず、従来どおり
`ComponentStatusPublisherConfiguration`をPublisherへ直接渡すこともできます。

## 7. QoS

Publisherは`KeepLast(1)`、`Reliable`、`Transient Local`を使用します。監視ノードが同じQoSで
Subscribeすると、設備ノードより後に起動した場合も、保持された最新状態を受信できます。
定期Publishも行うため、一時的な切断から復旧した後は最新状態を再受信できます。
