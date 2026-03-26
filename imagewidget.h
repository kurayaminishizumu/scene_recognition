#ifndef IMAGEWIDGET_H
#define IMAGEWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QImage>
#include <QRect>
#include <QPointF>
#include <opencv2/core.hpp>

class ImageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ImageWidget(QWidget* parent = nullptr);
    ~ImageWidget() override = default;

    // 1. 核心图像加载接口
    void setImage(const cv::Mat& mat);
    void loadImage(const QString& filePath);
    cv::Mat getImage() const;

    // 2. 提供给 MainWindow 调用：获取当前显示的位图用于发送给后端
    QPixmap currentPixmap() const;

    // 3. 接收 AI 推理结果的接口
    void setDetectionResult(const QList<QRect>& rects, const QImage& mask);

    // 清除当前的检测结果
    void clearDetection();

    // 4. 视角控制
    void resetView();
    signals:
    // 当某个绿框被点击时发出信号，传递该框在列表中的索引和矩形区域
    void boxClicked(int index, QRect rect);

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    // 原始图像数据
    cv::Mat m_cvImage;       
    QPixmap m_pixmap;        

    QList<QRect> m_detectionRects;  // 存储后端返回的像素级坐标
    QImage m_maskImage;      // 存储后端返回的掩膜图
    bool m_hasResult;        // 是否有识别结果需要绘制的标志位

    // 视图变换参数（缩放和平移）
    double m_scaleFactor;    
    QPointF m_offset;        
    
    // 鼠标交互状态
    bool m_isDragging;
    QPointF m_lastMousePos;  
    
    QPixmap matToPixmap(const cv::Mat& mat);
    QRect mapToView(const QRect& pixelRect) const;
};

#endif // IMAGEWIDGET_H