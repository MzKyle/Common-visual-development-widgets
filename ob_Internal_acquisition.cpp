#include "Pipeline.hpp"
#include "Context.hpp"
#include "Device.hpp"
#include <iostream>
#include "ObSensor.hpp"

int main() {
    //// 初始化相机设备
    //ob::Context context;
    //auto deviceList = context.queryDeviceList();

    //auto device = deviceList->getDevice(0);
    ob::Pipeline pipe;
    auto device = pipe.getDevice();
    auto profilesDepth = pipe.getStreamProfileList(OB_SENSOR_DEPTH);
    auto depthProfile = profilesDepth->getVideoStreamProfile(640, 400, OB_FORMAT_Y16, 30);
    
    if (depthProfile == nullptr) {
        std::cerr << "获取深度流配置失败，尝试默认配置..." << std::endl;
    }

    OBCameraIntrinsic depthIntrinsic = depthProfile->getIntrinsic();

    if (depthIntrinsic.width > 0 && depthIntrinsic.height > 0) {
        std::cout << "深度相机内参：" << std::endl;
        std::cout << "分辨率：" << depthIntrinsic.width << "x" << depthIntrinsic.height << std::endl;
        std::cout << "焦距(fx, fy)：" << depthIntrinsic.fx << ", " << depthIntrinsic.fy << std::endl;
        std::cout << "主点(cx, cy)：" << depthIntrinsic.cx << ", " << depthIntrinsic.cy << std::endl;
    }
    else {
        std::cerr << "获取深度相机内参失败！" << std::endl;
        return -1;
    }
    while (true) {};
    return 0;
}
