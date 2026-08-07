# Linux ROS 2 CIガイド

## 1. 目的

`.github/workflows/linux-ros2-ci.yml`は、GitHub Actions上でROS 2ワークスペースをビルドし、自動テストを
実行します。開発者の環境だけで成功する変更や、既存機能を壊す変更をmainブランチへ取り込む前に検出する
ことが目的です。

CI（Continuous Integration、継続的インテグレーション）は、変更を共有リポジトリへ送るたびに、ビルドと
テストを自動実行する仕組みです。

## 2. 実行環境

- Ubuntu 22.04
- ROS 2 Humble
- Qt 5
- C++17
- `QT_QPA_PLATFORM=offscreen`による画面なしGUIテスト

ソース取得には`actions/checkout@v6`、ROS環境の準備には`ros-tooling/setup-ros@v0.7`を使用します。
その後、ローカル手順と同じ`rosdep`、`colcon build`、`colcon test`、`colcon test-result`を個別のステップで
実行します。プロジェクト内の全ROSパッケージが対象です。

## 3. 実行条件

次の場合にCIを開始します。

- `main`ブランチへのpush
- Pull Requestの作成・更新
- GitHub Actions画面からの手動実行

同じブランチへ新しい変更がpushされた場合、古い実行を中止して最新の変更を検証します。1回の実行時間は
最大45分です。

## 4. 結果の確認

GitHubリポジトリのActions画面で`Linux ROS 2 CI`を選択します。成功時はビルドと全テストが完了しています。
失敗時は`Install dependencies`、`Build`、`Run tests`、`Report test results`のうち、失敗したステップの
出力を確認してください。処理を分けているため、依存解決、コンパイル、テストのどこで失敗したかを
Actions画面から判別できます。

成功・失敗にかかわらず、colconがワークスペースを作成できた場合は`colcon-logs`をアーティファクトとして
14日間保存します。アーティファクトはCI実行結果の画面からダウンロードでき、各パッケージの標準出力、
標準エラー、テスト結果の調査に使用できます。

## 5. ローカルでの再現

CIで失敗した変更は、まずLinux開発環境で次のコマンドを実行して再現します。

```bash
source /opt/ros/humble/setup.bash
rosdep install --from-paths src --ignore-src --rosdistro humble -y
colcon build
source install/setup.bash
QT_QPA_PLATFORM=offscreen colcon test --packages-select \
  yds_interfaces yds_ros2 yds_ros2_widgets ros2_qt_gui \
  sample_processor sample_processor_api_test
colcon test-result --verbose
```

CI固有の失敗である場合は、`colcon-logs`とローカルの`log`ディレクトリを比較します。
