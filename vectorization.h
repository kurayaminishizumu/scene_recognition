#ifndef VECTORIZATION_H
#define VECTORIZATION_H

#include <QImage>
#include <QList>
#include <QPolygonF>
#include <opencv2/core.hpp>

namespace Vectorization {
    /**
     * @brief 基于 OpenCV 轮廓提取与 RDP 算法的矢量化核心模块
     * @param maskImage SAM 返回的掩膜图像 (通常为 RGBA 格式)
     * @param epsilon RDP 算法的距离阈值，控制顶点稀疏化程度 (值越大越稀疏)
     * @return 提取并简化后的多边形列表
     */
    QList<QPolygonF> vectorizeMask(const QImage& maskImage, double epsilon = 2.0);
    
    /**
     * @brief 重载版本，直接接收 cv::Mat 格式的掩膜
     */
    QList<QPolygonF> vectorizeMask(const cv::Mat& maskMat, double epsilon = 2.0);
}

#endif // VECTORIZATION_H
