#pragma once
#include <iostream>
#include<vector>
#include<queue>
#include<fstream>

#include<opencv2/opencv.hpp>
#include <opencv2/highgui/highgui.hpp>
#include<opencv2/video/background_segm.hpp>
#include <opencv2/imgproc/imgproc.hpp>
#include "Device.hpp"
#include "Error.hpp"
#include "Pipeline.hpp"
#include "StreamProfile.hpp"
#include "Frame.hpp"

class  OrbbecFrame {
private:
	Mat bufferMat;
	Mat depthMat;
	Mat colorMat;
	Mat depthMat2;
	unsigned int bufferSize;
	std::shared_ptr< ob::Config > config;
	std::shared_ptr<ob::VideoStreamProfile> depthProfile;
	std::shared_ptr<ob::VideoStreamProfile> colorProfile;
	std::shared_ptr<ob::FrameSet> frameSet; //保存一次采集的帧数据（包括深度和颜色帧）
	//Window app;
public:
	ob::Pipeline pipe;
	int width;
	int height;
	OrbbecFrame(int pwidth, int pheight)
	{
		//MaxDitance = 3000;
		width = pwidth;// normal 640   1280
		height = pheight;// normal 400  800
		bufferSize = width * height * sizeof(unsigned short);
		bufferMat.create(height, width, CV_16SC1);//建立图像矩阵
		depthMat.create(height, width, CV_8UC1);
		colorMat.create(height, width, CV_8UC1);
		depthMat2.create(height, width, CV_8UC1);
	}

	~OrbbecFrame()
	{

	}


	int Open_orbbec_sensor();
	int Get_Frame_Set(int time);

	int Get_Depth_Frame(Mat& src);// get depth mat

	int Get_Color_Frame(Mat& src);// get RGB mat

	void Free_Frame();// release
};



int OrbbecFrame::Open_orbbec_sensor() { //打开并配置硬件设备
	auto device = pipe.getDevice(); //获取绑定的设备实例


	device->switchDepthWorkMode("Dimensioning"); //模式设置
	//device->switchDepthWorkMode("Unbinned Dense Default");
	//device->switchDepthWorkMode("Binned Sparse Default");
	//device->switchDepthWorkMode("Unbinned Sparse Default");

	device->setBoolProperty(OB_PROP_DEPTH_SOFT_FILTER_BOOL, false); //关闭软件滤波，拿到原始图像


	auto profilesDepth = pipe.getStreamProfileList(OB_SENSOR_DEPTH);
	auto profilesColor = pipe.getStreamProfileList(OB_SENSOR_COLOR);
	depthProfile = profilesDepth->getVideoStreamProfile(width, height, OB_FORMAT_Y16, 30);
	colorProfile = profilesColor->getVideoStreamProfile(width, height, OB_FORMAT_RGB, 30);


	//通过创建Config来配置Pipeline要启用或者禁用哪些流，这里将启用深度流和彩色流
	config = std::make_shared<ob::Config>();
	config->enableStream(depthProfile);
	config->enableStream(colorProfile);

	//RGB-Depth数据对齐
	config->setAlignMode(ALIGN_D2C_HW_MODE);//对齐模式
	pipe.start(config);
	
	return 1;
}



int OrbbecFrame::Get_Depth_Frame(Mat& src) {

	auto frameSet = pipe.waitForFrames(100);
	if (frameSet == nullptr) {
		std::cout << "Warning: dpt_frame is null" << std::endl;
		return 0;
	}

	auto dpt_frame = frameSet->depthFrame();//深度图
	if (dpt_frame == nullptr) {
		return 0;
	}
	
	// 获取设备和深度传感器
	auto device = pipe.getDevice();
	auto sensor = device->getSensor(OB_SENSOR_DEPTH);
	auto filterList = sensor->getRecommendedFilters();  // 获取推荐滤波器列表

	//Orbecc gemini 2L支持的四种滤波
	std::shared_ptr<ob::HoleFillingFilter> holeFilter;
	std::shared_ptr<ob::EdgeNoiseRemovalFilter> edgeFilter;
	std::shared_ptr<ob::TemporalFilter> TempoFilter;
	std::shared_ptr<ob::SpatialAdvancedFilter> SpatialFilter;


	for (int i = 0; i < filterList->count(); i++) {
		auto postProcessorfilter = filterList->getFilter(i);
		//std::cout << "滤波器类型Depth recommended post processor filter type: " << postProcessorfilter->type() << std::endl;

		if (postProcessorfilter->is<ob::HoleFillingFilter>()) {
			holeFilter = postProcessorfilter->as<ob::HoleFillingFilter>();
			// 可根据需求设置填洞模式（可选），转到查看即可
			//holeFilter->setFilterMode(OB_HOLE_FILL_TOP);
			holeFilter->setFilterMode(OB_HOLE_FILL_FAREST);
		}
		if (postProcessorfilter->is<ob::EdgeNoiseRemovalFilter>()) {
			edgeFilter = postProcessorfilter->as<ob::EdgeNoiseRemovalFilter>();
			//edgeFilter->setFilterParams();
		}
		if (postProcessorfilter->is<ob::TemporalFilter>()) {
			TempoFilter = postProcessorfilter->as<ob::TemporalFilter>();
			TempoFilter->setWeight(0.8);
			//TempoFilter->setDiffScale(0.2);

		}
		if (postProcessorfilter->is<ob::SpatialAdvancedFilter>()) {
			SpatialFilter = postProcessorfilter->as<ob::SpatialAdvancedFilter>();
		}
	}

	// 应用填洞滤波（核心修改：启用填洞处理）
	if (holeFilter != nullptr) {
		holeFilter->process(dpt_frame);  // 对深度帧应用填洞滤波
		edgeFilter->process(dpt_frame);
		TempoFilter->process(dpt_frame);
		//std::cout << "Hole and edge filling filter applied" << std::endl;  // 调试信息：确认滤波生效
	}
	else {
		std::cout << "Warning: HoleFillingFilter not found in recommended filters" << std::endl;
	}

	depthMat = Mat(dpt_frame->height(), dpt_frame->width(), CV_16UC1, (void*)dpt_frame->data());

	src = depthMat.clone();

	return 1;

}


int OrbbecFrame::Get_Color_Frame(Mat& src) {

	auto frameSet = pipe.waitForFrames(100);
	if (frameSet == nullptr) {
		return 0;
	}
	auto color_frame = frameSet->colorFrame();//彩色图
	if (color_frame == nullptr) {
		return 0;
	}
	const cv::Mat raw_mat(color_frame->height(), color_frame->width(), CV_8UC3, color_frame->data());
	if (waitKey(1) == 115) //保存BGR图像
	{
		cv::cvtColor(raw_mat, colorMat, cv::COLOR_RGB2BGR);
		imwrite("day.jpg", colorMat);
		return 0;
	}
	cv::cvtColor(raw_mat, colorMat, cv::COLOR_RGB2GRAY);
	src = colorMat.clone();
	return 1;
	//app.addToRender(color_frame);
	/*if (color_frame->format() == OB_FORMAT_MJPG) {
		const cv::Mat raw_mat(1, color_frame->dataSize(), CV_8UC1, color_frame->data());
		colorMat = cv::imdecode(raw_mat, 1);
	}
	else if (color_frame->format() == OB_FORMAT_NV21) {
		const cv::Mat raw_mat(color_frame->height() * 3 / 2, color_frame->width(), CV_8UC1, color_frame->data());
		cv::cvtColor(raw_mat, colorMat, cv::COLOR_YUV2BGR_NV21);
	}
	else if (color_frame->format() == OB_FORMAT_YUYV || frameSet->colorFrame()->format() == OB_FORMAT_YUY2) {
		const cv::Mat raw_mat(color_frame->height(), color_frame->width(), CV_8UC2, color_frame->data());
		cv::cvtColor(raw_mat, colorMat, cv::COLOR_YUV2BGR_YUY2);
	}
	else if (color_frame->format() == OB_FORMAT_RGB888) {
		const cv::Mat raw_mat(color_frame->height(), color_frame->width(), CV_8UC3, color_frame->data());
		cv::cvtColor(raw_mat, colorMat, cv::COLOR_RGB2BGR);
	}
	else if (color_frame->format() == OB_FORMAT_UYVY) {
		const cv::Mat raw_mat(color_frame->height(), color_frame->width(), CV_8UC2, color_frame->data());
		cv::cvtColor(raw_mat, colorMat, cv::COLOR_YUV2BGR_UYVY);
	}
	src = colorMat.clone();
	///Mat img = frame2mat(frame_set->depthFrame());*/
}

void OrbbecFrame::Free_Frame()
{
	/*if (pDepthFrame != NULL)
	{
		pDepthFrame->Release();
		pDepthFrame = NULL;
	}*/
	pipe.stop();
}
