#include "vectorization.h"
#include <opencv2/imgproc.hpp>
#include <vector>

namespace Vectorization {

QList<QPolygonF> vectorizeMask(const cv::Mat& maskMat, double epsilon) {
    QList<QPolygonF> polygons;
    if (maskMat.empty()) return polygons;

    cv::Mat gray;
    if (maskMat.channels() == 4) {
        // SAM 返回的 mask 是 RGBA，提取 Alpha 通道作为掩膜
        std::vector<cv::Mat> channels;
        cv::split(maskMat, channels);
        gray = channels[3]; 
    } else if (maskMat.channels() == 3) {
        cv::cvtColor(maskMat, gray, cv::COLOR_BGR2GRAY);
    } else {
        gray = maskMat.clone();
    }

    // 二值化处理
    cv::Mat binary;
    cv::threshold(gray, binary, 10, 255, cv::THRESH_BINARY);

    // 轮廓提取 (OpenCV 轮廓提取算子)
    std::vector<std::vector<cv::Point>> contours;
    std::vector<cv::Vec4i> hierarchy;
    cv::findContours(binary, contours, hierarchy, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);

    // RDP (Ramer-Douglas-Peucker) 算法智能稀疏化
    for (const auto& contour : contours) {
        std::vector<cv::Point> approxCurve;
        // epsilon 控制稀疏化程度，true 表示闭合多边形
        cv::approxPolyDP(contour, approxCurve, epsilon, true);

        // 转换为 QPolygonF 结构化数据
        QPolygonF polygon;
        for (const auto& pt : approxCurve) {
            polygon.append(QPointF(pt.x, pt.y));
        }
        
        // 过滤掉无效的多边形（至少需要3个顶点）
        if (polygon.size() >= 3) {
            polygons.append(polygon);
        }
    }

    return polygons;
}

QList<QPolygonF> vectorizeMask(const QImage& maskImage, double epsilon) {
    if (maskImage.isNull()) return QList<QPolygonF>();

    // 将 QImage 转换为 cv::Mat
    QImage formattedImage = maskImage.convertToFormat(QImage::Format_RGBA8888);
    cv::Mat mat(formattedImage.height(), formattedImage.width(), CV_8UC4, 
                (void*)formattedImage.constBits(), formattedImage.bytesPerLine());

    return vectorizeMask(mat, epsilon);
}

} // namespace Vectorization
