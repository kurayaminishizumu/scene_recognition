#include "imagewidget.h"
#include <QPainter>
#include <QWheelEvent>
#include <QMouseEvent>
#include <QPen>
#include <opencv2/imgproc.hpp>
#include <opencv2/imgcodecs.hpp>

ImageWidget::ImageWidget(QWidget* parent)
    : QWidget(parent)
    , m_hasResult(false)
    , m_scaleFactor(1.0)
    , m_offset(0.0, 0.0)
    , m_isDragging(false)
{
    setStyleSheet("background-color: #2b2b2b;");
    setMouseTracking(true);
}

QPixmap ImageWidget::currentPixmap() const
{
    return m_pixmap;
}

void ImageWidget::setDetectionResult(const QList<QRect>& rects)
{
    m_detectionRects = rects;
    m_hasResult = true;
    // 初始化全透明图像，用于累积 Mask
    m_accumulatedMask = QImage(m_cvImage.cols, m_cvImage.rows, QImage::Format_ARGB32);
    m_accumulatedMask.fill(Qt::transparent);
    
    update();
}

void ImageWidget::clearDetection()
{
    m_hasResult = false;
    m_detectionRects.clear();
    // 清空累积的 Mask 图层
    if (!m_accumulatedMask.isNull()) {
        m_accumulatedMask.fill(Qt::transparent);
    }
    update();
}

void ImageWidget::setImage(const cv::Mat& mat)
{
    if (mat.empty()) return;
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
void ImageWidget::addMask(const QImage& newMask)
{
    if (m_accumulatedMask.isNull() || newMask.isNull()) return;
    QPainter maskPainter(&m_accumulatedMask);
    // 设置叠加模式：SourceOver 表示将新像素盖在旧像素上（保留透明度）
    maskPainter.setCompositionMode(QPainter::CompositionMode_SourceOver);
    maskPainter.drawImage(0, 0, newMask);
    
    maskPainter.end();
    
    update(); 
}
void ImageWidget::paintEvent(QPaintEvent* event)
{
    QWidget::paintEvent(event);
    if (m_pixmap.isNull()) return;

    QPainter painter(this);
    painter.setRenderHint(QPainter::SmoothPixmapTransform);
    painter.setRenderHint(QPainter::Antialiasing);

    painter.save(); 
    painter.translate(m_offset);
    painter.scale(m_scaleFactor, m_scaleFactor);
    // 1. 绘制底层街景图
    painter.drawPixmap(0, 0, m_pixmap);
    // 2. 绘制 AI 识别结果
    if (m_hasResult) {
        // A. 绘制累积的 Mask 图层 (半透明)
        if (!m_accumulatedMask.isNull()) {
            painter.save();
            painter.setOpacity(0.5);
            painter.drawImage(0, 0, m_accumulatedMask); 
            painter.restore();
        }
        // B. 绘制 BBox 矩形框
        if (!m_detectionRects.isEmpty()) {
            QPen pen(Qt::green, 3);
            pen.setJoinStyle(Qt::MiterJoin);
            painter.setPen(pen);
            painter.setBrush(Qt::NoBrush);

            for (int i = 0; i < m_detectionRects.size(); ++i) {
                QRect rect = m_detectionRects[i];
                painter.drawRect(rect);

                // 绘制标签背景
                painter.setBrush(QColor(0, 255, 0, 150));
                QRect labelRect(rect.x(), rect.y() - 25, 100, 25);
                painter.drawRect(labelRect);
                
                // 绘制文字 (如 Target 1, Target 2...)
                painter.setPen(Qt::black);
                painter.setFont(QFont("Arial", 12, QFont::Bold));
                painter.drawText(labelRect, Qt::AlignCenter, QString("Obj %1").arg(i + 1));
                
                painter.setBrush(Qt::NoBrush);
                painter.setPen(pen);
            }
        }
    }
    
    painter.restore(); 
}

void ImageWidget::resizeEvent(QResizeEvent* event)
{
    QWidget::resizeEvent(event);
}

void ImageWidget::mousePressEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && m_hasResult) {
        // 1. 将鼠标点击的屏幕坐标 转换为 图像像素坐标
        // 转换公式：图像坐标 = (屏幕坐标 - 偏移量) / 缩放比例
        QPointF imgPos = (event->pos() - m_offset) / m_scaleFactor;
        // 2. 遍历所有绿框，检查点击点是否在框内
        for (int i = m_detectionRects.size() - 1; i >= 0; --i) {
            if (m_detectionRects[i].contains(imgPos.toPoint())) {
                // 3. 触发信号，并直接返回（不再执行拖拽逻辑）
                emit boxClicked(i, m_detectionRects[i]);
                return; 
            }
        }
    }

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