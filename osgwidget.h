#ifndef OSGWIDGET_H
#define OSGWIDGET_H

#include <QOpenGLWidget>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QKeyEvent>
#include <QTimer>

// OSG 核心头文件
#include <osgViewer/Viewer>
#include <osgViewer/GraphicsWindow>
#include <osg/ref_ptr>

class OSGWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    explicit OSGWidget(QWidget* parent = nullptr);
    ~OSGWidget() override;

    // 提供获取 Viewer 的接口，方便后续在主界面中向场景添加点云或矢量节点
    osgViewer::Viewer* getViewer();

protected:
    // QOpenGLWidget 必须重写的三大函数
    void initializeGL() override;
    void resizeGL(int width, int height) override;
    void paintGL() override;

    // Qt 事件重写，用于将交互事件转发给 OSG 漫游器
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;

private:
    // 初始化默认的场景（用作占位符）
    void initDefaultScene();

private:
    osg::ref_ptr<osgViewer::Viewer> m_viewer;
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> m_window;
    QTimer* m_timer;
};

#endif // OSGWIDGET_H