BEGIN
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
		WHERE basic_tool_id =NEW.basic_tool_id
		AND register_year_month =v_year_month;
	END IF;
END
