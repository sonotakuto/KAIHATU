#pragma once
#include <utility>

namespace layout {

/**
 * @struct Position
 * @brief 2次元座標を表す構造体。
 */
struct Position {
	int x = 0;
	int y = 0;

	// デフォルトコンストラクタ（他のコンストラクタを定義すると自動生成されなくなるため）
	constexpr Position() = default;
	constexpr Position(int _x, int _y) : x(_x), y(_y) {}

	/**
	 * @brief std::pair から Position への変換コンストラクタ。
	 * @param[in] p std::pair<int, int> 型の座標ペア。
	 */
	explicit constexpr Position(const std::pair<int, int>& p) : x(p.first), y(p.second) {}

	/**
	 * @brief Position から std::pair への変換演算子。
	 * @return std::pair<int, int> 型の座標ペア。
	 */
	explicit constexpr operator std::pair<int, int>() const {
		return std::make_pair(x, y);
	}

	// --- 加減算オペレータの追加 ---

	/**
	 * @brief += オペレータ
	 * @param[in] other 加算する Position オブジェクト
	 * @return 自身の参照
	 */
	constexpr Position& operator+=(const Position& other) {
		x += other.x;
		y += other.y;
		return *this;
	}

	/**
	 * @brief -= オペレータ
	 * @param[in] other 減算する Position オブジェクト
	 * @return 自身の参照
	 */
	constexpr Position& operator-=(const Position& other) {
		x -= other.x;
		y -= other.y;
		return *this;
	}

	/**
	 * @brief + オペレータ (friend関数として定義)
	 * @param[in] lhs 左辺の Position オブジェクト
	 * @param[in] rhs 右辺の Position オブジェクト
	 * @return 加算結果の新しい Position オブジェクト
	 */
	friend constexpr Position operator+(Position lhs, const Position& rhs) {
		lhs += rhs; // 上記の += を再利用
		return lhs;
	}

	/**
	 * @brief - オペレータ (friend関数として定義)
	 * @param[in] lhs 左辺の Position オブジェクト
	 * @param[in] rhs 右辺の Position オブジェクト
	 * @return 減算結果の新しい Position オブジェクト
	 */
	friend constexpr Position operator-(Position lhs, const Position& rhs) {
		lhs -= rhs; // 上記の -= を再利用
		return lhs;
	}
};

/**
 * @struct Size
 * @brief 2次元サイズを表す構造体。
 */
struct Size {
	int width = 0;
	int height = 0;

	constexpr Size() = default;
	constexpr Size(int _w, int _h) : width(_w), height(_h) {}

	/**
	 * @brief std::pair から Size への変換コンストラクタ。
	 * @param[in] p std::pair<int, int> 型のサイズペア。
	 */
	explicit constexpr Size(const std::pair<int, int>& p) : width(p.first), height(p.second) {}

	/**
	 * @brief Size から std::pair への変換演算子。
	 * @return std::pair<int, int> 型のサイズペア。
	 */
	explicit constexpr operator std::pair<int, int>() const {
		return std::make_pair(width, height);
	}

	// --- 加減算オペレータの追加 ---

	/**
	 * @brief += オペレータ
	 * @param[in] other 加算する Size オブジェクト
	 * @return 自身の参照
	 */
	constexpr Size& operator+=(const Size& other) {
		width += other.width;
		height += other.height;
		return *this;
	}

	/**
	 * @brief -= オペレータ
	 * @param[in] other 減算する Size オブジェクト
	 * @return 自身の参照
	 */
	constexpr Size& operator-=(const Size& other) {
		width -= other.width;
		height -= other.height;
		return *this;
	}

	/**
	 * @brief + オペレータ
	 * @param[in] lhs 左辺の Size オブジェクト
	 * @param[in] rhs 右辺の Size オブジェクト
	 * @return 加算結果の新しい Size オブジェクト
	 */
	friend constexpr Size operator+(Size lhs, const Size& rhs) {
		lhs += rhs;
		return lhs;
	}

	/**
	 * @brief - オペレータ
	 * @param[in] lhs 左辺の Size オブジェクト
	 * @param[in] rhs 右辺の Size オブジェクト
	 * @return 減算結果の新しい Size オブジェクト
	 */
	friend constexpr Size operator-(Size lhs, const Size& rhs) {
		lhs -= rhs;
		return lhs;
	}
};

/**
 * @enum Gravity
 * @brief ウィンドウ配置の基準位置定義 (layout.h の t_base_position と互換)
 * @details ビットマスクを利用してX軸/Y軸の位置を判定します。
 * X軸(0x03): 0=Left, 1=Center, 2=Right
 * Y軸(0x0C): 0=Top,  4=Center, 8=Bottom
 */
enum class Gravity : int {
	TopLeft		= 0,	NorthWest	= 0,	// 左上
	Top 		= 1,	North		= 1,	// 上中央
	TopRight	= 2,	NorthEast	= 2,	// 右上

	Left		= 4,	West		= 4,	// 左中央
	Center		= 5,	Center		= 5,	// 中央
	Right		= 6,	East		= 6,	// 右中央

	BottomLeft	= 8,	SouthWest	= 8,	// 左下
	Bottom		= 9,	South		= 9,	// 下中央
	BottomRight	= 10,	SouthEast	= 10	// 右下
};

/**
 * @class ScreenArea
 * @brief 画面やモニタの領域情報を保持し、各種基準点を計算するクラス。
 */
struct ScreenArea {
	Position offset = Position{0, 0};	///< 領域の左上の座標 (Global)
	Size size = Size{0, 0};				///< 領域のサイズ (幅, 高さ)

	// デフォルトコンストラクタ
	constexpr ScreenArea() = default;

	/**
	 * @brief コンストラクタ。
	 * @param[in] size 画面やモニタのサイズ (幅, 高さ)。
	 * @param[in] offset 画面やモニタの左上座標 (グローバル座標)。デフォルトは(0,0)。
	 */
	constexpr ScreenArea(const Size& size, const Position& offset = Position{0, 0}) noexcept :
		offset(offset),
		size(size)
	{}

	/**
	 * @brief コンストラクタ。
	 * @param[in] w 画面やモニタの幅 (ピクセル単位)。
	 * @param[in] h 画面やモニタの高さ (ピクセル単位)。
	 * @param[in] offset_x 画面やモニタの左上X座標 (グローバル座標)。デフォルトは0。
	 * @param[in] offset_y 画面やモニタの左上Y座標 (グローバル座標)。デフォルトは0。
	 */
	constexpr ScreenArea(int w, int h, int offset_x = 0, int offset_y = 0) noexcept :
		offset(offset_x, offset_y),
		size(w, h)
	{}

	// --- 座標計算ロジック (内部データを使用) ---

	/**
	 * @brief 左上隅の位置を取得します。
	 * @return スクリーンの左上隅の位置を表す Position 構造体。
	 */
	[[nodiscard]] constexpr Position top_left() const noexcept {
		return offset;
	}

	/**
	 * @brief 上中央の位置を取得します。
	 * @return スクリーンの上中央の位置を表す Position 構造体。
	 */
	[[nodiscard]] constexpr Position top_center() const noexcept {
		return Position(
			offset.x + (size.width - 1) / 2,
			offset.y
		);
	}

	/**
	 * @brief 右上隅の位置を取得します。
	 * @return スクリーンの右上隅の位置を表す Position 構造体。
	 */
	[[nodiscard]] constexpr Position top_right() const noexcept {
		return Position(
			offset.x + size.width - 1,
			offset.y
		);
	}

	/**
	 * @brief 左中央の位置を取得します。
	 * @return スクリーンの左中央の位置を表す Position 構造体。
	 */
	[[nodiscard]] constexpr Position center_left() const noexcept {
		return Position(
			offset.x,
			offset.y + (size.height - 1) / 2
		);
	}

	/**
	 * @brief 中央の位置を取得します。
	 * @return スクリーンの中央の位置を表す Position 構造体。
	 */
	[[nodiscard]] constexpr Position center() const noexcept {
		return Position(
			offset.x + (size.width - 1) / 2,
			offset.y + (size.height - 1) / 2
		);
	}

	/**
	 * @brief 右中央の位置を取得します。
	 * @return スクリーンの右中央の位置を表す Position 構造体。
	 */
	[[nodiscard]] constexpr Position center_right() const noexcept {
		return Position(
			offset.x + size.width - 1,
			offset.y + (size.height - 1) / 2
		);
	}

	/**
	 * @brief 左下隅の位置を取得します。
	 * @return スクリーンの左下隅の位置を表す Position 構造体。
	 */
	[[nodiscard]] constexpr Position bottom_left() const noexcept {
		return Position(
			offset.x,
			offset.y + size.height - 1
		);
	}

	/**
	 * @brief 下中央の位置を取得します。
	 * @return スクリーンの下中央の位置を表す Position 構造体。
	 */
	[[nodiscard]] constexpr Position bottom_center() const noexcept {
		return Position(
			offset.x + (size.width - 1) / 2,
			offset.y + size.height - 1
		);
	}

	/**
	 * @brief 右下隅の位置を取得します。
	 * @return スクリーンの右下隅の位置を表す Position 構造体。
	 */
	[[nodiscard]] constexpr Position bottom_right() const noexcept {
		return Position(
			offset.x + size.width - 1,
			offset.y + size.height - 1
		);
	}
};

/**
 * @brief 任意のGravity座標を左上(TL)座標へ変換するヘルパー関数
 * @param[in] pos 基準位置の座標
 * @param[in] size サイズ情報
 * @param[in] grav 基準位置のGravity
 * @return 左上(TL)座標
 */
[[nodiscard]] constexpr Position calculate_top_left(Position pos, const Size& size, Gravity grav) {
	int val = static_cast<int>(grav);

	// --- X軸の補正 (Mask: 0x03) ---
	switch (val & 0x03) {
	case 1:		// Center (TopCenter, Center, BottomCenter)
		pos.x -= (size.width - 1) / 2;
		break;
	case 2:		// Right (TopRight, CenterRight, BottomRight)
		pos.x -= (size.width - 1);
		break;
	default:	// Left (TopLeft, CenterLeft, BottomLeft)
		// そのまま (補正なし)
		break;
	}

	// --- Y軸の補正 (Mask: 0x0C) ---
	switch (val & 0x0C) {
	case 4:		// Center (CenterLeft, Center, CenterRight)
		pos.y -= (size.height - 1) / 2;
		break;
	case 8:		// Bottom (BottomLeft, BottomCenter, BottomRight)
		pos.y -= (size.height - 1);
		break;
	default:	// Top (TopLeft, TopCenter, TopRight)
		// そのまま (補正なし)
		break;
	}

	return pos;
}

/**
 * @brief 任意のGravity座標を左上(TL)座標へ変換するヘルパー関数
 * @param[in] x 基準位置のX座標
 * @param[in] y 基準位置のY座標
 * @param[in] w サイズの幅
 * @param[in] h サイズの高さ
 * @param[in] grav 基準位置のGravity
 * @return 左上(TL)座標
 */
[[nodiscard]] constexpr Position calculate_top_left(int x, int y, int w, int h, Gravity grav) {
	return calculate_top_left(Position(x, y), Size(w, h), grav);
}
} // namespace layout
