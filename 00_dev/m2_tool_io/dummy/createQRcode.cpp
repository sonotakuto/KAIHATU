// apt install libqrencode-dev
// g++ -std=c++17 -o createQRcode createQRcode.cpp `pkg-config --cflags --libs SimpleGtk libqrencode`

#include <SimpleGtk>
#include <qrencode.h>
#include <iostream>

// QRコード生成関数（元の実装を維持）
cv::Mat createQRCode(const std::string &data, int qrSize = 256)
{
	// QRコードを生成
	QRcode *qrcode = QRcode_encodeString(data.c_str(), 0, QR_ECLEVEL_L, QR_MODE_8, 1);
	if (!qrcode) {
		throw new std::runtime_error("Failed to generate QR code.");
	}

	// Mat型の画像に変換
	int size = qrcode->width;
	cv::Mat qrImage(size, size, CV_8UC1);
	for (int y = 0; y < size; y++) {
		for (int x = 0; x < size; x++) {
			qrImage.at<uchar>(y, x) = qrcode->data[y * size + x] & 1 ? 0 : 255;
		}
	}
	QRcode_free(qrcode);

	// 画像サイズの拡大と境界線の追加
	int borderSize = 10;
	cv::resize(qrImage, qrImage, cv::Size(qrSize-borderSize, qrSize-borderSize), 0, 0, cv::INTER_NEAREST);
	cv::copyMakeBorder(qrImage, qrImage, 1, 1, 1, 1, cv::BORDER_CONSTANT, cv::Scalar(255));
	borderSize -= 1;
	// 注意: 黒い背景に埋め込む形になっていますが、QRコードの読み取りには周囲に白い余白(Quiet Zone)が推奨されます。
	// 必要に応じて外側のScalar(0)をScalar(255)に変更してください。
	cv::copyMakeBorder(qrImage, qrImage, borderSize, borderSize, borderSize, borderSize, cv::BORDER_CONSTANT, cv::Scalar(0));

	return qrImage;
}

int main(int argc, char* argv[])
{
	// 引数のチェック: コマンドライン引数が足りない場合は使い方を表示して終了
	if (argc < 2) {
		std::cerr << "Usage: " << argv[0] << " <string_to_encode>" << std::endl;
		return -1;
	}

	// 引数から文字列を取得 (argv[1])
	std::string data = argv[1];
//	std::string window_name = "QR Code";
	std::string window_name = data;

	try {
		// QRコードを生成する
		cv::Mat qrImage = createQRCode(data);

		// 画像を表示する
		// コンソールにもエンコードした内容を表示（確認用）
		std::cout << "Generating QR Code for: " << data << std::endl;

		sg::imshow(window_name.c_str(), qrImage);
		sg::waitKey(0);
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		return -1;
	}

	return 0;
}
