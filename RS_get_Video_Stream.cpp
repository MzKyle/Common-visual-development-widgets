#pragma once

#include "head.h"
#include <librealsense2/rs.hpp>  // Realsense SDK头文件
#include <opencv2/opencv.hpp>

class RSFrame {
private:
    cv::Mat bufferMat;
    cv::Mat depthMat;       // 深度图（16位原始数据）
    cv::Mat colorMat;       // 彩色图（灰度图）
    cv::Mat depthMat2;
    unsigned int bufferSize;
    rs2::config cfg;        // Realsense配置对象（替换ob::Config）
    rs2::video_stream_profile depthProfile;  // 深度流配置
    rs2::video_stream_profile colorProfile;  // 彩色流配置

public:
    rs2::pipeline pipe;     // Realsense管道（替换ob::Pipeline）
    int width;
    int height;

    // 构造函数：初始化分辨率和图像矩阵
    RSFrame(int pwidth, int pheight) {
        width = pwidth;    // 例如640
        height = pheight;  // 例如480
        bufferSize = width * height * sizeof(unsigned short);
        bufferMat.create(height, width, CV_16SC1);
        depthMat.create(height, width, CV_16UC1);  // Realsense深度数据为16位无符号
        colorMat.create(height, width, CV_8UC1);   // 灰度图
        depthMat2.create(height, width, CV_8UC1);
    }

    ~RSFrame() {}

    // 初始化Realsense相机
    int Open_realsense_sensor();

    // 获取深度帧（转换为OpenCV Mat）
    int Get_Depth_Frame(cv::Mat& src);

    // 获取彩色帧（转换为灰度图）
    int Get_Color_Frame(cv::Mat& src);

    // 释放资源
    void Free_Frame();
};



int RSFrame::Open_realsense_sensor() {
    try {
        // 配置深度流：分辨率、格式（Z16=16位深度值）、帧率30
        cfg.enable_stream(RS2_STREAM_DEPTH, width, height, RS2_FORMAT_Z16, 30);
        // 配置彩色流：分辨率、格式（RGB8）、帧率30（Realsense默认输出RGB，需转换为BGR适配OpenCV）
        cfg.enable_stream(RS2_STREAM_COLOR, width, height, RS2_FORMAT_RGB8, 30);

        rs2::pipeline_profile profile = pipe.start(cfg);

        depthProfile = profile.get_stream(RS2_STREAM_DEPTH).as<rs2::video_stream_profile>();
        std::cout << "实际深度流：" << depthProfile.width() << "×" << depthProfile.height()
            << "，格式=" << depthProfile.format() << std::endl;

        colorProfile = profile.get_stream(RS2_STREAM_COLOR).as<rs2::video_stream_profile>();
        std::cout << "实际彩色流：" << colorProfile.width() << "×" << colorProfile.height()
            << "，格式=" << colorProfile.format() << std::endl;

        // 验证设备信息
        rs2::device dev = profile.get_device();
        std::cout << "Realsense设备名称: " << dev.get_info(RS2_CAMERA_INFO_NAME) << std::endl;
        return 1;
    }
    catch (const rs2::error& e) {
        std::cerr << "Realsense初始化失败: " << e.what() << std::endl;
        return 0;
    }
}

int RSFrame::Get_Depth_Frame(cv::Mat& src) {
    try {
        rs2::frameset frames = pipe.wait_for_frames(100);  // 延长超时，排除启动慢的问题

        // 提取深度帧
        rs2::depth_frame depth_frame = frames.get_depth_frame();
        if (!depth_frame) {
            std::cerr << "获取深度帧失败：未提取到深度流" << std::endl;
            return 0;
        }
        // 转换为OpenCV Mat（16位无符号格式，对应Z16）
        depthMat = cv::Mat(
            cv::Size(width, height),
            CV_16UC1,
            (void*)depth_frame.get_data(),
            cv::Mat::AUTO_STEP
        );

        src = depthMat.clone();  

        return 1;
    }
    catch (const rs2::error& e) {
        // 打印完整异常信息，定位具体原因
        std::cerr << "获取深度帧异常："
            << "类型=" << e.get_type()
            << "，描述=" << e.what()
            << "，函数=" << e.get_failed_function() << std::endl;
        return 0;
    }
}


// 获取彩色帧（转换为灰度图）
int RSFrame::Get_Color_Frame(cv::Mat& src) {
    try {
        // 等待帧数据（超时100ms）
        rs2::frameset frames = pipe.wait_for_frames(300);
        if (!frames) {
            return 0;
        }

        // 提取彩色帧
        rs2::video_frame color_frame = frames.get_color_frame();
        if (!color_frame) {
            return 0;
        }

        // 转换为OpenCV Mat（RGB格式）
        cv::Mat rgb_mat(
            cv::Size(width, height),
            CV_8UC3,
            (void*)color_frame.get_data(),
            cv::Mat::AUTO_STEP
        );

        // 保存图像（按's'键）
        if (cv::waitKey(1) == 115) {
            cv::Mat bgr_mat;
            cv::cvtColor(rgb_mat, bgr_mat, cv::COLOR_RGB2BGR);  // 转为BGR格式（OpenCV默认）
            cv::imwrite("color_image.jpg", bgr_mat);
            return 0;
        }

        // 转换为灰度图
        cv::cvtColor(rgb_mat, colorMat, cv::COLOR_RGB2GRAY);
        src = colorMat.clone();  // 输出灰度图
        return 1;
    }
    catch (const rs2::error& e) {
        std::cerr << "获取彩色帧失败: " << e.what() << std::endl;
        return 0;
    }
}


// 释放相机资源
void RSFrame::Free_Frame() {
    pipe.stop();  // 停止管道
}
