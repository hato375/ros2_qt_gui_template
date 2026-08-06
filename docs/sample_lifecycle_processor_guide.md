# Lifecycleサンプルノード利用ガイド

## 1. 目的

`sample_lifecycle_processor`は、`rclcpp_lifecycle::LifecycleNode`へ
`yds::ros2::ComponentStatusPublisher`を組み込む例です。Lifecycle状態、処理の実行状態、
およびSupervisorが監視する通信状態を別々に扱います。

| Lifecycle遷移 | ComponentStatus | 処理タイマー |
|---|---|---|
| 生成直後 | `INITIALIZING` | 停止 |
| configure | `READY` | 停止 |
| activate | `RUNNING` | 実行 |
| deactivate | `STOPPED` | 停止 |
| cleanup | `INITIALIZING` | 停止、処理回数をリセット |
| shutdown | `STOPPED` | 停止 |
| configure失敗 | `ERROR`、エラーコード`9101` | 停止 |
| activate失敗 | `ERROR`、エラーコード`9102` | 停止 |
| deactivate失敗 | `ERROR`、エラーコード`9103` | 停止 |
| cleanup失敗 | `ERROR`、エラーコード`9104` | 停止 |
| shutdown失敗 | `ERROR`、エラーコード`9105` | 停止 |
| その他のerror処理 | `ERROR`、エラーコード`9001` | 停止 |

コード上では`sample_processor/sample_lifecycle_processor_error_codes.h`の
`sampleprocessor::lifecycle_error_code`名前空間にある定数を使用します。`9000`番台はこの
Lifecycleサンプルの遷移エラー用とし、`9001`を汎用遷移エラー、`9101`～`9105`を各設備固有フックへ
割り当てています。派生ノードではこれらの値を別用途へ流用しないでください。

別パッケージから派生ノードを実装する場合は、`find_package(sample_processor REQUIRED)`を実行し、
対象へ`sample_processor::sample_lifecycle_processor_node`をリンクします。これによりLifecycleノード、
エラーコード、および必要な依存ライブラリのinclude・link設定が引き継がれます。

```cmake
find_package(sample_processor REQUIRED)
target_link_libraries(
	my_lifecycle_processor
	sample_processor::sample_lifecycle_processor_node
)
```

ComponentStatusの定期通知は処理タイマーとは別に動作します。このため、Lifecycle NodeがInactiveで
処理を停止していてもheartbeatは継続し、Supervisorは正常なInactive状態と通信断を区別できます。

## 2. ビルドと起動

```bash
source /opt/ros/humble/setup.bash
colcon build --packages-up-to sample_processor
source install/setup.bash
ros2 launch sample_processor sample_lifecycle_processor.launch.py
```

起動直後のLifecycle状態は`unconfigured`、ComponentStatusは`INITIALIZING`です。

別のターミナルで状態通知を確認します。

```bash
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 topic echo /sample_lifecycle_processor/status yds_interfaces/msg/ComponentStatus
```

## 3. Lifecycleを操作する

別のターミナルから順番に実行します。

```bash
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 lifecycle get /sample_lifecycle_processor_node
ros2 lifecycle set /sample_lifecycle_processor_node configure
ros2 lifecycle set /sample_lifecycle_processor_node activate
ros2 lifecycle set /sample_lifecycle_processor_node deactivate
ros2 lifecycle set /sample_lifecycle_processor_node cleanup
```

configure後は`READY`、activate後は`RUNNING`になります。Active中は処理回数を含むメッセージが更新され、
deactivate後は`STOPPED`のまま定期通知が続きます。

## 4. Supervisor GUIと同時に確認する

```bash
ros2 launch sample_processor lifecycle_component_status_demo.launch.py
```

GUIの監視ダイアログでは、起動直後からCommunicationが`RECEIVING`になります。Lifecycle操作に応じて
Component stateだけが`INITIALIZING`、`READY`、`RUNNING`、`STOPPED`へ変化することを確認します。
Inactive中もCommunicationは`RECEIVING`のままです。ノードを終了すると、既定では3秒後に
`TIMED OUT`へ変化します。

実プロセスの結合テストは、Lifecycleサービスを使ってconfigure、activate、shutdownを実行し、
各段階のLifecycle状態とComponentStatusをブラックボックスで検証します。次のコマンドで実行できます。

```bash
colcon test --packages-select sample_processor \
  --ctest-args -R sample_lifecycle_processor_launch_test
colcon test-result --verbose
```

## 5. パラメータ

| パラメータ | 既定値 | 範囲 | 用途 |
|---|---:|---:|---|
| `processing_interval_ms` | 1000 | 100～600000 | Active中の処理周期 |
| `component_status.component_id` | `sample-lifecycle-processor-1` | 空文字不可 | コンポーネントID |
| `component_status.status_topic` | `sample_lifecycle_processor/status` | 空文字不可 | 状態通知トピック |
| `component_status.publish_interval_ms` | 1000 | 100～600000 | Lifecycle状態に依存しない通知周期 |

ComponentStatus設定は`declareComponentStatusPublisherParameters()`で宣言・検証します。通常ノードと
Lifecycle Nodeで同じパラメータ構造を利用できます。

## 6. 実案件へ適用する場合

このサンプルのLifecycle状態とComponentStatusの対応は一例です。実案件では、Lifecycle遷移が成功しても
設備が運転可能とは限りません。カメラ接続、PLC応答、キャリブレーション、安全信号などを確認したうえで、
設備の実態に合うComponentStatusを明示的に設定してください。

`SampleLifecycleProcessorNode`を派生させ、設備固有の初期化処理を`configureProcessor()`、処理開始を
`activateProcessor()`へ実装できます。成功時は`true`を返します。失敗時は`errorMessage`へ理由を設定して
`false`を返すと、Lifecycle遷移はerror処理を経て`unconfigured`へ戻り、ComponentStatusは
`ERROR`になります。状態通知のheartbeatはその後も継続するため、SupervisorではComponent stateが
`ERROR`、Communicationが`RECEIVING`になります。

```cpp
bool CameraProcessorNode::configureProcessor(QString& errorMessage) {
	if (!camera_.connect()) {
		errorMessage = QStringLiteral("Camera connection failed");
		return false;
	}
	return true;
}
```

状態通知自体に失敗した場合もLifecycleコールバックは`ERROR`を返します。error処理で`ERROR`通知を
再試行し、それにも失敗した場合は`FAILURE`を返してLifecycle状態を`finalized`へ遷移させます。

error処理が成功するとLifecycle状態は`unconfigured`へ戻ります。失敗原因を取り除いた後にconfigureを
再実行するとComponentStatusは`READY`となり、エラーコードとエラーメッセージがクリアされます。
activate失敗後も、configureから再実行して`READY`、`RUNNING`の順に復旧します。自動復旧を行う場合は、
無制限に再試行せず、回数上限、待機時間、および安全条件を設備要件に合わせて定義してください。

`configureProcessor()`または`activateProcessor()`から例外が送出された場合は、Lifecycleノードが
`std::exception`とその他の例外を境界で捕捉します。例外の詳細は原因調査用としてROSログに記録し、
ComponentStatusには内部情報を含まない定型メッセージを設定します。エラーコードは処理が`false`を
返した場合と同じく、configureでは`9101`、activateでは`9102`です。例外後の復旧手順も同じです。

終了側の設備固有処理は、目的ごとに次のフックへ分けます。

| フック | 用途 | 呼び出し時のLifecycle状態 |
|---|---|---|
| `deactivateProcessor()` | 動作の安全停止、停止完了確認 | Activeからのdeactivate |
| `cleanupProcessor()` | 接続切断、設定済み資源の解放 | Inactiveからのcleanup |
| `shutdownProcessor()` | 遷移元に依存しない最終終了処理 | shutdown |

処理タイマーは各フックより先に停止します。フックが`false`を返した場合や例外を送出した場合は
ComponentStatusを`ERROR`にし、error処理成功後は`unconfigured`へ戻ります。shutdown処理の失敗時も
`finalized`にしたことにはせず、原因を解消してshutdownを再試行できます。デストラクタだけに安全停止を
依存させず、`shutdownProcessor()`は複数回呼ばれても安全な処理として実装してください。shutdownは
Unconfigured、Inactive、Activeのどの状態からも呼ばれます。Activeからの場合も処理タイマーを先に停止し、
フック成功後にComponentStatusを`STOPPED`としてLifecycle状態を`finalized`へ遷移させます。

ComponentStatus通知は安全停止を代行しません。`ERROR`や`CRITICAL`を通知する処理とは別に、必要な停止、
リトライ、復旧処理を実装します。
