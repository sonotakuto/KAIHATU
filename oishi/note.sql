BEGIN
	DECLARE v_basic_tool_id INT;
	DECLARE v_year_month VARCHAR(7);
	DECLARE v_count INT;
	DECLARE v_current_stc INT;

	-- 取出以外、または tool_num が負でない場合は処理しない
	IF NEW.operation_class = '取出' AND NEW.tool_num < 0 THEN

		-- unique_tool_id → basic_tool_id を取得
		SELECT basic_tool_id
		INTO v_basic_tool_id
		FROM unique_tool
		WHERE unique_tool_id = NEW.unique_tool_id;

		-- 処理日時の年月（YYYY-MM）
		SET v_year_month = DATE_FORMAT(NEW.proc_time, '%Y-%m');

		-- 追加: 現在の在庫数を取得 (NULLの場合は0とする)
		SELECT COALESCE(stc, 0)
		INTO v_current_stc
		FROM mst_tool
		WHERE basic_tool_id = v_basic_tool_id;

		-- rec_stock の存在確認
		SELECT COUNT(*) INTO v_count
		FROM rec_stock
		WHERE basic_tool_id = v_basic_tool_id
		AND register_year_month = v_year_month;

		IF v_count > 0 THEN
			-- 存在する → out_num を加算
			UPDATE rec_stock
			SET out_num = out_num + ABS(NEW.tool_num)
			WHERE basic_tool_id = v_basic_tool_id
			AND register_year_month = v_year_month;
		ELSE
			-- 存在しない → 新規作成（end_period_stock には現在の在庫数を設定しておく）
			INSERT INTO rec_stock (
				register_year_month,
				basic_tool_id,
				end_period_stock,
				out_num
			) VALUES (
				v_year_month,
				v_basic_tool_id,
				v_current_stc,
				ABS(NEW.tool_num)
			);
		END IF;

	END IF;
END
