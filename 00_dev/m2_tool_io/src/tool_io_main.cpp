/**
 * @file tool_io_main.cpp
 * @brief アプリケーションの入り口となるmain関数などの実装ファイル。
 */
#include "utils/filesystem_utils.hpp"
#include "includes.hpp"
#include "keyboard_utils.hpp"
#include "plc_communicator.hpp"
#include "tracking_communicator.hpp"
#include "tool_io_gui.hpp"

int main(int argc, char *argv[])
{
	// アプリケーションオブジェクトの生成
	Application app;

	onboard::ensure_running(); // onboardキーボードが起動していなければ起動する

	// activateハンドラ: アプリケーション起動時に一度だけ呼ばれる
	app.on_activate([&app]{
		// 1. CSSファイルを読み込む
		load_css(utils::filesystem::path_join(utils::filesystem::get_executable_dir().first, "..", "gui", "tool_io.css").lexically_normal().c_str());

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
		[](const tracker::ToolTrackerClient::Notification& notification) {
			// このラムダ式は ToolTrackerClient のリスナースレッドから呼び出される

			// GUIスレッドで実行したい処理を std::function にキャプチャする
			// notification 変数をコピーキャプチャ [=] する
			// gutils::idle_add_once を使って GUI スレッドでの実行を予約
			gutils::idle_add_once([=]() {
				if (gui::mainWindow) { // mainWindow が有効か確認
					// MainWindow のメソッド (例: tracker_on_notified) を呼び出す
					gui::mainWindow->tracker_on_notified(notification);
				}
			});
		}
	);

	// 録画クライアントのインスタンスを初期化
	tracker::g_recorder_client = std::make_unique<tracker::RecorderClient>();

	plc::initialize();	// PLC連携用スレッドを起動

	// --- シグナル対応 ---

	// SIGPIPE は「無視」するように設定 (OSレベルでシグナルを無効化)
	// これにより、通信切断時に send() がエラーを返すようになり、アプリが落ちなくなる
	::signal(SIGPIPE, SIG_IGN);

	// 終了要求系シグナルのハンドリング (RAII)
	// 共通で使うラムダ式
	auto safe_exit_handler = []() -> bool {
		if (gui::mainWindow) {
			gui::mainWindow->force_close();
		} else {
			gtk_main_quit();
		}
		return false; // ハンドラ削除
	};

	sgex::UnixSignal sig_int(SIGINT, safe_exit_handler);
	sgex::UnixSignal sig_term(SIGTERM, safe_exit_handler);
	sgex::UnixSignal sig_hup(SIGHUP, safe_exit_handler);
	sgex::UnixSignal sig_quit(SIGQUIT, safe_exit_handler);

	// メッセージループを開始
	// この関数内でイベントの待受が行われ、終了するまで制御は戻らない
	return app.run(argc, argv);
}
