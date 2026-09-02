# コンポーネントステータスPublish共通基底ノード 利用ガイド

## 1. 目的

`yds::ros2::ComponentStatusPublisher`は、camera、PLCなどの物理設備や、画像処理、点群生成などの
ロジック機能を実装するノードから`yds_interfaces/msg/ComponentStatus`を共通の方法で通知します。
通常の`rclcpp::Node`では`yds::ros2::ComponentStatusNode`、Lifecycle Nodeでは
`yds::ros2::LifecycleComponentStatusNode`を便利な基底クラスとして利用できます。

これらのクラスは次の処理を提供します。

- 状態変更時の即時Publish
- 最新状態の定期Publish
- 初期状態`INITIALIZING`のPublish
- コンポーネントID、トピック名、通知周期のROSパラメータ対応
- 複数スレッドからの状態更新に対する排他制御
- 最新状態を保持するReliable、Transient Local QoS

コンポーネント固有の初期化、復旧処理、安全停止判断は利用するノードの責務です。

状態通知の実装本体は`ComponentStatusPublisher`へ分離されています。両基底クラスはノード種別に固有の
構築を吸収し、同じ状態通知APIを提供します。別の基底クラスを継承済みのクラスでも、Publisherをメンバー
として所有すれば同じ状態管理処理を再利用できます。Lifecycle状態とコンポーネント状態は統合せず、
別の軸として扱います。

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

### 5.1 値の検証ポリシー

`ComponentStatusPublisher`は定義外の`ComponentState`を拒否し、現在状態を変更せず`false`を返します。
一方、次の意味的な不整合は警告ログを出したうえで、指定された値をそのままPublishします。

- `INITIALIZING`、`READY`、`RUNNING`、`STOPPED`で`errorCode`が0以外
- `WARNING`、`ERROR`、`CRITICAL`で`errorCode`が0
- `WARNING`、`ERROR`、`CRITICAL`でメッセージが空または空白だけ

設備固有コードでは負数やコード0が有効な場合もあるため、共通層では値を自動修正しません。同じ内容の
不整合が繰り返し設定された場合は警告ログを抑制し、不整合の内容が変化したときに再度記録します。
`UNKNOWN`は原因を特定できない状態を保持するため、エラーコードとメッセージの組み合わせを制限しません。

ROS通信に依存せず事前確認したい場合は、`validateComponentStatus()`を利用できます。

```cpp
#include <yds/ros2/component_status_validation.h>

const auto validation = yds::ros2::validateComponentStatus(state, errorCode, message);
if (!validation.validState) {
	// 定義外状態として処理する
}
if (validation.hasConsistencyWarning()) {
	// 案件固有ルールと照合する
}
```

## 6. Lifecycle Nodeへ組み込む

Lifecycle Nodeでは`LifecycleComponentStatusNode`を継承します。この共通基底クラスが
`rclcpp_lifecycle::LifecycleNode`の継承、ROSパラメータの宣言・検証、および
`ComponentStatusPublisher`の所有を吸収します。公開する状態通知APIは通常ノード用の
`ComponentStatusNode`と同じです。

```cpp
#include <chrono>

#include <yds/ros2/lifecycle_component_status_node.h>

class LifecycleCameraNode final : public yds::ros2::LifecycleComponentStatusNode {
public:
	LifecycleCameraNode()
		: LifecycleComponentStatusNode(
			"camera_node",
			QStringLiteral("camera-1"),
			QStringLiteral("camera/status"),
			std::chrono::milliseconds(1000)) {}
};
```

`LifecycleComponentStatusNode`は内部で通常のPublisherとTimerを使用し、Lifecycle Nodeが`INACTIVE`の間も
最新状態を定期通知します。これにより、Supervisorは正常にInactiveであるノードと、ノード停止や
通信断を区別できます。Inactive時のコンポーネント状態は、設備の実態に応じて`READY`や`STOPPED`などを
Lifecycleコールバックから明示的に設定してください。

Lifecycle状態からComponentStatusへの自動変換は行いません。Lifecycle状態はノードの管理段階、
ComponentStatusは設備またはロジック機能の業務状態であり、必ずしも一対一に対応しないためです。
`LifecycleComponentStatusNode`の利用側は`yds_ros2`へ依存します。公開ヘッダーでLifecycle APIを直接使用する
場合や、Lifecycle実行ファイルを構成する場合は`rclcpp_lifecycle`にも依存してください。

別の基底クラスを継承済みで`LifecycleComponentStatusNode`を利用できない場合は、従来どおり
`ComponentStatusPublisher`をメンバーとして所有できます。`declareComponentStatusPublisherParameters()`は
通常ノードとLifecycle Nodeのどちらでも利用でき、
3章のROSパラメータを読み取り専用として宣言します。戻り値はパラメータoverrideを反映済みの
`ComponentStatusPublisherConfiguration`です。これにより、ノード種別が異なっても同じYAML設定と
入力値検証を使用できます。設定値をコード内で固定する場合は、この関数を使わず、従来どおり
`ComponentStatusPublisherConfiguration`をPublisherへ直接渡すこともできます。

## 7. QoS

Publisherは`KeepLast(1)`、`Reliable`、`Transient Local`を使用します。Supervisorも
`Reliable`、`Transient Local`でSubscribeするため、設備ノードより後に起動した場合も、保持された
最新状態を受信できます。
定期Publishも行うため、一時的な切断から復旧した後は最新状態を再受信できます。
