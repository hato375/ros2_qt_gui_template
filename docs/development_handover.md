# 開発引き継ぎ書

- 更新日時: 2026-09-02T17:24:30+09:00
- 対象リポジトリ: `hato375/ros2_qt_gui_template`

## 1. 30秒で分かる現在地

- ノード間の死活・状態監視基盤は、共通Publisher、Supervisor、GUI、通常・Lifecycleサンプル、テスト、
  Linux CIまで実装済みです。
- Lifecycleノード用の共通基底クラス`yds::ros2::LifecycleComponentStatusNode`を追加し、サンプルノード、
  テスト、関連資料を移行しました。機能コミットは`374aa3c`です。
- ブランチは`main`です。本書更新時点では機能コミットが未pushで、本書だけが未ステージの変更です。
- 一時ディレクトリを使用したクリーンビルドと全テストは、依存する6パッケージすべて成功しています。
  テスト結果は91件成功、失敗・エラー・スキップなしです。
- 直近CIは[Linux ROS 2 CI #8](https://github.com/hato375/ros2_qt_gui_template/actions/runs/33596047014)で、
  `ce78245`を対象に成功しています。
- 次の推奨作業は、Supervisor内部異常を自身の`ComponentStatus`へ連動させる判定基準の設計です。

## 2. 完了した作業

### 状態監視基盤

状態監視基盤は`fcd2dbf`以前のコミットで完成しています。主な機能は次のとおりです。

- `yds_interfaces/msg/ComponentStatus`によるコンポーネントID、状態、エラーコード、メッセージ、生成時刻の通知
- `yds::ros2::ComponentStatusPublisher`による状態変更時の即時Publishと最新状態の定期Publish
- `Reliable`、`Transient Local` QoSによる最新状態保持
- `yds::ros2::ComponentStatusNode`による通常の`rclcpp::Node`向け便利基底クラス
- `yds::ros2::LifecycleComponentStatusNode`によるLifecycleノード向け便利基底クラス
- `ros2qtgui::RosNode`による複数コンポーネントの受信、値・時刻品質検査、タイムアウト・復旧判定
- Supervisor自身の`ros2_qt_gui/status`への定期状態通知
- GUIでの全体状態集約、詳細表示、検索、並び替え、要確認項目の絞り込み
- 通常ノードとLifecycleノードのサンプル、正常・不正YAMLの起動テスト

### GitHubプロジェクトへの展開手順

- `a8bbf83 GitHubプロジェクトへの展開手順を追加`
- `docs/development_setup_guide.md`へ、Git履歴やcolcon生成物を除外してテンプレートを複製し、
  `setup_dev.sh`で初期化してGitHubへ初回pushする手順を追加しました。

### セッション引き継ぎプロンプト

- `ce78245 開発セッションの引き継ぎプロンプトを追加`
- `prompts/development_handover_prompt.md`へ、Git、差分、検証、CIを実確認して本書を更新するための
  再利用可能なプロンプトを追加しました。
- `README.md`のドキュメント一覧からプロンプトへ移動できるようにしました。

### Lifecycle用ComponentStatusNodeの共通クラス化

- `374aa3c Lifecycleノード用の状態通知基底クラスを追加`
- `yds_ros2`へ`LifecycleComponentStatusNode`を追加し、Lifecycleノード固有の基底クラス、
  `ComponentStatusPublisher`の所有、状態通知パラメータの宣言・検証を共通層へ集約しました。
- 公開APIは通常ノード用の`ComponentStatusNode`と同じです。
- `SampleLifecycleProcessorNode`を新しい共通基底クラスへ移行しました。
- Inactive中の定期通知、不正な通知周期の拒否、インストール後の公開API利用をテストへ追加しました。
- Lifecycle状態からComponentStatusへの自動変換は追加していません。設備・処理の実態に応じ、派生クラスの
  Lifecycleコールバックから明示的に設定します。

## 3. 現在進行中の作業

- なし。Lifecycle共通基底クラスと本書のコミットを`origin/main`へpushした後、CI結果を確認します。

## 4. 設計判断と制約

### 状態送信の責務

状態管理とROS Publishは`ComponentStatusPublisher`へ集約しています。通常ノードは
`ComponentStatusNode`、Lifecycleノードは`LifecycleComponentStatusNode`を継承できます。別の基底クラスを
持つノードはPublisherをメンバーとして所有します。

### Lifecycleノードへの対応

`LifecycleComponentStatusNode`は`rclcpp_lifecycle::LifecycleNode`を継承し、状態Publisherの所有と
ROSパラメータ処理を共通層で吸収します。実装例は`SampleLifecycleProcessorNode`、共通層での動作確認は
`LifecycleComponentStatusNodeTest.PublishesHeartbeatWhileInactive`です。すでに別の基底クラスを持つ場合は、
従来どおり`ComponentStatusPublisher`をメンバーとして所有できます。

Lifecycle状態はノードの管理段階、ComponentStatusは設備・処理の業務状態であり、一対一とは限りません。
このため自動変換せず、各Lifecycleコールバックから`READY`、`RUNNING`、`STOPPED`、`ERROR`などを明示的に
設定します。PublisherはLifecycleノードがInactiveでも定期通知を継続し、Supervisorが正常なInactiveと
通信断を区別できる設計です。

### Supervisorの責務

`RosNode`は監視対象の状態受信と品質検査を担当し、自身の状態送信は`ComponentStatusNode`へ委譲します。
現時点ではGUIコールバックの継続失敗などの内部異常を、Supervisor自身の`WARNING`や`ERROR`へ連動させて
いません。連動条件、復旧条件、通知抑制を設計してから実装する必要があります。

### 共通制約

- C++17、Qt 5、ROS 2 Humbleを基準とします。
- Lifecycle状態通知を設備の安全停止処理の代替にしてはいけません。
- 接続、切断、エラー、リトライ、設定変更はログへ記録します。
- Windows互換性の実機確認は未実施であり、現在のROS 2運用対象はLinuxです。

## 5. 検証結果

### 今回実施（Lifecycle共通基底クラス）

- `/tmp`配下の独立したbuild/install/logディレクトリで、`sample_processor_api_test`まで依存する
  6パッケージをクリーンビルドし、すべて成功しました。
- ROS 2通信を許可した環境で全6パッケージのテストを実行し、91件成功、失敗・エラー・スキップなしでした。
- `yds_ros2`の27件のテストには、新しいLifecycle基底クラスのInactive中通知と入力検証が含まれます。
- `sample_processor_api_test`により、インストール済みの公開ヘッダーとライブラリから新しい基底クラスを
  継承し、状態通知APIを利用できることを確認しました。
- `git diff --check`は成功しています。
- 既存の`build/yds_interfaces`を使用した`--symlink-install`ビルドは、残存ディレクトリと生成対象の
  シンボリックリンクが衝突しました。既存生成物は削除せず、`/tmp`配下の独立環境で検証しました。

### 引き継ぎ書の最新化時に実施

- `git status --short --branch`、`git log -10 --oneline --decorate`、`git branch -vv`でGit状態を確認しました。
- GitHub APIでCI #8が`ce78245`を対象に完了し、`success`であることを確認しました。
- Lifecycle対応について、実装、利用ガイド、単体テストを照合しました。
- 本書更新後に`git diff --check`、Git状態、記載パスとコミットの存在を確認しました。

### 未実施

- `374aa3c`に対するCIは、push前のため未実施です。
- CI #8の正確なテスト件数はGitHub Actionsの実行結果APIから確認していません。

### 直近の確認済み結果

- 2026-08-07: ローカル全テスト89件成功、失敗・エラー・スキップなし。
- 2026-09-02確認: Linux ROS 2 CI #8成功。Ubuntu 22.04、ROS 2 Humble上で依存導入、ビルド、全テスト、
  結果集計を実行するワークフローです。

## 6. GitとCI

- ブランチ: `main`
- 本書更新時のHEAD: `374aa3c Lifecycleノード用の状態通知基底クラスを追加`
- upstream: `origin/main`
- 本書更新時のahead/behind: `1/0`
- 本書更新時の未pushコミット: `374aa3c`の1件。本書のコミット後に2件をまとめてpushする予定
- 作業開始前の作業ツリー: `docs/development_handover.md`だけが未ステージ
- 本書更新時の作業ツリー: `docs/development_handover.md`だけが未ステージ
- CI: `Linux ROS 2 CI`、run #8、対象`ce78245`、成功
- CI URL: https://github.com/hato375/ros2_qt_gui_template/actions/runs/33596047014

## 7. 残件と次の作業候補

### 推奨: Supervisor内部異常と自身の状態の連動

- 目的: Supervisor内部の継続的な処理失敗を上位監視から検出可能にします。
- 最初の一手: GUIコールバック失敗など、内部異常として扱う事象、連続回数、状態遷移、復旧条件、
  繰り返し通知抑制を設計資料へまとめます。
- 完了条件: 判定基準の合意後、実装、単体テスト、関連資料、全テスト、CIが完了することです。

### 低優先度: 実設備ノードへの適用

- 目的: カメラ、PLC、ロボット、画像処理などの実ノードへ状態送信を組み込みます。
- 最初の一手: 適用対象ノードと状態・エラーコード体系を決定します。
- 完了条件: 対象ノードの正常、異常、復旧、安全停止をSupervisorから確認できることです。

### 低優先度: 上位Supervisorによる階層監視の構成例

- 目的: 複数設備または複数セルの状態を上位で統合監視できる構成を示します。
- 最初の一手: 上位へ集約する状態、コンポーネントID、トピック階層、異常伝播の規則を整理します。
- 完了条件: 必要性が具体化した時点で構成例、設定例、監視テストを追加することです。

### 低優先度: ROS 2コードのWindows互換性点検

- 目的: CMake、パス、コンパイラ差異、公開APIのWindows互換性を確認します。
- 最初の一手: Linux依存箇所とWindowsで利用するROS 2ディストリビューションを洗い出します。
- 完了条件: Windowsでビルドと主要テストを実行し、制約を文書化することです。

### 低優先度: 監視イベント専用の履歴表示

- 目的: コンポーネント監視の異常・復旧だけを時系列で確認できるようにします。
- 最初の一手: 表示項目、保持上限、絞り込み条件を設計します。
- 完了条件: 件数制限付き履歴、対象・異常種別の絞り込み、GUIテストが完成することです。

低優先度項目の正本は`docs/development_backlog.md`です。

## 8. 再開手順

最初に次の順番で資料を読みます。

1. `AGENTS.md`
2. `docs/development_handover.md`
3. `prompts/ros2_qt_gui_agent_prompt.md`
4. 選択した作業に対応する関連資料

引き継ぎ書の状態を鵜呑みにせず、最初にGitとCIを再確認します。

```bash
git status --short --branch
git log -10 --oneline --decorate
git branch -vv
```

標準のビルド・テスト手順です。

```bash
source /opt/ros/humble/setup.bash
colcon build --symlink-install
source install/setup.bash
ROS_LOG_DIR=/tmp/ros2_qt_gui_test \
QT_QPA_PLATFORM=offscreen \
colcon test --event-handlers console_direct+
colcon test-result --verbose
```

次の推奨作業を選ぶ場合は、`src/ros2_qt_gui/src/ros_node.cpp`と
`docs/topic_reception_supervisor_guide.md`から内部異常の発生経路と現在の状態通知箇所を確認します。

## 9. 関連資料

- `prompts/development_handover_prompt.md`: 本書を実状態から更新するためのプロンプト
- `prompts/ros2_qt_gui_agent_prompt.md`: AIエージェント向けの基本設計・実装方針
- `docs/development_backlog.md`: 低優先度の残件の正本
- `docs/component_status_node_guide.md`: 通常・Lifecycleノードへの状態送信機能の組み込み方法
- `docs/sample_lifecycle_processor_guide.md`: Lifecycleサンプルの状態遷移、異常、復旧、終了処理
- `docs/topic_reception_supervisor_guide.md`: Supervisorの設定、監視判定、手動確認
- `docs/component_monitor_dialog_guide.md`: GUI詳細ダイアログの利用方法
- `docs/continuous_integration_guide.md`: Linux ROS 2 CIの実行条件とログ確認方法
- `src/sample_processor/include/sample_processor/sample_lifecycle_processor_node.h`: Lifecycle対応の公開クラス
- `src/yds_ros2/test/component_status_node_test.cpp`: 通常・LifecycleノードでのPublisher動作テスト
