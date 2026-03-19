#include "imagewidget.h"
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPen>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

ImageWidget::ImageWidget(QWidget* parent)
    : QWidget(parent)
    , m_hasResult(false) // 初始化标志位
    , m_scaleFactor(1.0)
    , m_offset(0.0, 0.0)
    , m_isDragging(false)
{
    setStyleSheet("background-color: #2b2b2b;");
    setMouseTracking(true);
}

// --- 新增接口实现 ---

QPixmap ImageWidget::currentPixmap() const
{
    return m_pixmap;
}

void ImageWidget::setDetectionResult(const QRect& rect, const QImage& mask)
{
    m_detectionRect = rect;
    // 将传入的 Mask 转换为带颜色的半透明图（例如青色）
    // 如果后端传回的是 0/255 的灰度图，这里直接存储即可，后续在 paintEvent 处理透明度
    m_maskImage = mask;
    m_hasResult = true;
    update(); // 触发重绘
}

void ImageWidget::clearDetection()
{
    m_hasResult = false;
    m_detectionRect = QRect();
    m_maskImage = QImage();
    update();
}

// --- 原有逻辑修改与增强 ---

void ImageWidget::setImage(const cv::Mat& mat)
{
    if (mat.empty()) return;

    // 加载新图前，先清理上一次的 AI 识别结果
    clearDetection();

    m_cvImage = mat.clone(); 
    m_pixmap = matToPixmap(m_cvImage);
    
    resetView();
}

void ImageWidget::loadImage(const QString& filePath)
{
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

    double scaleX = static_cast<double>(width()) / m_pixmap.width();
    double scaleY = static_cast<double>(height()) / m_pixmap.height();
    m_scaleFactor = std::min(scaleX, scaleY); 

    if (m_scaleFactor > 1.0) m_scaleFactor = 1.0; 

    double imgDispWidth = m_pixmap.width() * m_scaleFactor;
    double imgDispHeight = m_pixmap.height() * m_scaleFactor;
    m_offset.setX((width() - imgDispWidth) / 2.0);
    m_offset.setY((height() - imgDispHeight) / 2.0);

    update(); 
}

void ImageWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    if (m_pixmap.isNull()) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::Antialiasing);

    // --- 开始变换坐标系 ---
    painter.save(); // 保存初始状态
    painter.translate(m_offset);
    painter.scale(m_scaleFactor, m_scaleFactor);

    // 1. 绘制底层街景图
    painter.drawPixmap(0, 0, m_pixmap);

    // 2. 绘制 AI 识别结果
    if (m_hasResult) {
        // A. 绘制 Mask (半透明)
        if (!m_maskImage.isNull()) {
            painter.save();
            painter.setOpacity(0.5); // 设置 50% 透明度
            // 注意：由于我们在变换后的坐标系，这里的 (0,0,w,h) 对应的是图像像素位置
            painter.drawImage(0, 0, m_maskImage); 
            painter.restore();
        }

        // B. 绘制 BBox 矩形框
        if (m_detectionRect.isValid()) {
            QPen pen(Qt::green, 3); // 绿色粗线条
            pen.setJoinStyle(Qt::MiterJoin);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);
            // 直接使用原始像素坐标绘制，Qt 矩阵会自动将其缩放到屏幕位置
            painter.drawRect(m_detectionRect);

            // 绘制标签背景
            painter.setBrush(QColor(0, 255, 0, 150));
            QRect labelRect(m_detectionRect.x(), m_detectionRect.y() - 25, 100, 25);
            painter.drawRect(labelRect);
            
            // 绘制文字
            painter.setPen(Qt::black);
            painter.setFont(QFont("Arial", 12, QFont::Bold));
            painter.drawText(labelRect, Qt::AlignCenter, "Target");
        }
    }
    
    painter.restore(); // 恢复初始状态
}

// --- 以下交互逻辑保持不变 ---

void ImageWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
}

void ImageWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        m_isDragging = true;
        m_lastMousePos = event->pos();
        setCursor(Qt::ClosedHandCursor);
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
        unsetCursor();
    }
    QWidget::mouseReleaseEvent(event);
}

void ImageWidget::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton) {
        resetView(); 
    }
    QWidget::mouseDoubleClickEvent(event);
}

void ImageWidget::wheelEvent(QWheelEvent* event)
{
    if (m_pixmap.isNull()) return;
    int delta = event->angleDelta().y();
    const double zoomInFactor = 1.1;
    const double zoomOutFactor = 1.0 / 1.1;

    QPointF mousePos = event->position();
    QPointF imgPos = (mousePos - m_offset) / m_scaleFactor;

    if (delta > 0) m_scaleFactor *= zoomInFactor;
    else if (delta < 0) m_scaleFactor *= zoomOutFactor;

    m_scaleFactor = std::max(0.01, std::min(m_scaleFactor, 50.0));
    m_offset = mousePos - imgPos * m_scaleFactor;
    update();
}

QPixmap ImageWidget::matToPixmap(const cv::Mat& mat)
{
    cv::Mat tempMat;
    if (mat.channels() == 3) cv::cvtColor(mat, tempMat, cv::COLOR_BGR2RGB);
    else if (mat.channels() == 4) cv::cvtColor(mat, tempMat, cv::COLOR_BGRA2RGBA);
    else tempMat = mat.clone();

    QImage::Format format = QImage::Format_RGB888;
    if (tempMat.channels() == 1) format = QImage::Format_Grayscale8;
    else if (tempMat.channels() == 4) format = QImage::Format_RGBA8888;

    QImage image(tempMat.data, tempMat.cols, tempMat.rows, static_cast<int>(tempMat.step), format);
    return QPixmap::fromImage(image.copy());
}