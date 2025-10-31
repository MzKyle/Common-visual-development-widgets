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



// int RSFrame::Open_realsense_sensor() {
//     try {
//         // 配置深度流：分辨率、格式（Z16=16位深度值）、帧率30
//         cfg.enable_stream(RS2_STREAM_DEPTH, width, height, RS2_FORMAT_Z16, 30);
//         // 配置彩色流：分辨率、格式（RGB8）、帧率30（Realsense默认输出RGB，需转换为BGR适配OpenCV）
//         cfg.enable_stream(RS2_STREAM_COLOR, width, height, RS2_FORMAT_RGB8, 30);

//         rs2::pipeline_profile profile = pipe.start(cfg);

//         depthProfile = profile.get_stream(RS2_STREAM_DEPTH).as<rs2::video_stream_profile>();
//         std::cout << "实际深度流：" << depthProfile.width() << "×" << depthProfile.height()
//             << "，格式=" << depthProfile.format() << std::endl;

//         colorProfile = profile.get_stream(RS2_STREAM_COLOR).as<rs2::video_stream_profile>();
//         std::cout << "实际彩色流：" << colorProfile.width() << "×" << colorProfile.height()
//             << "，格式=" << colorProfile.format() << std::endl;

//         // 验证设备信息
//         rs2::device dev = profile.get_device();
//         std::cout << "Realsense设备名称: " << dev.get_info(RS2_CAMERA_INFO_NAME) << std::endl;
//         return 1;
//     }
//     catch (const rs2::error& e) {
//         std::cerr << "Realsense初始化失败: " << e.what() << std::endl;
//         return 0;
//     }
// }

// int RSFrame::Get_Depth_Frame(cv::Mat& src) {
//     try {
//         rs2::frameset frames = pipe.wait_for_frames(100);  // 延长超时，排除启动慢的问题

//         // 提取深度帧
//         rs2::depth_frame depth_frame = frames.get_depth_frame();
//         if (!depth_frame) {
//             std::cerr << "获取深度帧失败：未提取到深度流" << std::endl;
//             return 0;
//         }
//         // 转换为OpenCV Mat（16位无符号格式，对应Z16）
//         depthMat = cv::Mat(
//             cv::Size(width, height),
//             CV_16UC1,
//             (void*)depth_frame.get_data(),
//             cv::Mat::AUTO_STEP
//         );

//         src = depthMat.clone();  

//         return 1;
//     }
//     catch (const rs2::error& e) {
//         // 打印完整异常信息，定位具体原因
//         std::cerr << "获取深度帧异常："
//             << "类型=" << e.get_type()
//             << "，描述=" << e.what()
//             << "，函数=" << e.get_failed_function() << std::endl;
//         return 0;
//     }
// }

//他妈的这个realsense可变量太多了，而且给出的接口特别底层，直接读取和操作源数据就是一坨答辩

//realsense提供丰富的底层配置接口，需要进行较为全面的配置和滤波后才数据才方便直接使用
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

        auto depth_sensor = dev.first<rs2::depth_sensor>();

        // 设置视觉预设为"高精度模式"（官方推荐的高精度配置）
        if (depth_sensor.supports(RS2_OPTION_VISUAL_PRESET)) {
            depth_sensor.set_option(RS2_OPTION_VISUAL_PRESET, RS2_RS400_VISUAL_PRESET_HIGH_ACCURACY);
        }

        // 提高激光功率（增强低纹理区域的深度检测）
        if (depth_sensor.supports(RS2_OPTION_LASER_POWER)) {
            // 激光功率范围：0-360mW，高精度模式建议设为最大值
            depth_sensor.set_option(RS2_OPTION_LASER_POWER, 360.0f);
        }

        // 禁用自动曝光，设置手动曝光时间（减少帧间波动）
        if (depth_sensor.supports(RS2_OPTION_ENABLE_AUTO_EXPOSURE)) {
            depth_sensor.set_option(RS2_OPTION_ENABLE_AUTO_EXPOSURE, 0.0f);
        }

        // 设置曝光时间（根据环境光线调整，范围：1-33000us）
        if (depth_sensor.supports(RS2_OPTION_EXPOSURE)) {
            depth_sensor.set_option(RS2_OPTION_EXPOSURE, 1500.0f); // 15ms曝光
        }

        //// 获取实际分辨率
        //auto depth_stream = profile.get_stream(RS2_STREAM_DEPTH).as<rs2::video_stream_profile>();
        //width = depth_stream.width();
        //height = depth_stream.height();


        return 1;



    }
    catch (const rs2::error& e) {
        std::cerr << "Realsense初始化失败: " << e.what() << std::endl;
        return 0;
    }
}

//int RSFrame::Get_Depth_Frame(cv::Mat& src) {
//    try {
//        rs2::frameset frames = pipe.wait_for_frames(100);  // 延长超时，排除启动慢的问题
//
//        // 提取深度帧
//        rs2::depth_frame depth_frame = frames.get_depth_frame();
//        if (!depth_frame) {
//            std::cerr << "获取深度帧失败：未提取到深度流" << std::endl;
//            return 0;
//        }
//        rs2::frame filled_frame = hole_filler.process(depth_frame);
//
//
//        // 转换为OpenCV Mat（16位无符号格式，对应Z16）
//        depthMat = cv::Mat(
//            cv::Size(width, height),
//            CV_16UC1,
//            (void*)filled_frame.get_data(),
//            cv::Mat::AUTO_STEP
//        );
//
//        src = depthMat.clone();  
//
//        return 1;
//    }
//    catch (const rs2::error& e) {
//        // 打印完整异常信息，定位具体原因
//        std::cerr << "获取深度帧异常："
//            << "类型=" << e.get_type()
//            << "，描述=" << e.what()
//            << "，函数=" << e.get_failed_function() << std::endl;
//        return 0;
//    }
//}
int RSFrame::Get_Depth_Frame(cv::Mat& src) {
    try {
        // 配置滤波器（保留之前的优化滤波策略）
        static bool filtersInitialized = false;
        static rs2::hole_filling_filter hole_filler;
        static rs2::spatial_filter spat_filter;
        static rs2::temporal_filter temp_filter;
        static rs2::disparity_transform depth2disparity;
        static rs2::disparity_transform disparity2depth(false);
        static rs2::decimation_filter dec_filter;

        if (!filtersInitialized) {
            // 高精度模式下的滤波器参数优化
            dec_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, 1); // 不降采样以保留细节
            hole_filler.set_option(RS2_OPTION_HOLES_FILL, 2);
            spat_filter.set_option(RS2_OPTION_FILTER_MAGNITUDE, 2.0f);
            spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, 0.4f); // 保留更多细节
            spat_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, 10.0f);
            spat_filter.set_option(RS2_OPTION_HOLES_FILL, 2);
            temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_ALPHA, 0.3f); // 减少时间模糊
            temp_filter.set_option(RS2_OPTION_FILTER_SMOOTH_DELTA, 10.0f);
            temp_filter.set_option(RS2_OPTION_HOLES_FILL, 2);

            filtersInitialized = true;
        }

        rs2::frameset frames = pipe.wait_for_frames(100);
        rs2::depth_frame depth_frame = frames.get_depth_frame();

        if (!depth_frame) {
            std::cerr << "获取深度帧失败：未提取到深度流" << std::endl;
            return 0;
        }

        // 应用滤波处理（优化顺序以保留高精度细节）
        rs2::frame processed = depth_frame;
        processed = depth2disparity.process(processed);
        processed = spat_filter.process(processed);
        processed = temp_filter.process(processed);
        processed = disparity2depth.process(processed);
        processed = hole_filler.process(processed);

        rs2::depth_frame filled_depth = processed.as<rs2::depth_frame>();

        // 转换为OpenCV Mat
        depthMat = cv::Mat(
            cv::Size(width, height),
            CV_16UC1,
            (void*)filled_depth.get_data(),
            cv::Mat::AUTO_STEP
        );

        // 额外的后处理：限制深度范围，过滤异常值
        // 可根据实际应用场景调整阈值
        const uint16_t MIN_DEPTH = 300;   // 30厘米
        const uint16_t MAX_DEPTH = 5000;  // 5米
        cv::threshold(depthMat, depthMat, MAX_DEPTH, 0, cv::THRESH_TOZERO_INV);
        cv::threshold(depthMat, depthMat, MIN_DEPTH, 0, cv::THRESH_TOZERO);

        src = depthMat.clone();

        return 1;
    }
    catch (const rs2::error& e) {
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
