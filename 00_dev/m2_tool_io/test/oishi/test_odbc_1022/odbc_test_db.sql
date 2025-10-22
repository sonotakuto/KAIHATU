-- データベースがなければ作成し、選択する
CREATE DATABASE IF NOT EXISTS `odbc_test_db` CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci;
USE `odbc_test_db`;

-- もしテーブルが存在すれば、一度削除してクリーンな状態にする
DROP TABLE IF EXISTS `prepared_stmt_test`;

-- プリペアドステートメントの全機能をテストするためのテーブル
CREATE TABLE `prepared_stmt_test` (
	`id` INT PRIMARY KEY AUTO_INCREMENT,
	`int_val` INT NULL,
	`uint_val` INT UNSIGNED NULL,
	`bigint_val` BIGINT NULL,
	`ubigint_val` BIGINT UNSIGNED NULL,
	`double_val` DOUBLE NULL,
	`string_val` VARCHAR(255) NULL,
	`blob_val` LONGBLOB NULL,
	`timestamp_val` TIMESTAMP NULL
) ENGINE=InnoDB;

-- 念のためテーブルを空にしておく
TRUNCATE TABLE `prepared_stmt_test`;