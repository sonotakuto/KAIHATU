// src/main/java/jp/ac/kinki_pc/repository/OperationRepository.java

package jp.ac.kinki_pc.repository;

import java.util.List;

import org.springframework.data.jpa.repository.JpaRepository;
import org.springframework.data.jpa.repository.Query;
import org.springframework.data.repository.query.Param;
import org.springframework.stereotype.Repository;

import jp.ac.kinki_pc.entity.Operation;

@Repository
public interface OperationRepository extends JpaRepository<Operation, Integer> {
	
	/**
	 * 氏名、年度月、作業内容を条件に、関連する工具情報を含む操作履歴を検索します。
	 * (DTOへの依存を排除し、Projectionインターフェースを返すように変更)
	 * @param year 年
	 * @param month 月
	 * @param username ユーザー名 (null許容)
	 * @param operationContent 作業内容 (null許容)
	 * @return 条件に一致する操作履歴 (HistoryProjectionのリスト)
	 */
	@Query("SELECT " +
		   "  ro.procTime AS procTime, " +
		   "  mu.userName AS userName, " +
		   "  ro.operationClass AS operationClass, " +
		   "  ro.videoPath AS videoPath, " +
		   "  mt.toolCategory AS toolCategory, " +
		   "  mt.maker AS maker, " +
		   "  mt.toolName AS toolName, " +
		   "  mt.toolMaterial AS toolMaterial, " +
		   "  ro.toolNum AS toolNum, " +
		   "  sa.displayAddress AS displayAddress, " +
	       "  ro.trackResult AS trackResult " +
		   "FROM Operation ro, User mu, StorageArea sa, Tool mt " +
		   "WHERE ro.userId = mu.userId " +
		   "  AND ro.storageAreaId = sa.storageAreaId " +
		   "  AND sa.basicToolId = mt.basicToolId " +
		   "  AND YEAR(ro.procTime) = :year " +
		   "  AND MONTH(ro.procTime) = :month " +
		   "  AND (:username IS NULL OR mu.userName = :username) " +
		   "  AND (:operationContent IS NULL OR ro.operationClass = :operationContent) " +
		   "ORDER BY ro.procTime DESC")
	List<HistoryProjection> findHistoryWithToolDetails(
		@Param("year") Integer year, 
		@Param("month") Integer month, 
		@Param("username") String username, 
		@Param("operationContent") String operationContent
	);
	
	
	/**
	 * 履歴(rec_operation)テーブルに存在する操作履歴の「年月」を重複なく取得します。
	 * (旧 OperationHistoryRepository.findHistoryCamdodate を代替)
	 * * @return 'YYYY-MM' 形式の年月のリスト
	 */
	// メソッド名のスペルミスを修正 (findHistoryCamdodate -> findHistoryCandidate)
	@Query("SELECT DISTINCT FUNCTION('DATE_FORMAT', ro.procTime, '%Y-%m') " +
		   "FROM Operation ro " +
		   "ORDER BY 1 DESC")
	List<String> findHistoryCandidate();

}