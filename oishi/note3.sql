BEGIN
	-- 在庫数の増減を条件にして、rec_stockの取り出し個数と期間末在庫数を更新していくトリガー

	-- ■■■ 変数の宣言 ■■■
	DECLARE v_year_month VARCHAR(50);	-- 現在の年月
	DECLARE v_row_exists INT;			-- レコード存在確認用
	DECLARE v_out_num INT;				-- 取り出された個数

	-- 在庫数(stc)カラムが変更された場合のみ実行
	IF NOT (NEW.stc <=> OLD.stc) THEN

		-- 現在の年月を'YYYY-MM'形式で取得
		SET v_year_month = DATE_FORMAT(NOW(), '%Y-%m');

		-- rec_stockテーブルに、該当工具の当月レコードが存在するか確認
		SELECT COUNT(*) INTO v_row_exists
		FROM rec_stock
		WHERE basic_tool_id = NEW.basic_tool_id
		AND register_year_month = v_year_month;

		-- 在庫が減少した場合（取出）の取出個数を計算
		-- stcがNULLの場合はOLD.stcもNULLの可能性があり計算できないため、
		-- NULLを0として扱うCOALESCE関数を使用
		IF (COALESCE(NEW.stc, 0) - COALESCE(OLD.stc, 0)) < 0 THEN
			SET v_out_num = COALESCE(OLD.stc, 0) - COALESCE(NEW.stc, 0);
		ELSE
			SET v_out_num = 0; -- 補充または変更なし
		END IF;

		IF v_row_exists > 0 THEN
			-- レコードが存在する場合: 在庫数と取出個数を更新
			UPDATE rec_stock
			SET end_period_stock = COALESCE(NEW.stc, 0),	-- NULLの場合は0を代入
				out_num = out_num + v_out_num
			WHERE basic_tool_id = NEW.basic_tool_id
			AND register_year_month = v_year_month;
		ELSE
			-- レコードが存在しない場合（その月で初めての在庫変動）
			-- 新しいレコードを挿入
			INSERT INTO rec_stock (
				register_year_month,
				basic_tool_id,
				end_period_stock,
				out_num
			) VALUES (
				v_year_month,
				NEW.basic_tool_id,
				COALESCE(NEW.stc, 0),	-- NULLの場合は0を代入
				v_out_num -- 計算した取出個数を挿入
			);
		END IF;
	END IF;
END
