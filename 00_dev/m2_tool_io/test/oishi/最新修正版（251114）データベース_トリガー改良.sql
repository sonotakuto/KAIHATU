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


-- system1114 のデータベース構造をダンプしています
CREATE DATABASE IF NOT EXISTS `system1114` /*!40100 DEFAULT CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci */;
USE `system1114`;

--  テーブル system1114.mst_address の構造をダンプしています
CREATE TABLE IF NOT EXISTS `mst_address` (
  `display_address` varchar(304) NOT NULL COMMENT '画面表示住所',
  `control_address` varchar(3) NOT NULL DEFAULT '0' COMMENT 'PLC制御住所',
  PRIMARY KEY (`display_address`),
  UNIQUE KEY `display_address` (`display_address`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- テーブル system1114.mst_address: ~1 rows (約) のデータをダンプしています
DELETE FROM `mst_address`;
INSERT INTO `mst_address` (`display_address`, `control_address`) VALUES
	('A01', '12');

--  テーブル system1114.mst_line の構造をダンプしています
CREATE TABLE IF NOT EXISTS `mst_line` (
  `line_id` int(4) unsigned zerofill NOT NULL AUTO_INCREMENT COMMENT 'ラインID',
  `line_name` varchar(50) NOT NULL DEFAULT '0' COMMENT 'ライン名',
  `is_frozen` tinyint(1) NOT NULL DEFAULT 0 COMMENT 'データ凍結フラグ',
  PRIMARY KEY (`line_id`),
  KEY `line_id` (`line_id`)
) ENGINE=InnoDB AUTO_INCREMENT=9 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- テーブル system1114.mst_line: ~8 rows (約) のデータをダンプしています
DELETE FROM `mst_line`;
INSERT INTO `mst_line` (`line_id`, `line_name`, `is_frozen`) VALUES
	(0001, 'MC1', 0),
	(0002, 'MC2', 0),
	(0003, 'MC3', 0),
	(0004, 'MC4', 0),
	(0005, 'MC5', 0),
	(0006, 'MC6', 0),
	(0007, 'MC7', 0),
	(0008, 'MC8', 0);

--  テーブル system1114.mst_password の構造をダンプしています
CREATE TABLE IF NOT EXISTS `mst_password` (
  `password` varchar(60) NOT NULL COMMENT 'システムパスワード',
  PRIMARY KEY (`password`),
  UNIQUE KEY `password` (`password`)
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- テーブル system1114.mst_password: ~0 rows (約) のデータをダンプしています
DELETE FROM `mst_password`;
INSERT INTO `mst_password` (`password`) VALUES
	('$2a$08$L1Hyqo3Cas585TYIjXSnBeh5uHo8teHI53GtD.61MS7HPFFjDFUni');

--  テーブル system1114.mst_program の構造をダンプしています
CREATE TABLE IF NOT EXISTS `mst_program` (
  `program_id` int(3) unsigned zerofill NOT NULL AUTO_INCREMENT COMMENT 'プログラム割り当てID',
  `line_id` int(4) unsigned zerofill NOT NULL COMMENT 'ライン名',
  `program_name` varchar(50) NOT NULL DEFAULT '0' COMMENT 'プログラム番号',
  `program_print_time` timestamp NOT NULL COMMENT 'プログラムQR印刷日時',
  `is_frozen` tinyint(1) NOT NULL DEFAULT 0 COMMENT 'データ凍結フラグ',
  PRIMARY KEY (`program_id`) USING BTREE,
  UNIQUE KEY `program_assignment_id` (`program_id`) USING BTREE,
  KEY `FK_mst_program_mst_line` (`line_id`),
  CONSTRAINT `FK_mst_program_mst_line` FOREIGN KEY (`line_id`) REFERENCES `mst_line` (`line_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB AUTO_INCREMENT=20 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- テーブル system1114.mst_program: ~19 rows (約) のデータをダンプしています
DELETE FROM `mst_program`;
INSERT INTO `mst_program` (`program_id`, `line_id`, `program_name`, `program_print_time`, `is_frozen`) VALUES
	(001, 0008, 'O02031J507', '2025-10-29 09:44:36', 0),
	(002, 0008, 'O02041E516', '2025-10-29 09:44:36', 0),
	(003, 0008, 'O02051K883', '2025-10-29 09:44:36', 0),
	(004, 0008, 'O02061E499', '2025-10-29 09:44:36', 0),
	(005, 0008, 'O02075T140', '2025-10-29 09:44:36', 0),
	(006, 0008, 'O02085H945', '2025-10-29 09:44:36', 0),
	(007, 0008, 'O02103B234', '2025-10-29 09:44:36', 0),
	(008, 0008, 'O02113B481', '2025-10-29 09:44:36', 0),
	(009, 0008, 'O02121J050', '2025-10-29 09:44:36', 0),
	(010, 0008, 'O02131J087', '2025-10-29 09:44:36', 0),
	(011, 0008, 'O0214EG507', '2025-10-29 09:44:36', 0),
	(012, 0008, 'O02151J080EX', '2025-10-29 09:44:36', 0),
	(013, 0008, 'O02161J080MF', '2025-10-29 09:44:36', 0),
	(014, 0008, 'O02171J696', '2025-10-29 09:44:36', 0),
	(015, 0008, 'O02181J804EX', '2025-10-29 09:44:36', 0),
	(016, 0008, 'O0219EG505EX', '2025-10-29 09:44:36', 0),
	(017, 0008, 'O02201A455EX', '2025-10-29 09:44:36', 0),
	(018, 0008, 'O02215230EX', '2025-10-29 09:44:36', 0),
	(019, 0008, 'O02221J825MF', '2025-10-29 09:44:36', 0);

--  テーブル system1114.mst_tool の構造をダンプしています
CREATE TABLE IF NOT EXISTS `mst_tool` (
  `basic_tool_id` int(4) unsigned zerofill NOT NULL AUTO_INCREMENT COMMENT '基本工具ID',
  `tool_name` varchar(50) NOT NULL DEFAULT '0' COMMENT '型番',
  `maker` varchar(50) NOT NULL DEFAULT '0' COMMENT 'メーカー',
  `tool_category` enum('ドリル','インサート','タップ','リーマー','エンドミル') NOT NULL COMMENT '工具分類',
  `tool_material` varchar(50) NOT NULL DEFAULT '0' COMMENT '材質',
  `stc` int(4) DEFAULT NULL COMMENT '在庫数',
  `rop` int(4) NOT NULL DEFAULT 0 COMMENT '発注点',
  `buyer` varchar(50) NOT NULL DEFAULT '0' COMMENT '購入商社',
  `is_frozen` tinyint(1) NOT NULL DEFAULT 0 COMMENT 'データ凍結フラグ',
  PRIMARY KEY (`basic_tool_id`),
  UNIQUE KEY `basic_tool_id` (`basic_tool_id`)
) ENGINE=InnoDB AUTO_INCREMENT=16 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- テーブル system1114.mst_tool: ~13 rows (約) のデータをダンプしています
DELETE FROM `mst_tool`;
INSERT INTO `mst_tool` (`basic_tool_id`, `tool_name`, `maker`, `tool_category`, `tool_material`, `stc`, `rop`, `buyer`, `is_frozen`) VALUES
	(0001, 'testkimura', 'kimura', 'ドリル', '超硬', 2, 0, 'kimura', 0),
	(0002, 'uehehata', 'uehata', 'タップ', '超硬', 2, 3, 'kimura', 0),
	(0003, 'kubo', 'kubo', 'ドリル', '超硬', 6, 0, 'kimura', 0),
	(0004, 'demo1', '木村ツール', 'インサート', 'T515', 10, 0, 'ポリテク工業', 0),
	(0005, 'demo2', '木村ツール', 'インサート', 'AH120', 5, 0, 'ポリテク工業', 0),
	(0006, 'demo3', '木村ツール', 'インサート', 'AH120', 5, 0, 'ポリテク工業', 0),
	(0007, 'demo4', '木村ツール', 'インサート', 'T1215', 5, 0, 'ポリテク工業', 0),
	(0008, 'demo5', '木村ツール', 'インサート', 'AH120', 5, 0, 'ポリテク工業', 0),
	(0009, 'demo6', '宗像プロダクト', 'インサート', 'AH120', 5, 0, 'ポリテク工業', 0),
	(0010, 'demo7', '宗像プロダクト', 'インサート', 'ACP3000', 5, 0, 'ポリテク工業', 0),
	(0011, 'demo8', '宗像プロダクト', 'インサート', 'ACU2500', 5, 0, 'ポリテク工業', 0),
	(0012, 'demo9', '宗像プロダクト', 'インサート', 'AH120', 5, 0, 'ポリテク工業', 0),
	(0013, 'demo10', '宗像プロダクト', 'インサート', 'AH3225', 5, 0, 'ポリテク工業', 0),
	(0015, 'test', 'test', 'インサート', 'test', NULL, 0, 'test', 0);

--  テーブル system1114.mst_tool_assignment の構造をダンプしています
CREATE TABLE IF NOT EXISTS `mst_tool_assignment` (
  `tool_assignment_id` int(4) unsigned zerofill NOT NULL AUTO_INCREMENT COMMENT '工具割当識別用ID',
  `program_id` int(3) unsigned zerofill NOT NULL COMMENT 'プログラム割り当てID',
  `tool_num` varchar(50) NOT NULL DEFAULT '0' COMMENT 'ツール番号',
  `basic_tool_id` int(4) unsigned zerofill NOT NULL COMMENT '基本工具ID',
  PRIMARY KEY (`tool_assignment_id`),
  UNIQUE KEY `tool_assignment_id` (`tool_assignment_id`),
  KEY `FK_mst_tool_assignment_mst_program` (`program_id`),
  KEY `FK_mst_tool_assignment_mst_tool` (`basic_tool_id`),
  CONSTRAINT `FK_mst_tool_assignment_mst_program` FOREIGN KEY (`program_id`) REFERENCES `mst_program` (`program_id`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `FK_mst_tool_assignment_mst_tool` FOREIGN KEY (`basic_tool_id`) REFERENCES `mst_tool` (`basic_tool_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB AUTO_INCREMENT=11 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- テーブル system1114.mst_tool_assignment: ~10 rows (約) のデータをダンプしています
DELETE FROM `mst_tool_assignment`;
INSERT INTO `mst_tool_assignment` (`tool_assignment_id`, `program_id`, `tool_num`, `basic_tool_id`) VALUES
	(0001, 001, 'T1', 0004),
	(0002, 001, 'T2', 0005),
	(0003, 001, 'T3', 0006),
	(0004, 001, 'T4', 0007),
	(0005, 001, 'T5', 0008),
	(0006, 001, 'T6', 0009),
	(0007, 001, 'T7', 0010),
	(0008, 001, 'T8', 0011),
	(0009, 001, 'T9', 0012),
	(0010, 001, 'T10', 0013);

--  テーブル system1114.mst_user の構造をダンプしています
CREATE TABLE IF NOT EXISTS `mst_user` (
  `user_id` int(4) unsigned zerofill NOT NULL AUTO_INCREMENT COMMENT 'ユーザーID',
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

-- テーブル system1114.mst_user: ~2 rows (約) のデータをダンプしています
DELETE FROM `mst_user`;
INSERT INTO `mst_user` (`user_id`, `user_name`, `user_print_time`, `per_add`, `per_inventory`, `per_user`, `per_tool`, `per_line`, `per_analysis`, `per_history`, `per_db`, `per_setting`, `is_frozen`) VALUES
	(0001, 'kimura', '2025-10-14 23:47:26', 1, 1, 1, 1, 1, 1, 1, 1, 0, 0),
	(0002, 'uehta', '2025-10-22 04:36:07', 1, 1, 1, 0, 0, 0, 0, 0, 0, 0);

--  テーブル system1114.rec_operation の構造をダンプしています
CREATE TABLE IF NOT EXISTS `rec_operation` (
  `operation_id` int(4) NOT NULL AUTO_INCREMENT COMMENT '記録用連番',
  `operation_class` enum('補充','取出','棚卸') NOT NULL COMMENT '履歴分類名称',
  `user_id` int(4) unsigned zerofill NOT NULL COMMENT 'ユーザー識別用連番',
  `proc_time` timestamp NOT NULL COMMENT '処理日時',
  `storage_area_id` int(4) unsigned zerofill NOT NULL COMMENT '工具保管領域識別用連番',
  `unique_tool_id` bigint(10) unsigned zerofill NOT NULL COMMENT '工具識別用連番',
  `video_path` varchar(50) DEFAULT '0' COMMENT '動画の保存先リンク',
  `tool_num` int(2) DEFAULT 0 COMMENT '操作個数',
  `line_id` int(4) unsigned zerofill NOT NULL COMMENT 'ラインID',
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
) ENGINE=InnoDB AUTO_INCREMENT=50 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- テーブル system1114.rec_operation: ~1 rows (約) のデータをダンプしています
DELETE FROM `rec_operation`;
INSERT INTO `rec_operation` (`operation_id`, `operation_class`, `user_id`, `proc_time`, `storage_area_id`, `unique_tool_id`, `video_path`, `tool_num`, `line_id`) VALUES
	(49, '取出', 0001, '2025-10-31 02:18:21', 0001, 0000000001, 'recording_20251007_155511.mp4', 1, 0000);

--  テーブル system1114.rec_stock の構造をダンプしています
CREATE TABLE IF NOT EXISTS `rec_stock` (
  `stock_id` int(8) NOT NULL AUTO_INCREMENT COMMENT '記録用連番',
  `register_year_month` varchar(50) NOT NULL DEFAULT '' COMMENT '年月',
  `basic_tool_id` int(4) unsigned zerofill NOT NULL COMMENT '工具種別ごとの連番',
  `out_num` int(4) NOT NULL DEFAULT 0 COMMENT '取り出し個数（期間末）',
  `end_period_stock` int(4) NOT NULL DEFAULT 0 COMMENT '期間末在庫数',
  PRIMARY KEY (`stock_id`),
  UNIQUE KEY `stock_id` (`stock_id`),
  UNIQUE KEY `インデックス 4` (`register_year_month`,`basic_tool_id`) USING BTREE,
  KEY `FK_rec_stock_mst_tool` (`basic_tool_id`),
  CONSTRAINT `FK_rec_stock_mst_tool` FOREIGN KEY (`basic_tool_id`) REFERENCES `mst_tool` (`basic_tool_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB AUTO_INCREMENT=6 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- テーブル system1114.rec_stock: ~1 rows (約) のデータをダンプしています
DELETE FROM `rec_stock`;
INSERT INTO `rec_stock` (`stock_id`, `register_year_month`, `basic_tool_id`, `out_num`, `end_period_stock`) VALUES
	(5, '2025-10', 0004, 4, 10);

--  テーブル system1114.storage_area の構造をダンプしています
CREATE TABLE IF NOT EXISTS `storage_area` (
  `storage_area_id` int(4) unsigned zerofill NOT NULL AUTO_INCREMENT COMMENT '保管領域ID',
  `basic_tool_id` int(4) unsigned zerofill NOT NULL COMMENT '基本工具ID',
  `toolcase_stc_num` int(2) DEFAULT NULL COMMENT '保管ケース個数',
  `display_address` varchar(304) DEFAULT NULL COMMENT '画面表示住所',
  PRIMARY KEY (`storage_area_id`),
  UNIQUE KEY `storage_area_id` (`storage_area_id`),
  KEY `FK_storage_area_mst_tool` (`basic_tool_id`),
  KEY `FK_storage_area_mst_address` (`display_address`),
  CONSTRAINT `FK_storage_area_mst_address` FOREIGN KEY (`display_address`) REFERENCES `mst_address` (`display_address`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `FK_storage_area_mst_tool` FOREIGN KEY (`basic_tool_id`) REFERENCES `mst_tool` (`basic_tool_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB AUTO_INCREMENT=3 DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- テーブル system1114.storage_area: ~2 rows (約) のデータをダンプしています
DELETE FROM `storage_area`;
INSERT INTO `storage_area` (`storage_area_id`, `basic_tool_id`, `toolcase_stc_num`, `display_address`) VALUES
	(0001, 0001, 5, 'A01'),
	(0002, 0004, 1, 'A01');

--  テーブル system1114.unique_tool の構造をダンプしています
CREATE TABLE IF NOT EXISTS `unique_tool` (
  `unique_tool_id` bigint(10) unsigned zerofill NOT NULL COMMENT '個別工具ID',
  `tool_print_time` timestamp NOT NULL COMMENT '個別工具QR印刷日時',
  `basic_tool_id` int(4) unsigned zerofill NOT NULL COMMENT '基本工具ID',
  `unique_num` int(6) NOT NULL COMMENT '個別番号',
  `case_pack_num` int(2) NOT NULL COMMENT 'ケース内個数',
  `storage_area_id` int(4) unsigned zerofill DEFAULT NULL COMMENT '保管領域ID',
  `storage_condition` enum('補充前','保管中','取出') NOT NULL DEFAULT '補充前' COMMENT '保管状態',
  `regrind_count` int(2) DEFAULT NULL COMMENT '工具の再研磨回数',
  PRIMARY KEY (`unique_tool_id`),
  UNIQUE KEY `unique_tool` (`unique_tool_id`),
  KEY `FK_unique_tool_mst_tool` (`basic_tool_id`),
  KEY `FK_unique_tool_storage_area` (`storage_area_id`),
  CONSTRAINT `FK_unique_tool_mst_tool` FOREIGN KEY (`basic_tool_id`) REFERENCES `mst_tool` (`basic_tool_id`) ON DELETE NO ACTION ON UPDATE NO ACTION,
  CONSTRAINT `FK_unique_tool_storage_area` FOREIGN KEY (`storage_area_id`) REFERENCES `storage_area` (`storage_area_id`) ON DELETE NO ACTION ON UPDATE NO ACTION
) ENGINE=InnoDB DEFAULT CHARSET=utf8mb4 COLLATE=utf8mb4_general_ci;

-- テーブル system1114.unique_tool: ~2 rows (約) のデータをダンプしています
DELETE FROM `unique_tool`;
INSERT INTO `unique_tool` (`unique_tool_id`, `tool_print_time`, `basic_tool_id`, `unique_num`, `case_pack_num`, `storage_area_id`, `storage_condition`, `regrind_count`) VALUES
	(0000000001, '2025-10-14 23:45:56', 0001, 1, 0, NULL, '取出', 3),
	(0000400001, '2025-10-29 10:30:40', 0004, 1, 10, 0002, '保管中', NULL);

--  トリガー system1114.mst_tool_after_update の構造をダンプしています
SET @OLDTMP_SQL_MODE=@@SQL_MODE, SQL_MODE='STRICT_TRANS_TABLES,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION';
DELIMITER //
CREATE TRIGGER `mst_tool_after_update` AFTER UPDATE ON `mst_tool` FOR EACH ROW BEGIN
		-- 在庫数の増減を条件にして、rec_stockの期間末在庫数を更新していくトリガー
	DECLARE
		v_row_exists INT;
	DECLARE
		v_year_month VARCHAR(50);

	-- 在庫数(stc)カラムが実際に変更された場合のみ実行
	IF NEW.stc <> OLD.stc THEN

		-- 現在の年月を'YYYY-MM'形式で取得
		SET v_year_month = DATE_FORMAT(NOW(), '%Y-%m');

		-- rec_stockテーブルに、該当工具の当月レコードが既に存在するか確認
		SELECT
			COUNT(*) INTO v_row_exists
		FROM
			rec_stock
		WHERE
			basic_tool_id = NEW.basic_tool_id
			AND register_year_month = v_year_month;

		-- レコードが存在する場合
		IF v_row_exists > 0 THEN
			-- 既存レコードの「期間末在庫数」をmst_toolの新しい在庫数で更新
			UPDATE rec_stock
			SET
				end_period_stock = NEW.stc
			WHERE
				basic_tool_id = NEW.basic_tool_id
				AND register_year_month = v_year_month;
		ELSE
			-- レコードが存在しない場合（その月で初めての在庫変動）
			-- 新しいレコードを挿入
			INSERT INTO rec_stock (
				register_year_month,
				basic_tool_id,
				end_period_stock,
				out_num -- このカラムはNotNullなので初期値が必要
			)
		VALUES
			(
				v_year_month,
				NEW.basic_tool_id,
				NEW.stc,
				0 -- 月初の時点では取り出し個数を0として挿入
			);
		END IF;
	END IF;
END//
DELIMITER ;
SET SQL_MODE=@OLDTMP_SQL_MODE;

--  トリガー system1114.mst_tool_after_update(2) の構造をダンプしています
SET @OLDTMP_SQL_MODE=@@SQL_MODE, SQL_MODE='STRICT_TRANS_TABLES,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION';
DELIMITER //
CREATE TRIGGER `mst_tool_after_update(2)` AFTER UPDATE ON `mst_tool` FOR EACH ROW BEGIN
	-- 在庫数の減少を条件にして期間末の取り出し個数を求めるトリガー

	-- 取り出された個数
	DECLARE v_out_num INT;

	-- 取り出した年月
	DECLARE v_year_month VARCHAR(50);

	IF (NEW.stc - OLD.stc) < 0 THEN

		-- 現在の年月を'YYYY-MM'形式で取得
		SET v_year_month = DATE_FORMAT(NOW(), '%Y-%m');

		SET v_out_num = OLD.stc -NEW.stc; -- 取り出した個数を格納する

		UPDATE rec_stock
		SET out_num = out_num + v_out_num
		WHERE basic_tool_id = NEW.basic_tool_id
		AND register_year_month = v_year_month;
	END IF;
END//
DELIMITER ;
SET SQL_MODE=@OLDTMP_SQL_MODE;

--  トリガー system1114.rec_operation_after_insert の構造をダンプしています
SET @OLDTMP_SQL_MODE=@@SQL_MODE, SQL_MODE='STRICT_TRANS_TABLES,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION';
DELIMITER //
CREATE TRIGGER `rec_operation_after_insert` AFTER INSERT ON `rec_operation` FOR EACH ROW BEGIN
	-- ■■■ 変数の宣言 ■■■
	-- 操作対象となる工具の現在の状態を保存するための変数を宣言します。
	DECLARE v_current_storage_condition VARCHAR(255);
	DECLARE v_current_case_pack_num INT;

	-- ■■■ 現在の状態を取得 ■■■
	-- 操作対象の工具（unique_tool）の現在の「保管状態」と「ケース内個数」を取得し、変数に格納します。
	SELECT storage_condition, case_pack_num
	INTO v_current_storage_condition, v_current_case_pack_num
	FROM unique_tool
	WHERE unique_tool_id = NEW.unique_tool_id;

	-- ■■■ 操作クラスによる分岐 ■■■
	-- 記録された操作が「補充」か「取出」かで処理を分けます。
	IF NEW.tool_num <> null THEN
		IF NEW.tool_num >= 0 THEN
			--
			-- ■■■ 補充時の処理 ■■■
			--
			-- 1. 今回の補充が、工具を棚に新たに戻す（または新規に置く）操作かどうかを判断します。
			--    「補充前」または「取出」状態からの補充が対象です。
			IF v_current_storage_condition = '補充前' OR v_current_storage_condition = '取出' THEN
				-- 初めての補充、または再補充の場合（補充前/取出 → 保管中）
				-- a. 保管エリアのケース数を1つ増やします。
				UPDATE storage_area
				SET toolcase_stc_num = toolcase_stc_num + 1
				WHERE storage_area_id = NEW.storage_area_id;

				-- b. 工具の状態を「保管中」に更新し、ケース内の個数を補充数で初期化します。
				UPDATE unique_tool
				SET storage_condition = '保管中',
					case_pack_num = NEW.tool_num,
					storage_area_id = NEW.storage_area_id
				WHERE unique_tool_id = NEW.unique_tool_id;

			-- 2. すでに「保管中」の工具ケースに、追加で補充する場合の処理です。
			ELSEIF v_current_storage_condition = '保管中' THEN
				-- 追加補充の場合（保管中 → 保管中）
				-- a. 保管エリアのケース数は変更しません。
				-- b. ケース内の工具の個数（case_pack_num）だけを補充された数だけ増やします。
				UPDATE unique_tool
				SET case_pack_num = case_pack_num + NEW.tool_num
				WHERE unique_tool_id = NEW.unique_tool_id;
			END IF;

		ELSE THEN
			--
			-- ■■■ 取出時の処理 ■■■
			--
			-- 1. 今回の取出操作によって、ケース内の個数が0個以下になる（ケースが空になる）かどうかを判断します。
			IF (v_current_case_pack_num + NEW.tool_num) <= 0 THEN
				-- ケースが空になる場合
				-- a. 工具の状態を「取出」に更新します。
				UPDATE unique_tool
				SET storage_condition = '取出',
					case_pack_num = 0,
					storage_area_id = null
				WHERE unique_tool_id = NEW.unique_tool_id;

				-- b. 保管エリアのケース数を1つ減らします。
				UPDATE storage_area
				SET toolcase_stc_num = toolcase_stc_num - 1
				WHERE storage_area_id = NEW.storage_area_id;

			ELSE
				-- ケースが空にならない場合
				-- a. 工具の状態は「保管中」のまま変更しません。
				-- b. ケース内の工具の個数（case_pack_num）だけを取り出された数だけ減らします。
				UPDATE unique_tool
				SET case_pack_num = case_pack_num + NEW.tool_num
				WHERE unique_tool_id = NEW.unique_tool_id;
			END IF;
		END IF;
	END IF;
END//
DELIMITER ;
SET SQL_MODE=@OLDTMP_SQL_MODE;

--  トリガー system1114.rec_operation_after_insert(2) の構造をダンプしています
SET @OLDTMP_SQL_MODE=@@SQL_MODE, SQL_MODE='STRICT_TRANS_TABLES,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION';
DELIMITER //
CREATE TRIGGER `rec_operation_after_insert(2)` AFTER INSERT ON `rec_operation` FOR EACH ROW BEGIN
	-- 操作履歴か基本工具の在庫数を求めるトリガー

	-- ■■■ 変数の宣言 ■■■
	-- 操作された工具の基本工具ID（mst_toolを特定するため）を格納する変数を宣言します。
	DECLARE v_basic_tool_id INT;

	-- ■■■ 基本工具IDの取得 ■■■
	-- 新しく追加された操作履歴（NEW）のunique_tool_idを使い、
	-- unique_toolテーブルから対応するbasic_tool_idを取得します。
	SELECT basic_tool_id
	INTO v_basic_tool_id
	FROM unique_tool
	WHERE unique_tool_id = NEW.unique_tool_id;

	-- ■■■ 操作クラスによる在庫数の更新 ■■■
	-- 記録された操作が「補充」か「取出」かで処理を分けます。
	IF NEW.operation_class = '補充' THEN
		--
		-- ■ 補充時の処理
		-- 工具マスタの在庫数（stc）に、補充された個数（NEW.tool_num）を加算します。
		UPDATE mst_tool
		SET stc = stc + NEW.tool_num
		WHERE basic_tool_id = v_basic_tool_id;

	ELSEIF NEW.operation_class = '取出' THEN
		--
		-- ■ 取出時の処理
		-- 工具マスタの在庫数（stc）から、取り出された個数（NEW.tool_num）を減算します。
		UPDATE mst_tool
		SET stc = stc - NEW.tool_num
		WHERE basic_tool_id = v_basic_tool_id;

	END IF;
	-- 「棚卸」など、他の操作クラスの場合は在庫数を変更しません。

END//
DELIMITER ;
SET SQL_MODE=@OLDTMP_SQL_MODE;

--  トリガー system1114.storage_area_before_insert の構造をダンプしています
SET @OLDTMP_SQL_MODE=@@SQL_MODE, SQL_MODE='STRICT_TRANS_TABLES,ERROR_FOR_DIVISION_BY_ZERO,NO_AUTO_CREATE_USER,NO_ENGINE_SUBSTITUTION';
DELIMITER //
CREATE TRIGGER `storage_area_before_insert` BEFORE INSERT ON `storage_area` FOR EACH ROW BEGIN
	-- 挿入前のトリガー
	-- 挿入前にstorage_areaテーブルにある挿入を行う表示住所をカウントしておく
	-- 表示住所が2件以上ある場合は挿入できないエラーを出す。

	DECLARE v_existing_count INT;

	SELECT COUNT(*)
	INTO v_existing_count
	FROM storage_area
	WHERE display_address =NEW.display_address;

	IF v_existing_count >=2 THEN

		SIGNAL SQLSTATE '45000'
				SET MESSAGE_TEXT ='このコンテナには、すでに二種類の工具が登録されているため追加できません';
	END IF;
END//
DELIMITER ;
SET SQL_MODE=@OLDTMP_SQL_MODE;

/*!40103 SET TIME_ZONE=IFNULL(@OLD_TIME_ZONE, 'system1114') */;
/*!40101 SET SQL_MODE=IFNULL(@OLD_SQL_MODE, '') */;
/*!40014 SET FOREIGN_KEY_CHECKS=IFNULL(@OLD_FOREIGN_KEY_CHECKS, 1) */;
/*!40101 SET CHARACTER_SET_CLIENT=@OLD_CHARACTER_SET_CLIENT */;
/*!40111 SET SQL_NOTES=IFNULL(@OLD_SQL_NOTES, 1) */;
