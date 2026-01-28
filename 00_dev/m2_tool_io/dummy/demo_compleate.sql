-- --------------------------------------------------------
-- ホスト:                          127.0.0.1
-- サーバーのバージョン:                   11.4.8-MariaDB - mariadb.org binary distribution
-- サーバー OS:                      Win64
-- HeidiSQL バージョン:               12.11.0.7065
-- --------------------------------------------------------

/*!40101 SET @OLD_CHARACTER_SET_CLIENT=@@CHARACTER_SET_CLIENT */;
/*!40101 SET NAMES utf8 */;
/*!50503 SET NAMES utf8mb4 */;
/*!40103 SET @OLD_TIME_ZONE=@@TIME_ZONE */;
/*!40103 SET TIME_ZONE='+00:00' */;
/*!40014 SET @OLD_FOREIGN_KEY_CHECKS=@@FOREIGN_KEY_CHECKS, FOREIGN_KEY_CHECKS=0 */;
/*!40101 SET @OLD_SQL_MODE=@@SQL_MODE, SQL_MODE='NO_AUTO_VALUE_ON_ZERO' */;
/*!40111 SET @OLD_SQL_NOTES=@@SQL_NOTES, SQL_NOTES=0 */;


-- system のデータベース構造をダンプしています
DROP DATABASE IF EXISTS `system`;
CREATE DATABASE IF NOT EXISTS `system` /*!40100 DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci */;
USE `system`;

--  テーブル system.mst_address の構造をダンプしています
DROP TABLE IF EXISTS `mst_address`;
CREATE TABLE IF NOT EXISTS `mst_address` (
  `display_address` varchar(104) NOT NULL COMMENT '画面表示住所',
  `control_address` varchar(3) NOT NULL DEFAULT '0' COMMENT 'PLC制御住所',
  PRIMARY KEY (`display_address`),
  UNIQUE KEY `display_address` (`display_address`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- テーブル system.mst_address: ~126 rows (約) のデータをダンプしています
REPLACE INTO `mst_address` (`display_address`, `control_address`) VALUES
	('A11', '000'),
	('A12', '001'),
	('A13', '002'),
	('A14', '003'),
	('A15', '004'),
	('A16', '005'),
	('A17', '006'),
	('A18', '007'),
	('A19', '008'),
	('A21', '010'),
	('A22', '011'),
	('A23', '012'),
	('A24', '013'),
	('A25', '014'),
	('A26', '015'),
	('A27', '016'),
	('A28', '017'),
	('A29', '018'),
	('A31', '020'),
	('A32', '021'),
	('A33', '022'),
	('A34', '023'),
	('A35', '024'),
	('A36', '025'),
	('A37', '026'),
	('A38', '027'),
	('A39', '028'),
	('A41', '030'),
	('A42', '031'),
	('A43', '032'),
	('A44', '033'),
	('A45', '034'),
	('A46', '035'),
	('A47', '036'),
	('A48', '037'),
	('A49', '038'),
	('A51', '040'),
	('A52', '041'),
	('A53', '042'),
	('A54', '043'),
	('A55', '044'),
	('A56', '045'),
	('A57', '046'),
	('A58', '047'),
	('A59', '048'),
	('A61', '050'),
	('A62', '051'),
	('A63', '052'),
	('A64', '053'),
	('A65', '054'),
	('A66', '055'),
	('A67', '056'),
	('A68', '057'),
	('A69', '058'),
	('A71', '060'),
	('A72', '061'),
	('A73', '062'),
	('A74', '063'),
	('A75', '064'),
	('A76', '065'),
	('A77', '066'),
	('A78', '067'),
	('A79', '068'),
	('A81', '070'),
	('A82', '071'),
	('A83', '072'),
	('A84', '073'),
	('A85', '074'),
	('A86', '075'),
	('A87', '076'),
	('A88', '077'),
	('A89', '078'),
	('B11', '130'),
	('B12', '131'),
	('B13', '132'),
	('B14', '133'),
	('B15', '134'),
	('B16', '135'),
	('B17', '136'),
	('B18', '137'),
	('B19', '138'),
	('B21', '140'),
	('B22', '141'),
	('B23', '142'),
	('B24', '143'),
	('B25', '144'),
	('B26', '145'),
	('B27', '146'),
	('B28', '147'),
	('B29', '148'),
	('B31', '150'),
	('B32', '151'),
	('B33', '152'),
	('B34', '153'),
	('B35', '154'),
	('B36', '155'),
	('B37', '156'),
	('B38', '157'),
	('B39', '158'),
	('C11', '100'),
	('C12', '101'),
	('C13', '102'),
	('C14', '103'),
	('C15', '104'),
	('C16', '105'),
	('C17', '106'),
	('C18', '107'),
	('C19', '108'),
	('C21', '110'),
	('C22', '111'),
	('C23', '112'),
	('C24', '113'),
	('C25', '114'),
	('C26', '115'),
	('C27', '116'),
	('C28', '117'),
	('C29', '118'),
	('C31', '120'),
	('C32', '121'),
	('C33', '122'),
	('C34', '123'),
	('C35', '124'),
	('C36', '125'),
	('C37', '126'),
	('C38', '127'),
	('C39', '128');

--  テーブル system.mst_line の構造をダンプしています
DROP TABLE IF EXISTS `mst_line`;
CREATE TABLE IF NOT EXISTS `mst_line` (
  `line_id` int(4) NOT NULL AUTO_INCREMENT COMMENT 'ラインID',
  `line_name` varchar(50) NOT NULL DEFAULT '0' COMMENT 'ライン名',
  `is_frozen` tinyint(1) NOT NULL DEFAULT 0 COMMENT 'データ凍結フラグ',
  PRIMARY KEY (`line_id`),
  KEY `line_id` (`line_id`)
) ENGINE=InnoDB AUTO_INCREMENT=4 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- テーブル system.mst_line: ~3 rows (約) のデータをダンプしています
REPLACE INTO `mst_line` (`line_id`, `line_name`, `is_frozen`) VALUES
	(1, 'MC7', 0),
	(2, 'MC2', 0),
	(3, 'MC3', 0);

--  テーブル system.mst_password の構造をダンプしています
DROP TABLE IF EXISTS `mst_password`;
CREATE TABLE IF NOT EXISTS `mst_password` (
  `password` varchar(60) NOT NULL COMMENT 'システムパスワード',
  PRIMARY KEY (`password`),
  UNIQUE KEY `password` (`password`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- テーブル system.mst_password: ~1 rows (約) のデータをダンプしています
REPLACE INTO `mst_password` (`password`) VALUES
	('$2a$08$6UffR9fo4CT4iMmaqm9cNOouuBrIpyfsR4bEe0DXHDPKRho1Uio.m');

--  テーブル system.mst_program の構造をダンプしています
DROP TABLE IF EXISTS `mst_program`;
CREATE TABLE IF NOT EXISTS `mst_program` (
  `program_id` int(3) NOT NULL AUTO_INCREMENT COMMENT 'プログラム割り当てID',
  `line_id` int(4) NOT NULL COMMENT 'ライン名',
  `program_name` varchar(50) NOT NULL DEFAULT '0' COMMENT 'プログラム番号',
  `program_print_time` timestamp NOT NULL COMMENT 'プログラムQR印刷日時',
  PRIMARY KEY (`program_id`) USING BTREE,
  UNIQUE KEY `program_assignment_id` (`program_id`) USING BTREE,
  KEY `FK_mst_program_mst_line` (`line_id`),
  CONSTRAINT `FK_mst_program_mst_line` FOREIGN KEY (`line_id`) REFERENCES `mst_line` (`line_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB AUTO_INCREMENT=2 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- テーブル system.mst_program: ~1 rows (約) のデータをダンプしています
REPLACE INTO `mst_program` (`program_id`, `line_id`, `program_name`, `program_print_time`) VALUES
	(1, 1, 'O0201', '2025-12-16 08:23:22');

--  テーブル system.mst_tool の構造をダンプしています
DROP TABLE IF EXISTS `mst_tool`;
CREATE TABLE IF NOT EXISTS `mst_tool` (
  `basic_tool_id` int(4) NOT NULL AUTO_INCREMENT COMMENT '基本工具ID',
  `tool_name` varchar(50) NOT NULL DEFAULT '0' COMMENT '型番',
  `maker` varchar(50) NOT NULL DEFAULT '0' COMMENT 'メーカー',
  `tool_category` enum('ドリル','インサート','タップ','リーマー','エンドミル') NOT NULL COMMENT '工具分類',
  `tool_material` varchar(50) NOT NULL DEFAULT '0' COMMENT '材質',
  `stc` int(4) DEFAULT NULL COMMENT '在庫数',
  `rop` int(4) DEFAULT NULL COMMENT '発注点',
  `buyer` varchar(50) NOT NULL DEFAULT '0' COMMENT '購入商社',
  `is_frozen` tinyint(1) NOT NULL DEFAULT 0 COMMENT 'データ凍結フラグ',
  PRIMARY KEY (`basic_tool_id`),
  UNIQUE KEY `basic_tool_id` (`basic_tool_id`)
) ENGINE=InnoDB AUTO_INCREMENT=40002 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- テーブル system.mst_tool: ~2 rows (約) のデータをダンプしています
REPLACE INTO `mst_tool` (`basic_tool_id`, `tool_name`, `maker`, `tool_category`, `tool_material`, `stc`, `rop`, `buyer`, `is_frozen`) VALUES
	(10001, '旋削用インサート TNMG', 'タンガロイ', 'インサート', 'サーメット', 8, 5, '商社B', 0),
	(20001, '超硬ドリル φ10', 'OSG', 'ドリル', '超硬', 5, 5, '商社A', 0);

--  テーブル system.mst_tool_assignment の構造をダンプしています
DROP TABLE IF EXISTS `mst_tool_assignment`;
CREATE TABLE IF NOT EXISTS `mst_tool_assignment` (
  `tool_assignment_id` int(4) NOT NULL AUTO_INCREMENT COMMENT '工具割当識別用ID',
  `program_id` int(3) NOT NULL COMMENT 'プログラム割り当てID',
  `tool_num` varchar(50) NOT NULL DEFAULT '0' COMMENT 'ツール番号',
  `basic_tool_id` int(4) NOT NULL COMMENT '基本工具ID',
  PRIMARY KEY (`tool_assignment_id`),
  UNIQUE KEY `tool_assignment_id` (`tool_assignment_id`),
  KEY `FK_mst_tool_assignment_mst_program` (`program_id`),
  KEY `FK_mst_tool_assignment_mst_tool` (`basic_tool_id`),
  CONSTRAINT `FK_mst_tool_assignment_mst_program` FOREIGN KEY (`program_id`) REFERENCES `mst_program` (`program_id`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `FK_mst_tool_assignment_mst_tool` FOREIGN KEY (`basic_tool_id`) REFERENCES `mst_tool` (`basic_tool_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB AUTO_INCREMENT=3 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- テーブル system.mst_tool_assignment: ~2 rows (約) のデータをダンプしています
REPLACE INTO `mst_tool_assignment` (`tool_assignment_id`, `program_id`, `tool_num`, `basic_tool_id`) VALUES
	(1, 1, 'T01', 10001),
	(2, 1, 'T02', 20001);

--  テーブル system.mst_user の構造をダンプしています
DROP TABLE IF EXISTS `mst_user`;
CREATE TABLE IF NOT EXISTS `mst_user` (
  `user_id` int(4) NOT NULL AUTO_INCREMENT COMMENT 'ユーザーID',
  `user_name` varchar(50) NOT NULL DEFAULT '0' COMMENT 'ユーザー名',
  `user_print_time` timestamp NOT NULL COMMENT 'ユーザーQR印刷日時',
  `per_add` tinyint(1) NOT NULL DEFAULT 0 COMMENT '補充権限',
  `per_inventory` tinyint(1) NOT NULL DEFAULT 0 COMMENT '棚卸し権限',
  `per_user` tinyint(1) NOT NULL DEFAULT 0 COMMENT 'ユーザー権限',
  `per_tool` tinyint(1) NOT NULL DEFAULT 0 COMMENT '工具管理権限',
  `per_line` tinyint(1) NOT NULL DEFAULT 0 COMMENT 'ライン管理権限',
  `per_analysis` tinyint(1) NOT NULL DEFAULT 0 COMMENT '工具分析権限',
  `per_history` tinyint(1) NOT NULL DEFAULT 0 COMMENT '操作履歴権限',
  `per_db` tinyint(1) NOT NULL DEFAULT 0 COMMENT 'データベース管理権限',
  `per_setting` tinyint(1) NOT NULL DEFAULT 0 COMMENT 'パスワード設定',
  `is_frozen` tinyint(1) NOT NULL DEFAULT 0 COMMENT 'データ凍結フラグ',
  PRIMARY KEY (`user_id`),
  UNIQUE KEY `user_id` (`user_id`)
) ENGINE=InnoDB AUTO_INCREMENT=3 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- テーブル system.mst_user: ~2 rows (約) のデータをダンプしています
REPLACE INTO `mst_user` (`user_id`, `user_name`, `user_print_time`, `per_add`, `per_inventory`, `per_user`, `per_tool`, `per_line`, `per_analysis`, `per_history`, `per_db`, `per_setting`, `is_frozen`) VALUES
	(1, '管理者ユーザー', '2025-12-16 08:23:22', 1, 1, 1, 1, 1, 1, 1, 1, 1, 0),
	(2, '佐竹', '2025-12-16 08:23:22', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

--  テーブル system.rec_operation の構造をダンプしています
DROP TABLE IF EXISTS `rec_operation`;
CREATE TABLE IF NOT EXISTS `rec_operation` (
  `operation_id` int(4) NOT NULL AUTO_INCREMENT COMMENT '記録用連番',
  `operation_class` enum('補充','取出','棚卸') NOT NULL COMMENT '履歴分類名称',
  `user_id` int(4) NOT NULL COMMENT 'ユーザー識別用連番',
  `proc_time` timestamp NOT NULL COMMENT '処理日時',
  `storage_area_id` int(4) NOT NULL COMMENT '工具保管領域識別用連番',
  `unique_tool_id` bigint(10) DEFAULT NULL COMMENT '工具識別用連番',
  `video_path` varchar(50) NOT NULL DEFAULT '0' COMMENT '動画の保存先リンク',
  `tool_num` int(2) DEFAULT 0 COMMENT '操作個数',
  `line_id` int(4) DEFAULT NULL COMMENT 'ラインID',
  `track_result` enum('あり','なし') DEFAULT NULL COMMENT '工具追跡の結果',
  PRIMARY KEY (`operation_id`) USING BTREE,
  UNIQUE KEY `opereation_id` (`operation_id`) USING BTREE,
  KEY `FK_rec_operation_mst_user` (`user_id`),
  KEY `FK_rec_operation_mst_line` (`line_id`),
  KEY `FK_rec_operation_storage_area` (`storage_area_id`),
  KEY `FK_rec_operation_unique_tool` (`unique_tool_id`),
  CONSTRAINT `FK_rec_operation_mst_line` FOREIGN KEY (`line_id`) REFERENCES `mst_line` (`line_id`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `FK_rec_operation_mst_user` FOREIGN KEY (`user_id`) REFERENCES `mst_user` (`user_id`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `FK_rec_operation_storage_area` FOREIGN KEY (`storage_area_id`) REFERENCES `storage_area` (`storage_area_id`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `FK_rec_operation_unique_tool` FOREIGN KEY (`unique_tool_id`) REFERENCES `unique_tool` (`unique_tool_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB AUTO_INCREMENT=12 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- テーブル system.rec_operation: ~9 rows (約) のデータをダンプしています
REPLACE INTO `rec_operation` (`operation_id`, `operation_class`, `user_id`, `proc_time`, `storage_area_id`, `unique_tool_id`, `video_path`, `tool_num`, `line_id`, `track_result`) VALUES
	(1, '補充', 1, '2026-01-10 04:12:51', 1, 2000100001, '0', 1, NULL, NULL),
	(2, '補充', 1, '2026-01-11 04:12:51', 2, 1000100001, '0', 10, NULL, NULL),
	(3, '取出', 1, '2026-01-12 04:12:51', 2, 1000100001, '0', -2, NULL, 'なし'),
	(6, '補充', 1, '2026-01-15 02:12:51', 1, 2000100002, '0', 1, NULL, NULL),
	(7, '取出', 1, '2026-01-15 03:12:51', 1, 2000100002, '0', -1, NULL, 'あり'),
	(8, '補充', 1, '2026-01-15 13:28:44', 1, 2000100003, '0', 1, NULL, NULL),
	(9, '補充', 1, '2026-01-15 13:29:36', 1, 2000100004, '0', 1, NULL, NULL),
	(10, '補充', 1, '2026-01-15 13:30:01', 1, 2000100005, '0', 1, NULL, NULL),
	(11, '補充', 1, '2026-01-15 13:30:22', 1, 2000100006, '0', 1, NULL, NULL);

--  テーブル system.rec_stock の構造をダンプしています
DROP TABLE IF EXISTS `rec_stock`;
CREATE TABLE IF NOT EXISTS `rec_stock` (
  `stock_id` int(8) NOT NULL AUTO_INCREMENT COMMENT '記録用連番',
  `register_year_month` varchar(50) NOT NULL DEFAULT '' COMMENT '年月',
  `basic_tool_id` int(4) NOT NULL COMMENT '工具種別ごとの連番',
  `out_num` int(4) NOT NULL DEFAULT 0 COMMENT '取り出し個数（期間末）',
  `end_period_stock` int(4) NOT NULL DEFAULT 0 COMMENT '期間末在庫数',
  PRIMARY KEY (`stock_id`),
  UNIQUE KEY `stock_id` (`stock_id`),
  UNIQUE KEY `インデックス 4` (`register_year_month`,`basic_tool_id`) USING BTREE,
  KEY `FK_rec_stock_mst_tool` (`basic_tool_id`),
  CONSTRAINT `FK_rec_stock_mst_tool` FOREIGN KEY (`basic_tool_id`) REFERENCES `mst_tool` (`basic_tool_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB AUTO_INCREMENT=4 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- テーブル system.rec_stock: ~2 rows (約) のデータをダンプしています
REPLACE INTO `rec_stock` (`stock_id`, `register_year_month`, `basic_tool_id`, `out_num`, `end_period_stock`) VALUES
	(1, '2026-01', 20001, 1, 5),
	(2, '2026-01', 10001, 2, 8);

--  テーブル system.storage_area の構造をダンプしています
DROP TABLE IF EXISTS `storage_area`;
CREATE TABLE IF NOT EXISTS `storage_area` (
  `storage_area_id` int(4) NOT NULL AUTO_INCREMENT COMMENT '保管領域ID',
  `basic_tool_id` int(4) NOT NULL COMMENT '基本工具ID',
  `toolcase_stc_num` int(2) DEFAULT NULL COMMENT '保管ケース個数',
  `display_address` varchar(104) NOT NULL COMMENT '画面表示住所',
  `last_reconciliation_time` timestamp NULL DEFAULT NULL,
  PRIMARY KEY (`storage_area_id`),
  UNIQUE KEY `storage_area_id` (`storage_area_id`),
  KEY `FK_storage_area_mst_tool` (`basic_tool_id`),
  KEY `FK_storage_area_mst_address` (`display_address`),
  CONSTRAINT `FK_storage_area_mst_address` FOREIGN KEY (`display_address`) REFERENCES `mst_address` (`display_address`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `FK_storage_area_mst_tool` FOREIGN KEY (`basic_tool_id`) REFERENCES `mst_tool` (`basic_tool_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB AUTO_INCREMENT=5 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- テーブル system.storage_area: ~2 rows (約) のデータをダンプしています
REPLACE INTO `storage_area` (`storage_area_id`, `basic_tool_id`, `toolcase_stc_num`, `display_address`, `last_reconciliation_time`) VALUES
	(1, 20001, 5, 'A11', NULL),
	(2, 10001, 1, 'B39', NULL);

--  テーブル system.unique_tool の構造をダンプしています
DROP TABLE IF EXISTS `unique_tool`;
CREATE TABLE IF NOT EXISTS `unique_tool` (
  `unique_tool_id` bigint(10) NOT NULL COMMENT '個別工具ID',
  `tool_print_time` timestamp NOT NULL COMMENT '個別工具QR印刷日時',
  `basic_tool_id` int(4) NOT NULL COMMENT '基本工具ID',
  `unique_num` int(6) NOT NULL COMMENT '個別番号',
  `case_pack_num` int(2) NOT NULL COMMENT 'ケース内個数',
  `storage_area_id` int(4) DEFAULT NULL COMMENT '保管領域ID',
  `storage_condition` enum('補充前','保管中','取出') NOT NULL DEFAULT '補充前' COMMENT '保管状態',
  `regrind_count` int(2) DEFAULT NULL COMMENT '工具の再研磨回数',
  PRIMARY KEY (`unique_tool_id`),
  UNIQUE KEY `unique_tool` (`unique_tool_id`),
  KEY `FK_unique_tool_mst_tool` (`basic_tool_id`),
  KEY `FK_unique_tool_storage_area` (`storage_area_id`),
  CONSTRAINT `FK_unique_tool_mst_tool` FOREIGN KEY (`basic_tool_id`) REFERENCES `mst_tool` (`basic_tool_id`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `FK_unique_tool_storage_area` FOREIGN KEY (`storage_area_id`) REFERENCES `storage_area` (`storage_area_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- テーブル system.unique_tool: ~8 rows (約) のデータをダンプしています
REPLACE INTO `unique_tool` (`unique_tool_id`, `tool_print_time`, `basic_tool_id`, `unique_num`, `case_pack_num`, `storage_area_id`, `storage_condition`, `regrind_count`) VALUES
	(1000100001, '2025-12-16 08:23:22', 10001, 1, 8, 2, '保管中', 0),
	(1000100002, '2025-12-16 08:23:22', 10001, 2, 10, NULL, '補充前', 0),
	(2000100001, '2025-12-16 08:23:22', 20001, 1, 1, 1, '保管中', 0),
	(2000100002, '2025-12-16 08:23:22', 20001, 2, 0, NULL, '取出', 0),
	(2000100003, '2026-01-15 13:25:53', 20001, 3, 1, 1, '保管中', 0),
	(2000100004, '2026-01-15 13:26:41', 20001, 4, 1, 1, '保管中', 0),
	(2000100005, '2026-01-15 13:27:37', 20001, 5, 1, 1, '保管中', 0),
	(2000100006, '2026-01-15 13:28:05', 20001, 6, 1, 1, '保管中', 0);

-- --------------------------------------------------------
-- データ拡張用SQL (2025/12月度データ & 工具種類追加)
-- --------------------------------------------------------

USE `system`;

-- 1. mst_tool: 工具を10種類まで増やす (既存ID: 10001, 20001)
REPLACE INTO `mst_tool` (`basic_tool_id`, `tool_name`, `maker`, `tool_category`, `tool_material`, `stc`, `rop`, `buyer`, `is_frozen`) VALUES
	(30001, '超硬エンドミル φ8', 'OSG', 'エンドミル', '超硬', 10, 3, '商社A', 0),
	(40001, 'スパイラルタップ M6', 'ヤマワ', 'タップ', 'HSS', 15, 5, '商社B', 0),
	(50001, 'ハンドリーマー 10H7', '岡崎精工', 'リーマー', 'HSS', 3, 1, '商社A', 0),
	(60001, '旋削用チップ CCMT', '三菱', 'インサート', '超硬', 20, 10, '商社C', 0),
	(70001, '超硬ドリル φ5', '不二越', 'ドリル', '超硬', 8, 3, '商社A', 0),
	(80001, 'フェイスミル φ50', '京セラ', 'エンドミル', '超硬', 2, 1, '商社B', 0),
	(90001, 'ボールエンドミル R5', '日進工具', 'エンドミル', '超硬', 5, 2, '商社C', 0),
	(90002, 'ポイントタップ M10', 'ヤマワ', 'タップ', 'HSS', 12, 4, '商社A', 0);

-- 2. storage_area: 新しい工具の保管場所を確保 (既存ID: 1, 2)
-- アドレス A12 ～ A19 を使用
REPLACE INTO `storage_area` (`storage_area_id`, `basic_tool_id`, `toolcase_stc_num`, `display_address`, `last_reconciliation_time`) VALUES
	(3, 30001, 10, 'A12', '2025-12-20 17:00:00'),
	(4, 40001, 15, 'A13', '2025-12-20 17:00:00'),
	(5, 50001, 3,  'A14', '2025-12-20 17:00:00'),
	(6, 60001, 20, 'A15', '2025-12-20 17:00:00'),
	(7, 70001, 8,  'A16', '2025-12-20 17:00:00'),
	(8, 80001, 2,  'A17', '2025-12-20 17:00:00'),
	(9, 90001, 5,  'A18', '2025-12-20 17:00:00'),
	(10, 90002, 12,'A19', '2025-12-20 17:00:00');

-- 3. unique_tool: 新しい工具の実体データを作成
-- IDルール: basic_tool_id + 00001
REPLACE INTO `unique_tool` (`unique_tool_id`, `tool_print_time`, `basic_tool_id`, `unique_num`, `case_pack_num`, `storage_area_id`, `storage_condition`, `regrind_count`) VALUES
	(3000100001, '2025-12-01 09:00:00', 30001, 1, 10, 3, '保管中', 0),
	(4000100001, '2025-12-01 09:00:00', 40001, 1, 15, 4, '保管中', 0),
	(5000100001, '2025-12-01 09:00:00', 50001, 1, 3,  5, '保管中', 0),
	(6000100001, '2025-12-01 09:00:00', 60001, 1, 20, 6, '保管中', 0),
	(7000100001, '2025-12-01 09:00:00', 70001, 1, 8,  7, '保管中', 0),
	(8000100001, '2025-12-01 09:00:00', 80001, 1, 2,  8, '保管中', 0),
	(9000100001, '2025-12-01 09:00:00', 90001, 1, 5,  9, '保管中', 0),
	(9000200001, '2025-12-01 09:00:00', 90002, 1, 12, 10, '保管中', 0);

-- 4. rec_operation: 2025年12月の入庫履歴を作成 (履歴ID 100番台を使用)
-- ※トリガーが有効な状態で流すと重複計算される恐れがあるため、
--   ダンプデータの復元としては下記のようにREPLACE INTOで履歴だけ記録しておくのが安全です。
REPLACE INTO `rec_operation` (`operation_id`, `operation_class`, `user_id`, `proc_time`, `storage_area_id`, `unique_tool_id`, `video_path`, `tool_num`, `line_id`, `track_result`) VALUES
	(101, '補充', 1, '2025-12-05 10:00:00', 3, 3000100001, '0', 10, NULL, NULL),
	(102, '補充', 1, '2025-12-05 10:05:00', 4, 4000100001, '0', 15, NULL, NULL),
	(103, '補充', 1, '2025-12-05 10:10:00', 5, 5000100001, '0', 3,  NULL, NULL),
	(104, '補充', 1, '2025-12-05 10:15:00', 6, 6000100001, '0', 20, NULL, NULL),
	(105, '補充', 1, '2025-12-05 10:20:00', 7, 7000100001, '0', 8,  NULL, NULL),
	(106, '補充', 1, '2025-12-05 10:25:00', 8, 8000100001, '0', 2,  NULL, NULL),
	(107, '補充', 1, '2025-12-05 10:30:00', 9, 9000100001, '0', 5,  NULL, NULL),
	(108, '補充', 1, '2025-12-05 10:35:00', 10, 9000200001, '0', 12, NULL, NULL);

-- 5. rec_stock: 2025-12月度の在庫データを全10工具分作成
-- 既存2種(10001, 20001) + 新規8種
-- out_num(出庫数)はダミーでランダムに入力、end_period_stock(月末在庫)はstcと合わせるか調整
REPLACE INTO `rec_stock` (`register_year_month`, `basic_tool_id`, `out_num`, `end_period_stock`) VALUES
	('2025-12', 10001, 5, 10), -- 既存1: 12月末は10個だったと仮定
	('2025-12', 20001, 2, 6),  -- 既存2: 12月末は6個だったと仮定
	('2025-12', 30001, 0, 10),
	('2025-12', 40001, 3, 15),
	('2025-12', 50001, 0, 3),
	('2025-12', 60001, 10, 20),
	('2025-12', 70001, 2, 8),
	('2025-12', 80001, 1, 2),
	('2025-12', 90001, 0, 5),
	('2025-12', 90002, 5, 12);


--  トリガー system.mst_tool_after_update の構造をダンプしています
DROP TRIGGER IF EXISTS `mst_tool_after_update`;
SET @OLDTMP_SQL_MODE=@@SQL_MODE, SQL_MODE='STRICT_TRANS_TABLES,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION';
DELIMITER //
CREATE TRIGGER `mst_tool_after_update` AFTER UPDATE ON `mst_tool` FOR EACH ROW BEGIN
    DECLARE v_year_month VARCHAR(50);
    DECLARE v_row_exists INT;

    IF NOT (NEW.stc <=> OLD.stc) THEN

        SET v_year_month = DATE_FORMAT(NOW(), '%Y-%m');

        SELECT COUNT(*) INTO v_row_exists
        FROM rec_stock
        WHERE basic_tool_id = NEW.basic_tool_id
        AND register_year_month = v_year_month;

        IF v_row_exists > 0 THEN
            UPDATE rec_stock
            SET end_period_stock = COALESCE(NEW.stc, 0)
            WHERE basic_tool_id = NEW.basic_tool_id
            AND register_year_month = v_year_month;
        ELSE
            INSERT INTO rec_stock (
                register_year_month,
                basic_tool_id,
                end_period_stock,
                out_num
            ) VALUES (
                v_year_month,
                NEW.basic_tool_id,
                COALESCE(NEW.stc, 0),
                0
            );
        END IF;
    END IF;
END//
DELIMITER ;
SET SQL_MODE=@OLDTMP_SQL_MODE;

--  トリガー system.rec_operation_after_insert の構造をダンプしています
DROP TRIGGER IF EXISTS `rec_operation_after_insert`;
SET @OLDTMP_SQL_MODE=@@SQL_MODE, SQL_MODE='STRICT_TRANS_TABLES,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION';
DELIMITER //
CREATE TRIGGER `rec_operation_after_insert` AFTER INSERT ON `rec_operation` FOR EACH ROW main_block: BEGIN
    DECLARE v_current_storage_condition VARCHAR(255);
    DECLARE v_current_case_pack_num INT;

    IF NEW.tool_num IS NULL OR NEW.unique_tool_id IS NULL THEN
        LEAVE main_block;
    END IF;

    SELECT storage_condition, case_pack_num
    INTO v_current_storage_condition, v_current_case_pack_num
    FROM unique_tool
    WHERE unique_tool_id = NEW.unique_tool_id;

    IF NEW.tool_num >= 0 THEN   -- 補充
        IF v_current_storage_condition = '補充前' OR v_current_storage_condition = '取出' THEN
            UPDATE storage_area
            SET toolcase_stc_num = toolcase_stc_num + 1
            WHERE storage_area_id = NEW.storage_area_id
            AND toolcase_stc_num IS NOT NULL;

            UPDATE unique_tool
            SET storage_condition = '保管中',
                case_pack_num = NEW.tool_num,
                storage_area_id = NEW.storage_area_id
            WHERE unique_tool_id = NEW.unique_tool_id;

        ELSEIF v_current_storage_condition = '保管中' THEN
            UPDATE unique_tool
            SET case_pack_num = case_pack_num + NEW.tool_num
            WHERE unique_tool_id = NEW.unique_tool_id;
        END IF;

    ELSE    -- 取出
        IF (v_current_case_pack_num + NEW.tool_num) <= 0 THEN
            UPDATE unique_tool
            SET storage_condition = '取出',
                case_pack_num = 0,
                storage_area_id = null
            WHERE unique_tool_id = NEW.unique_tool_id;

            UPDATE storage_area
            SET toolcase_stc_num = toolcase_stc_num - 1
            WHERE storage_area_id = NEW.storage_area_id
            AND toolcase_stc_num IS NOT NULL;

        ELSE
            UPDATE unique_tool
            SET case_pack_num = case_pack_num + NEW.tool_num
            WHERE unique_tool_id = NEW.unique_tool_id;
        END IF;
    END IF;
END//
DELIMITER ;
SET SQL_MODE=@OLDTMP_SQL_MODE;

--  トリガー system.rec_operation_after_insert(2) の構造をダンプしています
DROP TRIGGER IF EXISTS `rec_operation_after_insert(2)`;
SET @OLDTMP_SQL_MODE=@@SQL_MODE, SQL_MODE='STRICT_TRANS_TABLES,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION';
DELIMITER //
CREATE TRIGGER `rec_operation_after_insert(2)` AFTER INSERT ON `rec_operation` FOR EACH ROW main_block: BEGIN
    DECLARE v_basic_tool_id INT;

    IF NEW.tool_num IS NULL OR NEW.unique_tool_id IS NULL THEN
        LEAVE main_block;
    END IF;

    SELECT basic_tool_id
    INTO v_basic_tool_id
    FROM unique_tool
    WHERE unique_tool_id = NEW.unique_tool_id;

    UPDATE mst_tool
    SET stc = stc + NEW.tool_num
    WHERE basic_tool_id = v_basic_tool_id
    AND stc IS NOT NULL;
END//
DELIMITER ;
SET SQL_MODE=@OLDTMP_SQL_MODE;

--  トリガー system.rec_operation_after_insert(3) の構造をダンプしています
DROP TRIGGER IF EXISTS `rec_operation_after_insert(3)`;
SET @OLDTMP_SQL_MODE=@@SQL_MODE, SQL_MODE='STRICT_TRANS_TABLES,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION';
DELIMITER //
CREATE TRIGGER `rec_operation_after_insert(3)` AFTER INSERT ON `rec_operation` FOR EACH ROW BEGIN
    IF NEW.operation_class = '棚卸' THEN
        UPDATE storage_area
        SET last_reconciliation_time = NEW.proc_time
        WHERE storage_area_id = NEW.storage_area_id;
    END IF;
END//
DELIMITER ;
SET SQL_MODE=@OLDTMP_SQL_MODE;

--  トリガー system.rec_operation_after_insert(4) の構造をダンプしています
DROP TRIGGER IF EXISTS `rec_operation_after_insert(4)`;
SET @OLDTMP_SQL_MODE=@@SQL_MODE, SQL_MODE='STRICT_TRANS_TABLES,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION';
DELIMITER //
CREATE TRIGGER `rec_operation_after_insert(4)` AFTER INSERT ON `rec_operation` FOR EACH ROW BEGIN
    DECLARE v_basic_tool_id INT;
    DECLARE v_year_month VARCHAR(7);
    DECLARE v_count INT;

    IF NEW.operation_class = '取出' AND NEW.tool_num < 0 THEN

        SELECT basic_tool_id
        INTO v_basic_tool_id
        FROM unique_tool
        WHERE unique_tool_id = NEW.unique_tool_id;

        SET v_year_month = DATE_FORMAT(NEW.proc_time, '%Y-%m');

        SELECT COUNT(*) INTO v_count
        FROM rec_stock
        WHERE basic_tool_id = v_basic_tool_id
        AND register_year_month = v_year_month;

        IF v_count > 0 THEN
            UPDATE rec_stock
            SET out_num = out_num + ABS(NEW.tool_num)
            WHERE basic_tool_id = v_basic_tool_id
            AND register_year_month = v_year_month;
        ELSE
            INSERT INTO rec_stock (
                register_year_month,
                basic_tool_id,
                end_period_stock,
                out_num
            ) VALUES (
                v_year_month,
                v_basic_tool_id,
                0,
                ABS(NEW.tool_num)
            );
        END IF;

    END IF;
END//
DELIMITER ;
SET SQL_MODE=@OLDTMP_SQL_MODE;

--  トリガー system.rec_operation_before_insert の構造をダンプしています
DROP TRIGGER IF EXISTS `rec_operation_before_insert`;
SET @OLDTMP_SQL_MODE=@@SQL_MODE, SQL_MODE='STRICT_TRANS_TABLES,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION';
DELIMITER //
CREATE TRIGGER `rec_operation_before_insert` BEFORE INSERT ON `rec_operation` FOR EACH ROW BEGIN
    IF (NEW.operation_class = '取出' AND NEW.tool_num >= 0)
    OR (NEW.operation_class = '補充' AND NEW.tool_num < 0) THEN
        SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = '操作クラスと操作個数の符号が矛盾しています。';
    END IF;
END//
DELIMITER ;
SET SQL_MODE=@OLDTMP_SQL_MODE;

--  トリガー system.storage_area_before_insert の構造をダンプしています
DROP TRIGGER IF EXISTS `storage_area_before_insert`;
SET @OLDTMP_SQL_MODE=@@SQL_MODE, SQL_MODE='STRICT_TRANS_TABLES,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION';
DELIMITER //
CREATE TRIGGER `storage_area_before_insert` BEFORE INSERT ON `storage_area` FOR EACH ROW BEGIN
    DECLARE v_stc INT;
    DECLARE v_rop INT;
    DECLARE v_existing_count INT DEFAULT 0;

    -- 1. 挿入する工具の stc と rop を取得
    SELECT stc, rop
    INTO v_stc, v_rop
    FROM mst_tool
    WHERE basic_tool_id = NEW.basic_tool_id;

    -- 2. 条件分岐
    -- パターンA: 在庫数(stc)が0 かつ 発注点(rop)が0以上 の場合
    IF (v_stc = 0 AND v_rop >= 0) THEN

        -- 同じ場所に、同じ条件(stc=0, rop>=0)の工具がいくつあるかカウント
        SELECT COUNT(*)
        INTO v_existing_count
        FROM storage_area sa
        INNER JOIN mst_tool mt ON sa.basic_tool_id = mt.basic_tool_id
        WHERE sa.display_address = NEW.display_address
          AND is_frozen = 0;


        -- 既に2個以上あればエラー (今回足すと3個になる、または既storage_area_before_insertに満杯)
        IF v_existing_count = 2 THEN
            SIGNAL SQLSTATE '45000'
            SET MESSAGE_TEXT = 'このコンテナには管理対象の工具が既に2件あるため追加できません';
        END IF;

    -- パターンB: 在庫数も発注点も NULL の場合
    ELSEIF (v_stc IS NULL AND v_rop IS NULL) THEN
        -- こちらは「処理を次に進める」だけで、個数制限チェックはしない（＝許可）
        BEGIN END;

    -- パターンC: 上記以外の工具の場合
    ELSE
        -- もし「これら以外の工具は登録させない」のであれば、ここでエラーを出します。
        -- 制限しなくてよい（素通し）なら、このELSEブロックは削除してください。
        SIGNAL SQLSTATE '45000' SET MESSAGE_TEXT = '許可されていない入力情報です';
    END IF;

END//
DELIMITER ;
SET SQL_MODE=@OLDTMP_SQL_MODE;

/*!40103 SET TIME_ZONE=IFNULL(@OLD_TIME_ZONE, 'system') */;
/*!40101 SET SQL_MODE=IFNULL(@OLD_SQL_MODE, '') */;
/*!40014 SET FOREIGN_KEY_CHECKS=IFNULL(@OLD_FOREIGN_KEY_CHECKS, 1) */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40111 SET SQL_NOTES=IFNULL(@OLD_SQL_NOTES, 1) */;
