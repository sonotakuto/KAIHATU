/**
 * @file process_runner.hpp
 * @brief ProcessRunnerクラスの宣言。
 */
#pragma once

#include <glib.h>
#include <string>
#include <functional>
#include "common.hpp"

/**
 * @namespace gutils
 * @brief GLib関連のユーティリティをまとめた名前空間。
 */
namespace gutils {

/**
 * @brief 外部プロセスの非同期実行とライフサイクル管理を行うRAIIラッパークラス。
 * * GLibのg_spawn_asyncを使用し、メインループをブロックせずにコマンドを実行します。
 * オブジェクトの破棄時に実行中のプロセスを自動的に停止（SIGTERM送信）します。
 */
class ProcessRunner {
public:
	/**
	 * @brief コンストラクタ。
	 */
	ProcessRunner();

	/**
	 * @brief デストラクタ。
	 * プロセスが実行中の場合、SIGTERMを送信して停止させ、リソースを解放します。
	 */
	~ProcessRunner();

	// コピー/ムーブは禁止
	ProcessRunner(const ProcessRunner&) = delete;
	ProcessRunner& operator=(const ProcessRunner&) = delete;
	ProcessRunner(ProcessRunner&&) = delete;
	ProcessRunner& operator=(ProcessRunner&&) = delete;

	/**
	 * @brief 指定されたコマンドを非同期で実行します。
	 * @param[in] command 実行するコマンドライン文字列（例: "ping google.com"）。
	 * @param[in] on_exit (オプション・デフォルト nullptr) プロセス終了時に呼び出されるコールバック関数。
	 * 引数には終了コードが入り、コールバックはメインスレッド（GTKメインループ）から呼び出されます。
	 * @return プロセスの起動に成功した場合はtrue、失敗した場合（または既に実行中の場合）はfalse。
	 */
	bool start(const std::string& command, std::function<void(int)> on_exit = nullptr);

	/**
	 * @brief 実行中のプロセスに終了要請（SIGTERM）を送信します。
	 * \n
	 * プロセスの実際の回収は非同期で行われ、終了後に on_exit コールバックが呼ばれます。
	 */
	void stop() noexcept;

	/**
	 * @brief 実行中のプロセスを強制終了（SIGKILL）させ、即座にリソースを回収します。
	 * \n
	 * この関数を呼んだ場合、on_exit コールバックは呼ばれません。
	 */
	void kill() noexcept;

	/**
	 * @brief プロセスが現在実行中かどうかを確認します。
	 * @return 実行中の場合はtrue、停止している場合はfalse。
	 */
	bool is_running() const noexcept;

	/**
	 * @brief start() が失敗した際のエラーメッセージを取得します。
	 * 成功している場合は空文字列を返します。
	 */
	std::string get_error_message() const;

private:
	GPid pid_ = 0;			///< 実行中のプロセスのPID。実行していない場合は0。
	guint watch_id_ = 0;	///< g_child_watch_addの戻り値（イベントソースID）。
	std::function<void(int)> on_exit_;	///< ユーザー指定の終了時コールバック。
	std::string error_message_;	///< エラーメッセージ保持用。

	/**
	 * @brief GLib用のスタティックな終了検知コールバック。
	 * @param[in] pid 終了したプロセスのPID。
	 * @param[in] status プロセスの終了ステータス。
	 * @param[in] user_data ProcessRunnerインスタンスへのポインタ。
	 */
	static void on_child_exited_static(GPid pid, gint status, gpointer user_data);

	/**
	 * @brief インスタンスメンバとしての終了処理。
	 * @param[in] status プロセスの終了ステータス。
	 */
	void handle_child_exited(gint status);
};

} // namespace gutils
