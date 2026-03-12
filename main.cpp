#include <opencv2/opencv.hpp>
#include <pcl/point_cloud.h>
#include <osgViewer/Viewer>
#include <gdal.h>
#include <onnxruntime/onnxruntime_cxx_api.h>
#include <iostream>

int main() {
    std::cout << "OpenCV Version: " << CV_VERSION << std::endl;
    std::cout << "GDAL Version: " << GDALVersionInfo("RELEASE_NAME") << std::endl;
    
    // 测试 ONNX Runtime
    Ort::Env env(ORT_LOGGING_LEVEL_WARNING, "test");
    std::cout << "ONNX Runtime ready!" << std::endl;

    return 0;
}