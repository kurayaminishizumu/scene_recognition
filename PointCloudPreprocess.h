#ifndef POINTCLOUD_PREPROCESS_H
#define POINTCLOUD_PREPROCESS_H

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>

namespace PointCloudPreprocess {

    // 定义常用的点云类型别名，保持代码整洁
    using PointT = pcl::PointXYZRGB;
    using PointCloudPtr = pcl::PointCloud<PointT>::Ptr;

    /**
     * @brief 体素网格下采样 (VoxelGrid Downsampling)
     * 对应论文：大规模点云预处理技术，去除冗余点，提升计算与渲染速度
     * 
     * @param inputCloud 输入的原始点云
     * @param leafSize 体素大小 (单位:米)，默认 0.1f 表示 10cm x 10cm x 10cm 的体素块
     * @return PointCloudPtr 降采样后的新点云指针
     */
    PointCloudPtr voxelDownsample(const PointCloudPtr& inputCloud, float leafSize = 0.1f);

}

#endif // POINTCLOUD_PREPROCESS_H