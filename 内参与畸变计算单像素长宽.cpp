#include<fstream>
#include <fcntl.h>
#include <io.h>
#include <imgproc/types_c.h>
#include <WINSOCK.H>
#include < string >

#pragma comment(lib,"WS2_32.lib")
#define PI 3.1415926
#define MaxDitance 3000
#define count_line 220

// 全局存储相机内参（标定后填充）
double f_x = 306.195;  // 水平焦距（像素单位，需通过标定获得）
double f_y = 306.195;
double c_x = 327.067;  // 主点x坐标（像素，需通过标定获得）
double c_y = 201.222;
// 畸变系数（奥比中光相机出场已经矫正，SDK获取的畸变系数是0）
double k1 = 0, k2 = 0, p1 = 0, p2 = 0, k3 = 0;


// 畸变矫正函数
Point2f undistortPoint(Point2f pt, double depth) {
	double u = pt.x;
	double v = pt.y;

	// 归一化坐标（像素→相机坐标系）
	double x = (u - c_x) / f_x;
	double y = (v - c_y) / f_y;

	// 计算畸变
	double r2 = x * x + y * y;
	double x_distort = x * (1 + k1 * r2 + k2 * r2 * r2 + k3 * r2 * r2 * r2) + 2 * p1 * x * y + p2 * (r2 + 2 * x * x);
	double y_distort = y * (1 + k1 * r2 + k2 * r2 * r2 + k3 * r2 * r2 * r2) + p1 * (r2 + 2 * y * y) + 2 * p2 * x * y;

	// 畸变后像素坐标
	return Point2f(
		x_distort * f_x + c_x,
		y_distort * f_y + c_y
	);
}

double getWide(double depth, int pixel_x) {
	// 畸变矫正
	Point2f undistorted = undistortPoint(Point2f(pixel_x, 0), depth);  // y不影响水平宽度

	// 基于内参计算物理宽度（核心公式）
	double wide = depth / f_x;

	if (depth < 1000) {  // 针对2000mm左右的场景
		double compensation = 1.0 + (1200 - depth) * 0.001;  // 示例：每增加1mm补偿0.01%
		wide *= compensation;
	}

	return wide;
}

double getLength(double depth, int pixel_y) { 
	Point2f undistorted = undistortPoint(Point2f(0, pixel_y), depth);  

	double length = depth / f_y; 

	if (depth < 1000) {
		double compensation = 1.0 + (1200 - depth) * 0.001;  // 垂直方向补偿系数可不同
		length *= compensation;
	}

	return length;
}

