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
    OBCameraDistortion depthDistortion = depthProfile->getDistortion();


    /*typedef struct {
        float k1;  ///< Radial distortion factor 1
        float k2;  ///< Radial distortion factor 2
        float k3;  ///< Radial distortion factor 3
        float k4;  ///< Radial distortion factor 4
        float k5;  ///< Radial distortion factor 5
        float k6;  ///< Radial distortion factor 6
        float p1;  ///< Tangential distortion factor 1
        float p2;  ///< Tangential distortion factor 2
    } OBCameraDistortion, ob_camera_distortion;*/

    if (depthIntrinsic.width > 0 && depthIntrinsic.height > 0) {
        std::cout << "深度相机内参：" << std::endl;
        std::cout << "分辨率：" << depthIntrinsic.width << "x" << depthIntrinsic.height << std::endl;
        std::cout << "焦距(fx, fy)：" << depthIntrinsic.fx << ", " << depthIntrinsic.fy << std::endl;
        std::cout << "主点(cx, cy)：" << depthIntrinsic.cx << ", " << depthIntrinsic.cy << std::endl;
        std::cout << "径向畸变系数k(1,2,3,4,5,6): " << depthDistortion.k1 << ", " << depthDistortion.k2 << ", " << depthDistortion.k3 << ", " << depthDistortion.k4 << ", " << depthDistortion.k5 << ", " << depthDistortion.k6 << std::endl;
        std::cout << "切向畸变系数p（1，2）：" << depthDistortion.p1 << ", " << depthDistortion.p2 << std::endl;
    }
    else {
        std::cerr << "获取深度相机内参失败！" << std::endl;
        return -1;
    }
    while (true) {};
    return 0;
}
