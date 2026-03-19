#ifndef IMAGEWIDGET_H
#define IMAGEWIDGET_H

#include <QWidget>
#include <QPixmap>
#include <QImage>
#include <QPointF>
#include <opencv2/core.hpp>

class ImageWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ImageWidget(QWidget* parent = nullptr);
    ~ImageWidget() override = default;

    // 核心接口：直接加载 cv::Mat
    void setImage(const cv::Mat& mat);
    
    // 兼容接口：通过文件路径加载（内部自动转为 cv::Mat）
    void loadImage(const QString& filePath);

    // 获取当前显示的 cv::Mat 数据
    cv::Mat getImage() const;

    // 视角复位：自适应居中显示
    void resetView();

protected:
    // 重写绘制事件
    void paintEvent(QPaintEvent* event) override;
    
    // 重写窗口大小改变事件（用于初次加载或调整窗口时自动居中）
    void resizeEvent(QResizeEvent* event) override;

    // 重写鼠标交互事件（拖拽平移）
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    
    // 重写双击事件（双击恢复原始视图）
    void mouseDoubleClickEvent(QMouseEvent* event) override;

    // 重写滚轮事件（缩放）
    void wheelEvent(QWheelEvent* event) override;

private:
    cv::Mat m_cvImage;       // 缓存的原始 OpenCV 图像数据
    QPixmap m_pixmap;        // 用于 Qt 高效绘制的位图

    // 视图变换参数
    double m_scaleFactor;    // 当前缩放比例
    QPointF m_offset;        // 当前图像相对于视口左上角的偏移量
    
    // 拖拽状态记录
    bool m_isDragging;
    QPointF m_lastMousePos;  // 上一次鼠标的位置坐标
    
    // 内部工具：将 cv::Mat 转换为 QPixmap
    QPixmap matToPixmap(const cv::Mat& mat);
};

#endif // IMAGEWIDGET_H