#include <gtk/gtk.h>
#include <iostream>
#include <ctime>

// アプリケーションの状態を管理するための構造体
struct AppData {
	GtkWidget *main_window;
	GtkWidget *dialog;
	bool is_dialog_visible;
};

// 「×」ボタンでダイアログが閉じられるのを防ぐためのコールバック関数
gboolean on_dialog_delete_event(GtkWidget *widget, GdkEvent *event, gpointer user_data) {
	// TRUEを返すと、ウィンドウを閉じるデフォルトの動作がキャンセルされる
	return TRUE;
}

// 監視対象のダミー関数
// 5秒ごとに正常値(0)と不正値(-1)を交互に返す
int check_system_status() {
	long seconds = time(NULL);
	if ((seconds / 5) % 2 == 0) {
		std::cout << "System Status: Normal" << std::endl;
		return 0; // Normal value
	} else {
		std::cout << "System Status: Abnormal" << std::endl;
		return -1; // Abnormal value
	}
}

// 定期的に呼ばれ、状態をチェックしてダイアログを制御する関数
gboolean check_and_update(gpointer user_data) {
	AppData* data = static_cast<AppData*>(user_data);

	int status = check_system_status();

	if (status != 0) { // Abnormal value
		if (!data->is_dialog_visible) {
			// Show the dialog if it is not visible
			gtk_widget_show_all(data->dialog);
			data->is_dialog_visible = true;
		}
	} else { // Normal value
		if (data->is_dialog_visible) {
			// Hide the dialog if it is visible
			gtk_widget_hide(data->dialog);
			data->is_dialog_visible = false;
		}
	}

	// Return TRUE to keep the timer running
	return TRUE;
}

int main(int argc, char *argv[]) {
	// Initialize GTK
	gtk_init(&argc, &argv);

	// Create an instance of AppData
	AppData data = {nullptr, nullptr, false};

	// 1. Create the main window
	data.main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
	gtk_window_set_title(GTK_WINDOW(data.main_window), "Status Monitoring Application");
	gtk_window_set_default_size(GTK_WINDOW(data.main_window), 400, 200);
	gtk_container_set_border_width(GTK_CONTAINER(data.main_window), 10);
	g_signal_connect(data.main_window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

	// 2. Place a dummy button
	GtkWidget *button = gtk_button_new_with_label("Dummy Button");
	gtk_container_add(GTK_CONTAINER(data.main_window), button);

	// 3. Create the modal dialog (hidden at first)
	data.dialog = gtk_message_dialog_new(
		GTK_WINDOW(data.main_window),
		static_cast<GtkDialogFlags>(GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT),
		GTK_MESSAGE_WARNING,
		GTK_BUTTONS_NONE,
		"An invalid value was detected in the system."
	);
	gtk_message_dialog_format_secondary_text(
		GTK_MESSAGE_DIALOG(data.dialog),
		"Please wait until the status returns to normal..."
	);

	// ★★★ 追加部分 ★★★
	// ダイアログの "delete-event" (×ボタン) を捕捉してキャンセルする
	g_signal_connect(data.dialog, "delete-event", G_CALLBACK(on_dialog_delete_event), NULL);

	// 4. Set a timer to call check_and_update every 1000ms (1 second)
	g_timeout_add(1000, check_and_update, &data);

	// 5. Show the window
	gtk_widget_show_all(data.main_window);

	// 6. Start the GTK main loop
	gtk_main();

	return 0;
}