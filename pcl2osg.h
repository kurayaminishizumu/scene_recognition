#ifndef PCL2OSG_H
#define PCL2OSG_H

#include <pcl/point_cloud.h>
#include <pcl/point_types.h>
#include <osg/Node>
#include <osg/ref_ptr>

namespace Pcl2Osg {

    /**
     * @brief 将 PCL 点云 (XYZRGB) 转换为 OSG 可渲染的节点
     * @param cloud PCL 智能指针格式的点云数据
     * @return 包含点云几何信息的 osg::Node 节点，可直接挂载到 OSG 场景树中
     */
    osg::ref_ptr<osg::Node> convertCloudToOSG(const pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr& cloud);

}

#endif // PCL2OSG_H