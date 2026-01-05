/**
 * @file tool_io_main.cpp
 * @brief アプリケーションの入り口となるmain関数などの実装ファイル。
 */
#include "./includes.hpp"
#include "./plc_communicator.hpp"
#include "./tracking_communicator.hpp"
#include "./tool_io_gui.hpp"

int main(int argc, char *argv[])
{
	// アプリケーションオブジェクトの生成
	Application app;

	// activateハンドラ: アプリケーション起動時に一度だけ呼ばれる
	app.on_activate([&app]{
		// 1. CSSファイルを読み込む
		load_css("tool_io.css");

		// 2. メインウィンドウを生成
		// 設計図(m2-1-1)の「GUIとスレッドを起動する」に相当 [cite: 195]
		gui::mainWindow = make_unique<gui::MainWindow>();

		// 3. アプリケーションにウィンドウを登録する
		gui::mainWindow->set_application(app);

		// 4. ウィンドウを表示する
		gui::mainWindow->show_all();
	});

	// 工具追跡クライアントのインスタンスを初期化
	tracker::g_tool_tracker = std::make_unique<tracker::ToolTrackerClient>(
		// ▼▼▼ このラムダ式がコールバック関数 ▼▼▼
		[](const tracker::ToolTrackerClient::Notification& notification) {
			// このラムダ式は ToolTrackerClient のリスナースレッドから呼び出される

			// GUIスレッドで実行したい処理を std::function にキャプチャする
			// notification 変数をコピーキャプチャ [=] する
			auto* func_to_dispatch = new std::function<void()>([=]() {
				// この内部のコードは MainWindow::dispatch_to_gui_thread を経由して
				// GUI スレッド (メインスレッド) で安全に実行される

				if (gui::mainWindow) { // mainWindow が有効か確認
					// MainWindow のメソッド (例: tracker_on_notified) を呼び出す
					gui::mainWindow->tracker_on_notified(notification);
				}
			});

			// g_idle_add を使って GUI スレッドでの実行を予約
			g_idle_add(gui::MainWindow::dispatch_to_gui_thread, func_to_dispatch);
		}
		// ▲▲▲ コールバック関数の定義ここまで ▲▲▲
	);

	// 録画クライアントのインスタンスを初期化
	tracker::g_recorder_client = std::make_unique<tracker::RecorderClient>();

	plc::initialize();	// PLC連携用スレッドを起動

	// メッセージループを開始
	// この関数内でイベントの待受が行われ、終了するまで制御は戻らない
	return app.run(argc, argv);
}
