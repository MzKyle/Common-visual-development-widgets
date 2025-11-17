#pragma once
#include"head.h"
class KinectFrame {
private:
	IKinectSensor* pSensor;
	HRESULT hResult;
	IDepthFrameSource* pDepthSource;
	IDepthFrameReader* pDepthReader;
	IDepthFrame* pDepthFrame;
	Mat bufferMat;
	Mat depthMat;
	int MaxDitance;//1.8米的映射，改这个值在体积计算是也需要改那边的映射值
	unsigned int bufferSize;
public:
	int width;
	int height;
	KinectFrame(int pheight, int pwidth)
	{
		width = pwidth;// normal 512
		height = pheight;// normal 424
		bufferSize = width * height * sizeof(unsigned short);
		depthMat.create(height, width, CV_16UC1);
		hResult = S_OK;
		pSensor = nullptr;
		pDepthSource = nullptr;
		pDepthReader = nullptr;
		pDepthFrame = nullptr;
	}

	~KinectFrame()
	{
		// 1. 优先释放当前持有的深度帧
		Free_Frame();

		// 2. 释放深度帧读取器（IDepthFrameReader）：依赖 IDepthFrameSource，需先释放
		if (pDepthReader != nullptr)
		{
			pDepthReader->Release();  // 引用计数-1，触发资源回收（若计数为0）
			pDepthReader = nullptr;   // 置空指针，避免野指针访问
		}

		// 3. 释放深度帧源（IDepthFrameSource）：依赖 IKinectSensor，需先释放
		if (pDepthSource != nullptr)
		{
			pDepthSource->Release();
			pDepthSource = nullptr;
		}

		// 4. 释放传感器（IKinectSensor）：最后释放，需先关闭传感器
		if (pSensor != nullptr)
		{
			pSensor->Close();  // 关闭传感器（停止数据采集，释放硬件占用）
			pSensor->Release();// 释放传感器接口
			pSensor = nullptr;
		}

		// 说明：OpenCV 的 Mat（bufferMat、depthMat）无需手动释放
		// Mat 内部维护了引用计数，析构时会自动释放内存（浅拷贝不重复释放，深拷贝独立释放）
	}

	int Open_Sensor();// open Kinect

	int Get_Depth_Frame(Mat& src);// get depth mat

	void Free_Frame();// release
};

int KinectFrame::Open_Sensor()
{
	hResult = GetDefaultKinectSensor(&pSensor);	//获取感应器
	std::cout << hResult << std::endl;
	if (FAILED(hResult))
	{
		std::cerr << "Error:GetDefaultKinectSensor" << std::endl;
		return -1;
	}

	hResult = pSensor->Open();	//打开感应器
	if (FAILED(hResult))
	{
		std::cerr << "Error:IKinectSensor::Open()" << std::endl;
		return -1;
	}

	hResult = pSensor->get_DepthFrameSource(&pDepthSource);//取得深度数据
	if (FAILED(hResult))
	{
		std::cerr << "Error:IKinectSensor::get_DepthFrameSource()" << std::endl;
		return -1;
	}

	hResult = pDepthSource->OpenReader(&pDepthReader);	//打开深度数据的Reader 
	if (FAILED(hResult))
	{
		std::cerr << "Error:IDepthFrameSource::OpenReader()" << std::endl;
		return -1;
	}
	return 1;
}

int KinectFrame::Get_Depth_Frame(Mat& src)
{
	pDepthFrame = nullptr;
	hResult = pDepthReader->AcquireLatestFrame(&pDepthFrame);
	if (SUCCEEDED(hResult))
	{
		hResult = pDepthFrame->AccessUnderlyingBuffer(&bufferSize,
			reinterpret_cast<UINT16**>(&depthMat.data));
		if (SUCCEEDED(hResult))
		{
			//cv::imshow("bufferMat", bufferMat);

			//bufferMat.convertTo(depthMat, CV_8U, -255.0f / MaxDitance, 255.0f);	//16位转换为8
		}
		//blur(depthMat, depthMat, Size(3, 3));
		//else
			//return -1;
		src = depthMat.clone();
		//cv::imshow("depthMat", depthMat);
		return 1;
	}
	else
		return -1;
}

void KinectFrame::Free_Frame()
{
	if (pDepthFrame != NULL)
	{
		pDepthFrame->Release();
		pDepthFrame = NULL;
	}
}
