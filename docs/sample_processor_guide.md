# ロジック系サンプルノード利用ガイド

## 1. 目的

`sample_processor`は、物理設備を持たないロジック系ROSノードで
`yds::ros2::ComponentStatusPublisher`を利用する例です。通常の`rclcpp::Node`を継承し、
コンポーネント状態Publisherをメンバーとして所有します。

サンプルは次の順に状態を通知します。

1. 起動直後は`INITIALIZING`
2. 最初の処理タイマーで`READY`
3. 以降の処理タイマーで`RUNNING`

`RUNNING`のメッセージには完了した処理回数を設定します。状態変更時の即時通知に加えて、
`ComponentStatusPublisher`が最新状態を設定周期で再通知します。

## 2. ビルド

```bash
source /opt/ros/humble/setup.bash
colcon build --packages-up-to sample_processor
source install/setup.bash
```

## 3. サンプルノードだけを起動する

```bash
ros2 launch sample_processor sample_processor.launch.py
```

別のターミナルから状態を確認できます。

```bash
source /opt/ros/humble/setup.bash
source install/setup.bash
ros2 topic echo /sample_processor/status yds_interfaces/msg/ComponentStatus
```

## 4. Supervisor GUIと同時に起動する

```bash
ros2 launch sample_processor component_status_demo.launch.py
```

このlaunchファイルは次の2ノードを起動します。

- `sample_processor_node`
- `ros2_qt_gui_node`

GUIは`sample_processor/status`だけを監視するデモ専用設定で起動します。GUI上で通信状態が
`RECEIVING`、コンポーネント状態が`READY`から`RUNNING`へ変化することを確認できます。

## 5. パラメータ

| パラメータ | 既定値 | 範囲 | 用途 |
|---|---:|---:|---|
| `processing_interval_ms` | 1000 | 100～600000 | サンプル処理周期 |
| `component_status.component_id` | `sample-processor-1` | 空文字不可 | コンポーネントID |
| `component_status.status_topic` | `sample_processor/status` | 空文字不可 | 状態通知トピック |
| `component_status.publish_interval_ms` | 1000 | 100～600000 | 最新状態の定期通知周期 |

既定値は`src/sample_processor/config/sample_processor.yaml`にあります。

## 6. 実案件へ適用する場合

実際の画像処理や点群処理ノードでは、サンプルのタイマー処理を実処理の完了通知へ置き換えます。

- 初期化開始時: `INITIALIZING`
- 初期化成功時: `READY`
- 処理中または正常稼働時: `RUNNING`
- 処理を継続できる注意状態: `WARNING`
- 処理失敗時: `ERROR`
- 安全上直ちに対応が必要な状態: `CRITICAL`

状態通知は安全停止を代行しません。必要な停止、リトライ、復旧処理は利用するノードに実装します。
