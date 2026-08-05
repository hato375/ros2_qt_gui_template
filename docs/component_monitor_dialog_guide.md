# コンポーネント監視ダイアログ利用ガイド

## 1. 目的

`yds_ros2_widgets`の`yds::ros2::widgets::ComponentMonitorDialog`は、複数コンポーネントの
通信状態と動作状態を保守・障害解析時に確認するための再利用可能なQtダイアログです。

固定レイアウトは`component_monitor_dialog.ui`で定義し、Qt Designerから編集できます。動的な行更新、
状態集約、色分け、およびシグナル通知はC++へ分離しています。ビルド時にCMakeの`AUTOUIC`が`.ui`から
必要なヘッダーを生成するため、生成される`ui_component_monitor_dialog.h`は直接編集しません。

通常の業務画面へ詳細テーブルを常時配置する必要はありません。アプリケーションは集約状態だけを
通常画面へ表示し、必要なときに監視ダイアログを開けます。非表示の間も状態更新を渡し続ければ、
再表示時に最新状態を確認できます。

詳細テーブルは列ヘッダーをクリックして並び替えられます。検索欄は表示名、トピック名、ID、状態、
エラーコード、日時、およびメッセージを大文字・小文字を区別せず部分一致で絞り込みます。
`要確認のみ表示`を有効にすると、次の対象だけを表示します。

- 通信状態が`WAITING`または`TIMED OUT`
- 動作状態が`UNKNOWN`、`WARNING`、`ERROR`、または`CRITICAL`

`INITIALIZING`、`READY`、`RUNNING`、`STOPPED`で正常に受信中の対象は非表示になります。
フィルターは表示だけに作用し、非表示になった対象の監視と集約状態の計算は継続します。

## 2. 依存関係

利用するパッケージの`package.xml`へ追加します。

```xml
<depend>yds_ros2_widgets</depend>
```

`CMakeLists.txt`ではパッケージを検索し、ライブラリへリンクします。

```cmake
find_package(yds_ros2_widgets REQUIRED)

target_link_libraries(my_gui
	yds_ros2_widgets::yds_ros2_widgets
)
```

## 3. 生成と状態更新

ダイアログはGUIスレッドで生成します。

```cpp
#include <yds/ros2/widgets/component_monitor_dialog.h>

auto* monitorDialog =
	new yds::ros2::widgets::ComponentMonitorDialog(mainWindow);

monitorDialog->setComponentDisplayName(
	QStringLiteral("camera/status"),
	QStringLiteral("前面カメラ"));
monitorDialog->setTopicReceptionStatus(receptionStatus);
monitorDialog->setComponentStatus(componentStatus);
```

ROS Executorスレッドから直接ダイアログを更新してはいけません。`RosQtBridge`などのQtシグナルを使い、
`Qt::QueuedConnection`でGUIスレッドへ渡します。

```cpp
QObject::connect(
	&rosQtBridge,
	&RosQtBridge::topicReceptionStatusUpdated,
	monitorDialog,
	&yds::ros2::widgets::ComponentMonitorDialog::setTopicReceptionStatus,
	Qt::QueuedConnection);
```

## 4. 集約状態の利用

通常画面で概要だけを表示する場合は、`overallStatusChanged`を接続します。

```cpp
QObject::connect(
	monitorDialog,
	&yds::ros2::widgets::ComponentMonitorDialog::overallStatusChanged,
	mainWindow,
	&MainWindow::setOverallStatus);
```

シグナルは集約状態、受信中の対象数、全監視対象数を通知します。集約状態の判定規則は
`docs/topic_reception_supervisor_guide.md`を参照してください。

## 5. 表示と非表示

詳細表示ボタンなどから`show()`を呼びます。

```cpp
monitorDialog->show();
monitorDialog->raise();
monitorDialog->activateWindow();
```

通常の`close()`ではダイアログは非表示になり、親ウィジェットが所有している間は状態を保持します。
`Qt::WA_DeleteOnClose`は設定しないでください。アプリケーション終了時は親ウィジェットとともに破棄します。
