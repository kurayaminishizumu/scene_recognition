#include "imagewidget.h"
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

ImageWidget::ImageWidget(QWidget* parent)
    : QWidget(parent)
    , m_scaleFactor(1.0)
    , m_offset(0.0, 0.0)
    , m_isDragging(false)
{
    // 设置深色背景，符合专业软件视觉习惯
    setStyleSheet("background-color: #2b2b2b;");
    
    // 允许鼠标追踪，以便未来绘制悬浮提示或准星
    setMouseTracking(true);
}

void ImageWidget::setImage(const cv::Mat& mat)
{
    if (mat.empty()) return;

    m_cvImage = mat.clone(); // 深拷贝，防止外部内存释放导致崩溃
    m_pixmap = matToPixmap(m_cvImage);
    
    // 新图加载后自动居中并适配大小
    resetView();
}

void ImageWidget::loadImage(const QString& filePath)
{
    // 使用 OpenCV 读取图像 (支持中文路径需注意编码，这里假设路径标准)
    std::string path = filePath.toLocal8Bit().constData();
    cv::Mat mat = cv::imread(path, cv::IMREAD_COLOR);
    if (!mat.empty()) {
        setImage(mat);
    }
}

cv::Mat ImageWidget::getImage() const
{
    return m_cvImage;
}

void ImageWidget::resetView()
{
    if (m_pixmap.isNull()) return;

    // 计算适应窗口的缩放比例
    double scaleX = static_cast<double>(width()) / m_pixmap.width();
    double scaleY = static_cast<double>(height()) / m_pixmap.height();
    m_scaleFactor = std::min(scaleX, scaleY); // 保持宽高比

    // 限制最小初始缩放比例，防止极小图像被无限拉大
    if (m_scaleFactor > 1.0) m_scaleFactor = 1.0; 

    // 计算居中偏移量
    double imgDispWidth = m_pixmap.width() * m_scaleFactor;
    double imgDispHeight = m_pixmap.height() * m_scaleFactor;
    m_offset.setX((width() - imgDispWidth) / 2.0);
    m_offset.setY((height() - imgDispHeight) / 2.0);

    update(); // 触发重绘
}

void ImageWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    if (m_pixmap.isNull()) return;

    QPainter painter(this);
    // 开启平滑变换抗锯齿（对缩放显示极为重要）
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::Antialiasing);

    // 应用视图变换矩阵：先平移，后缩放
    painter.translate(m_offset);
    painter.scale(m_scaleFactor, m_scaleFactor);

    // 在变换后的坐标系中绘制原始大小的 pixmap
    painter.drawPixmap(0, 0, m_pixmap);
    
    /* 
     * 预留区域：阶段四/五中，可以继续在这里叠加绘制
     * 基于 cv::Mat 坐标系的 BBox 矩形框 或 矢量多边形
     * 此时无需手动做坐标系转换，Qt的变换矩阵已自动处理！
     */
}

void ImageWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
    // 当窗口大小改变时，重新适应窗口（如果希望保持用户当前缩放状态，可注释掉此行）
    // resetView(); 
}

void ImageWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor); // 切换为抓取手势
    }
    QWidget::mousePressEvent(event);
}

void ImageWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_isDragging) {
        QPointF delta = event->pos() - m_lastMousePos;
        m_offset += delta;
        m_lastMousePos = event->pos();
        update();
    }
    QWidget::mouseMoveEvent(event);
}

void ImageWidget::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = false;
        unsetCursor(); // 恢复默认鼠标手势
    }
    QWidget::mouseReleaseEvent(event);
}

void ImageWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        resetView(); // 双击鼠标左键恢复全图居中
    }
    QWidget::mouseDoubleClickEvent(event);
}

void ImageWidget::wheelEvent(QWheelEvent* event)
{
    if (m_pixmap.isNull()) return;

    // 获取滚轮滚动的角度差
    int delta = event->angleDelta().y();
    
    // 设置缩放步长参数
    const double zoomInFactor = 1.1;
    const double zoomOutFactor = 1.0 / 1.1;

    // 计算鼠标当前所在的图像上的实际坐标
    QPointF mousePos = event->position();
    QPointF imgPos = (mousePos - m_offset) / m_scaleFactor;

    // 更新缩放比例
    if (delta > 0) {
        m_scaleFactor *= zoomInFactor; // 向上滚，放大
    } else if (delta < 0) {
        m_scaleFactor *= zoomOutFactor; // 向下滚，缩小
    }

    // 限制极限缩放范围 (防止过小或过大导致渲染崩溃)
    m_scaleFactor = std::max(0.01, std::min(m_scaleFactor, 50.0));

    // 根据鼠标焦点重新计算偏移量，实现“以鼠标中心进行缩放”
    m_offset = mousePos - imgPos * m_scaleFactor;

    update();
}

QPixmap ImageWidget::matToPixmap(const cv::Mat& mat)
{
    // OpenCV 默认是 BGR 通道顺序，Qt 默认是 RGB 通道顺序
    cv::Mat tempMat;
    if (mat.channels() == 3) {
        cv::cvtColor(mat, tempMat, cv::COLOR_BGR2RGB);
    } else if (mat.channels() == 4) {
        cv::cvtColor(mat, tempMat, cv::COLOR_BGRA2RGBA);
    } else {
        tempMat = mat.clone(); // 单通道灰度图
    }

    QImage::Format format = QImage::Format_RGB888;
    if (tempMat.channels() == 1) format = QImage::Format_Grayscale8;
    else if (tempMat.channels() == 4) format = QImage::Format_RGBA8888;

    // 根据 cv::Mat 构造 QImage
    QImage image(tempMat.data, tempMat.cols, tempMat.rows, static_cast<int>(tempMat.step), format);
    
    // 必须执行 copy()！因为 QImage 此时直接引用了 tempMat 的内存，
    // 若不 copy，tempMat 析构后 QImage 会触发野指针崩溃。
    return QPixmap::fromImage(image.copy());
}