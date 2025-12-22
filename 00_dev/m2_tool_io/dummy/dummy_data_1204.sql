-- --------------------------------------------------------
-- テスト用ダミーデータ生成スクリプト（トリガー連動・実運用シミュレーション版）
-- --------------------------------------------------------

-- 1. データの初期化（外部キー制約順に削除）
SET FOREIGN_KEY_CHECKS = 0;
TRUNCATE TABLE `rec_operation`;
TRUNCATE TABLE `rec_stock`;
TRUNCATE TABLE `mst_tool_assignment`;
TRUNCATE TABLE `unique_tool`;
TRUNCATE TABLE `storage_area`;
TRUNCATE TABLE `mst_program`;
TRUNCATE TABLE `mst_tool`;
TRUNCATE TABLE `mst_user`;
TRUNCATE TABLE `mst_line`;
TRUNCATE TABLE `mst_address`;
TRUNCATE TABLE `mst_password`;
SET FOREIGN_KEY_CHECKS = 1;

-- --------------------------------------------------------
-- 2. 基本マスタの作成
-- --------------------------------------------------------

-- パスワード
INSERT INTO `mst_password` (`password`) VALUES
('$2a$08$6UffR9fo4CT4iMmaqm9cNOouuBrIpyfsR4bEe0DXHDPKRho1Uio.m'); -- パスワード: "1234"

-- 住所（A01～B02）
INSERT INTO `mst_address` (`display_address`, `control_address`) VALUES
('A01', '010'), ('A02', '011'), ('A03', '012'),
('B01', '120'), ('B02', '121');

-- ライン
INSERT INTO `mst_line` (`line_name`) VALUES
('Line-A'), ('Line-B'), ('Line-C');

-- ユーザー
INSERT INTO `mst_user` (`user_id`, `user_name`, `user_print_time`, `per_add`, `per_inventory`, `per_user`, `per_tool`, `per_line`, `per_analysis`, `per_history`, `per_db`, `per_setting`) VALUES
(0001, '管理者ユーザー', '20251216172322', 1, 1, 1, 1, 1, 1, 1, 1, 1),
(0002, '一般作業者', '20251216172322', 0, 0, 0, 0, 0, 0, 0, 0, 0);

-- --------------------------------------------------------
-- 3. 工具マスタの作成
-- --------------------------------------------------------
INSERT INTO `mst_tool` (`basic_tool_id`, `tool_name`, `maker`, `tool_category`, `tool_material`, `stc`, `rop`, `buyer`) VALUES
(20001, '超硬ドリル φ10', 'OSG', 'ドリル', '超硬', 0, 5, '商社A'),	-- パターンA, E用
(40001, 'ハイスエンドミル φ20', '三菱', 'エンドミル', 'HSS', 0, 2, '商社A'),	-- パターンC用
(10001, '旋削用インサート TNMG', 'タンガロイ', 'インサート', 'サーメット', 0, 20, '商社B'),	-- パターンB用
(30001, '特殊タップ M6', 'ヤマワ', 'タップ', 'HSS-E', 0, 1, '商社B'),	-- パターンD用
(20002, '管理外ドリル', '不明', 'ドリル', '不明', NULL, NULL, '不明');	-- 管理外テスト用

-- プログラム割当
INSERT INTO `mst_program` (`program_id`, `line_id`, `program_name`, `program_print_time`) VALUES
(001, 1, 'PROG-001', '20251216172322');

INSERT INTO `mst_tool_assignment` (`program_id`, `tool_num`, `basic_tool_id`) VALUES
(1, 'T01', 20001), (1, 'T02', 10001);

-- --------------------------------------------------------
-- 4. 保管領域の初期定義 (箱だけ用意する)
-- ※ トリガーで toolcase_stc_num が増減するため、初期値は 0 に設定します。
-- --------------------------------------------------------
INSERT INTO `storage_area` (`storage_area_id`, `basic_tool_id`, `toolcase_stc_num`, `display_address`, `last_reconciliation_time`) VALUES
(1, 20001, 0, 'A01', NULL), -- パターンA, E用（ドリル置き場）
(2, 10001, 0, 'A02', NULL), -- パターンB用（インサート置き場）
(3, 30001, 0, 'A03', NULL), -- パターンD用（タップ置き場）
(4, 40001, 0, 'B01', NULL); -- パターンC用（エンドミル置き場）

-- --------------------------------------------------------
-- 5. 個別工具の初期登録 (購入直後の状態)
-- ※ 全て「補充前」ステータス、保管場所なしで登録します。
-- --------------------------------------------------------
INSERT INTO `unique_tool` (`unique_tool_id`, `tool_print_time`, `basic_tool_id`, `unique_num`, `case_pack_num`, `storage_area_id`, `storage_condition`) VALUES
(2000100001, '20251216172322', 20001, 00001, 1, NULL, '補充前'), -- パターンA: 後で A01 に補充
(2000100002, '20251216172322', 20001, 00002, 1, NULL, '補充前'), -- パターンE: 後で A01 に補充し、取り出す
(4000100001, '20251216172322', 40001, 00001, 1, NULL, '補充前'), -- パターンC: そのまま放置（補充待ち）
(1000100001, '20251216172322', 10001, 00001, 10, NULL, '補充前'), -- パターンB: 後で A02 に補充(10個)
(3000100001, '20251216172322', 30001, 00001, 1, NULL, '補充前'); -- パターンD: 後で A03 に補充し、取り出す

-- --------------------------------------------------------
-- 6. 操作履歴の投入（トリガーを動作させて状態遷移させる）
-- --------------------------------------------------------

-- ■ パターンA: 【通常・保管中】を作る
-- ID:2000100001 を StorageID:1 (A01) に 1個補充
INSERT INTO `rec_operation` (`operation_class`, `user_id`, `proc_time`, `storage_area_id`, `unique_tool_id`, `tool_num`, `track_result`) VALUES
('補充', 1, DATE_SUB(NOW(), INTERVAL 5 DAY), 1, 2000100001, 1, NULL);
-- → トリガー効果:
--    unique_tool(2000100001): storage_condition='保管中', storage_area_id=1, case_pack_num=1
--    storage_area(1): toolcase_stc_num = 0 + 1 = 1


-- ■ パターンB: 【インサート・保管中】を作る
-- ID:1000100001 を StorageID:2 (A02) に 10個補充
INSERT INTO `rec_operation` (`operation_class`, `user_id`, `proc_time`, `storage_area_id`, `unique_tool_id`, `tool_num`, `track_result`) VALUES
('補充', 1, DATE_SUB(NOW(), INTERVAL 4 DAY), 2, 1000100001, 10, NULL);
-- → トリガー効果:
--    unique_tool(1000100001): storage_condition='保管中', storage_area_id=2, case_pack_num=10
--    storage_area(2): toolcase_stc_num = 0 + 1 = 1

-- その後、2個取り出す（部分取出）
INSERT INTO `rec_operation` (`operation_class`, `user_id`, `proc_time`, `storage_area_id`, `unique_tool_id`, `tool_num`, `track_result`) VALUES
('取出', 1, DATE_SUB(NOW(), INTERVAL 3 DAY), 2, 1000100001, -2, 'なし');
-- → トリガー効果:
--    unique_tool(1000100001): case_pack_num = 10 - 2 = 8 (ステータスは保管中のまま)
--    rec_stock: 累計出庫数などが更新される


-- ■ パターンD: 【取出済み】を作る
-- 1. まず StorageID:3 (A03) に補充
INSERT INTO `rec_operation` (`operation_class`, `user_id`, `proc_time`, `storage_area_id`, `unique_tool_id`, `tool_num`, `track_result`) VALUES
('補充', 1, DATE_SUB(NOW(), INTERVAL 2 DAY), 3, 3000100001, 1, NULL);
-- → storage_area(3) の toolcase_stc_num が 1 になる

-- 2. その後、全数取り出し
INSERT INTO `rec_operation` (`operation_class`, `user_id`, `proc_time`, `storage_area_id`, `unique_tool_id`, `tool_num`, `track_result`) VALUES
('取出', 1, DATE_SUB(NOW(), INTERVAL 1 DAY), 3, 3000100001, -1, 'なし');
-- → トリガー効果:
--    unique_tool(3000100001): storage_condition='取出', storage_area_id=NULL, case_pack_num=0
--    storage_area(3): toolcase_stc_num = 1 - 1 = 0 (空になる)


-- ■ パターンE: 【データ上は取出・物理的にはコンテナに残存】を作る
-- コンテナA(StorageID:1) に一度補充して、取り出したことにする
-- 1. StorageID:1 (A01) に補充 (これでA01には ID:2000100001 と 2000100002 の2つが入った履歴になる)
INSERT INTO `rec_operation` (`operation_class`, `user_id`, `proc_time`, `storage_area_id`, `unique_tool_id`, `tool_num`, `track_result`) VALUES
('補充', 1, DATE_SUB(NOW(), INTERVAL 2 HOUR), 1, 2000100002, 1, NULL);

-- 2. その後、取り出し処理を行う
INSERT INTO `rec_operation` (`operation_class`, `user_id`, `proc_time`, `storage_area_id`, `unique_tool_id`, `tool_num`, `track_result`) VALUES
('取出', 1, DATE_SUB(NOW(), INTERVAL 1 HOUR), 1, 2000100002, -1, 'あり');
-- → トリガー効果:
--    unique_tool(2000100002): storage_condition='取出', storage_area_id=NULL
--    storage_area(1): toolcase_stc_num が減算される

-- ※ 結果として、ID:2000100002 はデータ上「取出」ステータスでどこの棚にも紐付いていません。
--    しかし、テスト時に作業者が「A01のコンテナにある ID:1002」をスキャンすることで、
--    「取り出し済みのはずなのに、なぜA01にあるの？」という異常系テストが可能です。

-- ■ パターンC: 【補充待ち】
-- ID:4000100001 は初期登録のまま何もしない
-- → unique_tool(4000100001): storage_condition='補充前' のまま


-- --------------------------------------------------------
-- 結果確認用（必要であればコメントアウトを外して確認）
-- --------------------------------------------------------
-- SELECT * FROM unique_tool;
-- SELECT * FROM storage_area;
-- SELECT * FROM rec_stock;
