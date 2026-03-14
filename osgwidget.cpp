#include "osgwidget.h"

#include <osgGA/TrackballManipulator>
#include <osg/Geode>
#include <osg/ShapeDrawable>
#include <osg/Material>
#include <osg/StateSet>

OSGWidget::OSGWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    // 允许接收键盘焦点和鼠标追踪（即使没有按下按键也能捕获鼠标移动）
    setFocusPolicy(Qt::StrongFocus);
    setMouseTracking(true);

    // 1. 初始化 OSG Viewer
    m_viewer = new osgViewer::Viewer;
    
    // 必须设置为单线程，防止 OSG 内部线程与 Qt 的 OpenGL 上下文冲突
    m_viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);

    // 2. 创建嵌入式图形窗口上下文
    m_window = m_viewer->setUpViewerAsEmbeddedInWindow(0, 0, width(), height());

    // 3. 设置相机漫游器（轨迹球，最适合看点云和三维模型）
    m_viewer->setCameraManipulator(new osgGA::TrackballManipulator);

    // 4. 初始化一个默认的 3D 场景
    initDefaultScene();

    // 5. 设置定时器驱动渲染循环 (约 60 帧/秒)
    m_timer = new QTimer(this);
    connect(m_timer, &QTimer::timeout, this, [this]() {
        this->update(); // 触发 Qt 调用 paintGL()
    });
    m_timer->start(16);
}

OSGWidget::~OSGWidget()
{
    if (m_timer) {
        m_timer->stop();
    }
}

osgViewer::Viewer* OSGWidget::getViewer()
{
    return m_viewer.get();
}

void OSGWidget::initDefaultScene()
{
    // 创建一个测试用的几何体
    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    
    // 创建一个球体代表初始坐标原点
    osg::ref_ptr<osg::ShapeDrawable> sphere = new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(0.0f, 0.0f, 0.0f), 1.0f));
    sphere->setColor(osg::Vec4(0.2f, 0.6f, 0.8f, 1.0f)); 
    geode->addDrawable(sphere.get());

    m_viewer->setSceneData(geode.get());
}

void OSGWidget::initializeGL()
{
    // 在这里可以进行一些 OpenGL 全局状态的初始化
}

void OSGWidget::resizeGL(int width, int height)
{
    // 处理高分屏 (High DPI) 的像素缩放比例
    qreal ratio = devicePixelRatioF();
    int scaledWidth = static_cast<int>(width * ratio);
    int scaledHeight = static_cast<int>(height * ratio);

    if (m_window.valid()) {
        m_window->resized(x(), y(), scaledWidth, scaledHeight);
        m_viewer->getCamera()->setViewport(0, 0, scaledWidth, scaledHeight);
        
        // 设置透视投影矩阵 (视场角 30 度)
        m_viewer->getCamera()->setProjectionMatrixAsPerspective(
            30.0, 
            static_cast<double>(scaledWidth) / static_cast<double>(scaledHeight), 
            1.0, 10000.0
        );
    }
}

void OSGWidget::paintGL()
{
    if (m_viewer) {
        // 执行一帧渲染
        m_viewer->frame();
    }
}

// ==========================================
// 事件转发机制 (Qt -> OSG)
// ==========================================

void OSGWidget::mousePressEvent(QMouseEvent* event)
{
    int button = 0;
    if (event->button() == Qt::LeftButton) button = 1;
    else if (event->button() == Qt::MiddleButton) button = 2;
    else if (event->button() == Qt::RightButton) button = 3;

    if (m_window.valid()) {
        m_window->getEventQueue()->mouseButtonPress(event->x() * devicePixelRatioF(), event->y() * devicePixelRatioF(), button);
    }
}

void OSGWidget::mouseReleaseEvent(QMouseEvent* event)
{
    int button = 0;
    if (event->button() == Qt::LeftButton) button = 1;
    else if (event->button() == Qt::MiddleButton) button = 2;
    else if (event->button() == Qt::RightButton) button = 3;

    if (m_window.valid()) {
        m_window->getEventQueue()->mouseButtonRelease(event->x() * devicePixelRatioF(), event->y() * devicePixelRatioF(), button);
    }
}

void OSGWidget::mouseMoveEvent(QMouseEvent* event)
{
    if (m_window.valid()) {
        m_window->getEventQueue()->mouseMotion(event->x() * devicePixelRatioF(), event->y() * devicePixelRatioF());
    }
}

void OSGWidget::wheelEvent(QWheelEvent* event)
{
    if (m_window.valid()) {
        int delta = event->angleDelta().y();
        osgGA::GUIEventAdapter::ScrollingMotion motion = delta > 0 ? 
            osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN;
        m_window->getEventQueue()->mouseScroll(motion);
    }
}

void OSGWidget::keyPressEvent(QKeyEvent* event)
{
    if (m_window.valid()) {
        // 简单处理 ASCII 字符
        m_window->getEventQueue()->keyPress(static_cast<osgGA::GUIEventAdapter::KeySymbol>(event->key()));
    }
}

void OSGWidget::keyReleaseEvent(QKeyEvent* event)
{
    if (m_window.valid()) {
        m_window->getEventQueue()->keyRelease(static_cast<osgGA::GUIEventAdapter::KeySymbol>(event->key()));
    }
}