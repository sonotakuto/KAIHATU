-- データベースがなければ作成し、選択する
CREATE DATABASE IF NOT EXISTS `odbc_test_db` CHARACTER SET utf8mb4 COLLATE utf8mb4_general_ci;
USE `odbc_test_db`;

-- もしテーブルが存在すれば、一度削除してクリーンな状態にする
DROP TABLE IF EXISTS `comprehensive_test`;

-- 全ての型をテストするためのテーブルを作成
CREATE TABLE `comprehensive_test` (
    `id` INT PRIMARY KEY AUTO_INCREMENT,
    `description` VARCHAR(255) NOT NULL,          -- string型テスト用
    `integer_val` INT NULL,                       -- int型テスト用
    `double_val` DOUBLE NULL,                     -- double型テスト用
    `timestamp_val` TIMESTAMP NULL,               -- TIMESTAMP/DATETIME型テスト用
    `blob_val` LONGBLOB NULL                      -- BLOB型テスト用
) ENGINE=InnoDB;

-- 念のためテーブルを空にしておく
TRUNCATE TABLE `comprehensive_test`;