#include "pcl2osg.h"
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/Point>
#include <osg/StateSet>

namespace Pcl2Osg {

osg::ref_ptr<osg::Node> convertCloudToOSG(const pcl::PointCloud<pcl::PointXYZRGB>::ConstPtr& cloud) {
    // 1. 安全性检查
    if (!cloud || cloud->empty()) {
        return nullptr;
    }

    // 2. 创建 OSG 几何体和叶节点
    osg::ref_ptr<osg::Geode> geode = new osg::Geode();
    osg::ref_ptr<osg::Geometry> geom = new osg::Geometry();

    // 3. 预分配 OSG 的顶点数组和颜色数组
    size_t numPoints = cloud->size();
    osg::ref_ptr<osg::Vec3Array> vertices = new osg::Vec3Array(numPoints);
    osg::ref_ptr<osg::Vec4Array> colors = new osg::Vec4Array(numPoints);

    // 4. 遍历 PCL 点云，进行坐标与颜色的映射
    for (size_t i = 0; i < numPoints; ++i) {
        const auto& pt = (*cloud)[i];
        
        // 映射 XYZ 坐标
        (*vertices)[i].set(pt.x, pt.y, pt.z);
        
        // 映射 RGB 颜色 (PCL的RGB是0-255，OSG需要归一化到0.0-1.0)
        (*colors)[i].set(pt.r / 255.0f, pt.g / 255.0f, pt.b / 255.0f, 1.0f);
    }

    // 5. 将数组绑定到几何体
    geom->setVertexArray(vertices.get());
    geom->setColorArray(colors.get(), osg::Array::BIND_PER_VERTEX);
    
    // 6. 设置绘制图元类型为 GL_POINTS
    geom->addPrimitiveSet(new osg::DrawArrays(osg::PrimitiveSet::POINTS, 0, numPoints));

    // 7. 配置渲染状态 (StateSet)
    osg::ref_ptr<osg::StateSet> stateSet = geom->getOrCreateStateSet();
    
    // 7.1 设置点的大小 (例如 2.0 像素，可根据街景密度调整)
    osg::ref_ptr<osg::Point> pointSize = new osg::Point(2.0f);
    stateSet->setAttributeAndModes(pointSize.get(), osg::StateAttribute::ON);
    
    // 7.2 关闭光照计算，直接显示点云原生色彩
    stateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);

    // 8. 将几何体添加到叶节点并返回
    geode->addDrawable(geom.get());
    return geode.get();
}

} // namespace Pcl2Osg