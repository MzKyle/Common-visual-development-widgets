#include<fstream>
#include <fcntl.h>
#include <io.h>
#include <imgproc/types_c.h>
#include <WINSOCK.H>
#include < string >

double FOV_V = 66;//竖直视场角大小
double FOV_H = 91;//水平
double FOV_D = 111;//对角线

double getWide(double depth)//计算单个像素点的物理宽度
{
	double theta = double(FOV_H / 2.0 / 180.0) * PI;  //相机的角度参数 angle1 = 91
	double gama = double(FOV_V / 2.0 / 180.0) * PI; //angle2 = 66
	int img_view_cols = Frame.width;
	int img_view_rows = Frame.height;
	double height = depth * 1;//货物上一点距摄像头的距离
	double wide = height * tan(theta) * 2 / img_view_cols;
	//double h = height * tan(gama) * 2 / img_view_rows;
	return wide;
}

double getHight(double depth)//计算单个像素点的长度
{
	double theta = double(FOV_H / 2.0 / 180.0) * PI;  //相机的角度参数
	double gama = double(FOV_V / 2.0 / 180.0) * PI;
	int img_view_cols = Frame.width;
	int img_view_rows = Frame.height;
	double height = depth * 1;//货物上一点距摄像头的距离
	//double wide = height * tan(theta) * 2 / img_view_cols;
	double h = height * tan(gama) * 2 / img_view_rows;
	return h;
}
