#include<fstream>
#include <fcntl.h>
#include <io.h>
#include <imgproc/types_c.h>
#include <WINSOCK.H>
#include < string >



int get_white_pixel_num_65535(Mat& ROI)   //16位图中65535是白色
{
	int num = 0;
	for (int i = 0; i < ROI.rows; i++)
		for (int j = 0; j < ROI.cols; j++)
			if (ROI.at<ushort>(i, j) == 65535)
				num++;
	return num;//返回出检测区域中的所有白色像素点数
}


bool static_detect(cv::Mat& prevFrame, cv::Mat& currFrame, double noiseRatioThreshold = 0.991, int changePixelThreshold = 500)
{
	// 安全检查：确保两帧图像尺寸一致、类型为16位无符号整型（避免at<ushort>越界/类型错误）
	CV_Assert(prevFrame.size() == currFrame.size() && "前后帧图像尺寸不一致！");
	CV_Assert(prevFrame.type() == CV_16UC1 && currFrame.type() == CV_16UC1 && "图像类型必须为CV_16UC1（16位无符号单通道）！");

	//统计变化的像素数量
	int changePixelCount = 0;

	//计算物体在当前帧中的区域占比
	double totalPixelCount = static_cast<double>(currFrame.rows * currFrame.cols);
	double objectAreaPercent = static_cast<double>(get_white_pixel_num_65535(currFrame)) / totalPixelCount;

	// 调整系数
	// double adjustmentFactor = -0.033 * objectAreaPercent + 0.996;  
	double adjustmentFactor = 0.9942;  // 当前使用的固定系数

	// 噪声判断：若物体占比过高（超过阈值），判定为无有效物体，检测终止
	if (objectAreaPercent > noiseRatioThreshold)
	{
		cerr << "警告：噪声比例过高，无有效物体，检测无法进行！" << endl;
		return false;
	}

	//遍历所有像素，统计“变化幅度达65535”的像素数量
	for (int row = 0; row < prevFrame.rows; ++row)
	{
		for (int col = 0; col < prevFrame.cols; ++col)
		{
			// 获取前后帧对应位置的16位像素值
			ushort prevPixel = prevFrame.at<ushort>(row, col);
			ushort currPixel = currFrame.at<ushort>(row, col);

			// 若像素差值等于65535，判定为变化像素
			if (abs(currPixel - prevPixel) == 65535)
			{
				++changePixelCount;
			}
		}
	}

	// 根据变化像素数量判定状态：超过阈值→运动（返回false），否则→静止（返回true）
	if (changePixelCount > changePixelThreshold)
	{
		return false;  // 物体运动
	}
	else
	{
		return true;   // 物体静止
	}
}
