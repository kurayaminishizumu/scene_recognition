#include "PointCloudPreprocess.h"
#include <pcl/filters/voxel_grid.h>
#include <iostream>

namespace PointCloudPreprocess {

PointCloudPtr voxelDownsample(const PointCloudPtr& inputCloud, float leafSize) {
    // 1. 安全性检查
    if (!inputCloud || inputCloud->empty()) {
        std::cerr << "[Warning] Input cloud is empty or null! Cannot perform downsampling." << std::endl;
        return nullptr;
    }

    // 2. 创建用于存储输出结果的点云指针
    PointCloudPtr outputCloud(new pcl::PointCloud<PointT>());

    // 3. 实例化体素网格滤波器
    pcl::VoxelGrid<PointT> voxelFilter;
    
    // 4. 设置输入点云
    voxelFilter.setInputCloud(inputCloud);
    
    // 5. 设置体素的叶子大小 (X, Y, Z 方向的分辨率)
    voxelFilter.setLeafSize(leafSize, leafSize, leafSize);
    
    // 6. 执行滤波并保存到 outputCloud
    voxelFilter.filter(*outputCloud);

    // 7. 在控制台打印对比信息，方便调试与观察压缩率
    std::cout << "[Preprocess] VoxelGrid Downsampling completed.\n"
              << "  - Original points: " << inputCloud->points.size() << "\n"
              << "  - Downsampled points: " << outputCloud->points.size() << "\n"
              << "  - Leaf size: " << leafSize << "m" << std::endl;

    return outputCloud;
}

} // namespace PointCloudPreprocess