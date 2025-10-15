/**
 * @file tool_io_main.cpp
 * @brief アプリケーションの入り口となるmain関数などの実装ファイル。
 */
#include "./includes.hpp"
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

	// メッセージループを開始
	// この関数内でイベントの待受が行われ、終了するまで制御は戻らない
	return app.run(argc, argv);
}