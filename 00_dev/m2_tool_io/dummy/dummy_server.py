import socket
import string
import threading
import time

# ==========================================
# 設定 (C++側のヘッダファイルに合わせています)
# ==========================================
TRACKER_PORT = 5000        # ToolTrackerClient用
COUNTER_PORT = 12345       # InsertCounter用
MSG_LENGTH = 24            # 固定長メッセージ長

# 通知メッセージの定義 (tracking_communicator.hppより)
NOTIFICATIONS = {
    "1": "Scan_S",                  # スキャン成功
    "2": "Scan_F",                  # スキャン失敗
    "3": "Scanned_in_container",    # スキャン済工具 -> コンテナIN
    "4": "Scanned_out_container",   # スキャン済工具 -> コンテナOUT
    "5": "Other_in_container",      # その他工具 -> コンテナIN
    "6": "Other_out_container",     # その他工具 -> コンテナOUT
    "7": "Unscanned_in_container",  # 未スキャン -> コンテナIN
    "8": "Unscanned_out_container", # 未スキャン -> コンテナOUT
    "9": "Unscanned_out_frame",     # 未スキャン -> フレームOUT
}

# 接続中のクライアントリスト
clients = []
clients_lock = threading.Lock()

def pad_message(msg):
    """メッセージを24バイトにスペースでパディングする"""
    return msg.ljust(MSG_LENGTH).encode('utf-8')

def handle_tracker_client(conn, addr):
    """工具追跡クライアントとの通信を処理"""
    print(f"[Tracker] クライアント接続: {addr}")
    with clients_lock:
        clients.append(conn)

    try:
        while True:
            data = conn.recv(MSG_LENGTH)
            if not data:
                break

            # 受信データの処理
            raw_msg = data.decode('utf-8', errors='ignore')
            msg = raw_msg.rstrip(string.whitespace + '\0')
            print(f"[Tracker] 受信: '{msg}' (Raw: {len(data)} bytes)")

            # プロトコルに応じた応答
            if msg == "Connect":
                print("[Tracker] >> Connect_ACK を送信")
                conn.sendall(pad_message("Connect_ACK"))
            elif msg == "Disconnect":
                print("[Tracker] >> Disconnect_ACK を送信")
                conn.sendall(pad_message("Disconnect_ACK"))
                break # 切断処理へ
            elif msg == "QR_scan":
                print("[Tracker] QRコードスキャン通知を受信しました。")
                # 必要であればここで自動的に Scan_S などを返しても良い
            else:
                print(f"[Tracker] 未知のコマンド: {msg}")

    except ConnectionResetError:
        print(f"[Tracker] クライアント {addr} が強制切断されました")
    finally:
        with clients_lock:
            if conn in clients:
                clients.remove(conn)
        conn.close()
        print(f"[Tracker] 接続終了: {addr}")

def tracker_server_thread():
    """工具追跡サーバーのスレッド"""
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    # アドレス再利用設定 (再起動時のエラー防止)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        server.bind(('0.0.0.0', TRACKER_PORT))
        server.listen(5)
        print(f"[Tracker] サーバー起動: Port {TRACKER_PORT}")

        while True:
            conn, addr = server.accept()
            t = threading.Thread(target=handle_tracker_client, args=(conn, addr))
            t.daemon = True
            t.start()
    except Exception as e:
        print(f"[Tracker] サーバーエラー: {e}")

def handle_counter_client(conn, addr):
    """インサート個数取得サーバーの処理（ワンショット）"""
    print(f"[Counter] 接続: {addr}")
    try:
        # 接続されたら即座に数値を送って切断する仕様
        # 任意の個数（例: 100）を送信
        response = "3"
        conn.sendall(response.encode('utf-8'))
        print(f"[Counter] データ送信: {response}")
    except Exception as e:
        print(f"[Counter] エラー: {e}")
    finally:
        conn.close()

def counter_server_thread():
    """インサート個数サーバーのスレッド"""
    server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)

    try:
        server.bind(('0.0.0.0', COUNTER_PORT))
        server.listen(5)
        print(f"[Counter] サーバー起動: Port {COUNTER_PORT}")

        while True:
            conn, addr = server.accept()
            t = threading.Thread(target=handle_counter_client, args=(conn, addr))
            t.daemon = True
            t.start()
    except Exception as e:
        print(f"[Counter] サーバーエラー: {e}")

def input_loop():
    """ユーザー入力を受け付けてイベントを送信するループ"""
    time.sleep(1)
    print("\n=== 操作メニュー ===")
    for key, val in NOTIFICATIONS.items():
        print(f" {key}: {val} を送信")
    print(" q: 終了")
    print("==================\n")

    while True:
        cmd = input("コマンドを入力 > ")
        if cmd == 'q':
            print("終了します...")
            break

        if cmd in NOTIFICATIONS:
            msg_str = NOTIFICATIONS[cmd]
            padded_msg = pad_message(msg_str)

            with clients_lock:
                if not clients:
                    print("⚠️ 接続中のクライアントがいません")
                for conn in clients:
                    try:
                        conn.sendall(padded_msg)
                        print(f">> 送信成功: {msg_str}")
                    except Exception as e:
                        print(f"送信エラー: {e}")
        else:
            print("無効なコマンドです")

if __name__ == "__main__":
    # 工具追跡サーバーを別スレッドで起動
    t_tracker = threading.Thread(target=tracker_server_thread)
    t_tracker.daemon = True
    t_tracker.start()

    # インサート個数サーバーを別スレッドで起動
    #t_counter = threading.Thread(target=counter_server_thread)
    #t_counter.daemon = True
    #t_counter.start()

    # メインスレッドで入力待ち
    input_loop()
