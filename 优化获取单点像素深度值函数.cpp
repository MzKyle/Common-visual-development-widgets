#include<fstream>
#include <fcntl.h>
#include <io.h>
#include <imgproc/types_c.h>
#include <WINSOCK.H>
#include < string >

//获取单个像素的高度 单个点的深度值，误差比较大，一般取某一小区域中值
double get_single_pixel_height(cv::Mat img, int row, int col)
{
	CV_Assert(img.channels() == 1); // 确保是深度单通道图

	uchar pixel_init_value = img.at<uchar>(row, col);
	std::vector<uchar> neighbors;

	// 确定邻域范围
	int start_row = std::max(0, row - 1); //防止行数小于 0
	int end_row = std::min(img.rows - 1, row + 1);  //防止行数超过图像高度
	int start_col = std::max(0, col - 1);
	int end_col = std::min(img.cols - 1, col + 1);

	// 收集邻域像素
	for (int i = start_row; i <= end_row; ++i)
	{
		for (int j = start_col; j <= end_col; ++j)
		{
			neighbors.push_back(img.at<uchar>(i, j));
		}
	}

	std::sort(neighbors.begin(), neighbors.end());

	// 根据邻域大小选择中间区域的像素取平均
	int n = neighbors.size();
	double avg = 0.0;
	int count = 0;

	if (n == 1) { //只有一个像素点（按理说一般没有这种情况，为提高鲁棒性而添加）
		avg = neighbors[0];
	}
	else if (n <= 4) { // 边界 2x2 情况
		avg = (neighbors[1] + neighbors[2]) / 2.0;
	}
	else { // 内部 3x3 情况
		avg = (neighbors[3] + neighbors[4] + neighbors[5]) / 3.0;
	}

	// 判断是否需要替换
	if (n <= 4) {
		if (std::abs((neighbors[1] + neighbors[2]) - pixel_init_value * 2) > 5)
			return (double)pixel_init_value;
	}
	else {
		if (std::abs((neighbors[3] + neighbors[4] + neighbors[5]) - pixel_init_value * 3) > 7)
			return (double)pixel_init_value;
	}

	return avg;
}
