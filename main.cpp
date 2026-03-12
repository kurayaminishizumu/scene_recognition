/*
 * 课题：城市街景导航地图要素的零样本语义分割与矢量化方法研究
 * 作者：邱臻来 (模拟生成界面工程)
 * 描述：Qt + OSG 主界面框架 (包含图层树、属性面板、OSG渲染视口)
 */

#include <QApplication>
#include <QMainWindow>
#include <QMenuBar>
#include <QToolBar>
#include <QDockWidget>
#include <QTreeWidget>
#include <QTableWidget>
#include <QHeaderView>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QAction>
#include <QIcon>
#include <QtOpenGLWidgets/QOpenGLWidget>
#include <QTimer>
#include <QMouseEvent>

// OSG 头文件
#include <osgViewer/Viewer>
#include <osg/Node>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/ShapeDrawable>
#include <osgGA/TrackballManipulator>
#include <osgViewer/GraphicsWindow>

// ==========================================================
// 1. OSG 渲染视口窗口适配器 (结合 QOpenGLWidget 与 OSG)
// ==========================================================
class OSGWidget : public QOpenGLWidget
{
    Q_OBJECT
public:
    OSGWidget(QWidget* parent = nullptr) : QOpenGLWidget(parent)
    {
        // 允许键盘和鼠标事件
        setFocusPolicy(Qt::StrongFocus);
        setMouseTracking(true);

        // 初始化 OSG Viewer
        m_viewer = new osgViewer::Viewer;
        m_viewer->setThreadingModel(osgViewer::Viewer::SingleThreaded);

        // 创建图形上下文 (Graphics Window Embedded)
        m_window = m_viewer->setUpViewerAsEmbeddedInWindow(0, 0, width(), height());
        
        // 设置相机操控器 (轨迹球)
        m_viewer->setCameraManipulator(new osgGA::TrackballManipulator);

        // 创建一个测试用的地球/球体作为初始占位符
        osg::ref_ptr<osg::Geode> geode = new osg::Geode;
        osg::ref_ptr<osg::ShapeDrawable> sphere = new osg::ShapeDrawable(new osg::Sphere(osg::Vec3(0,0,0), 1.0f));
        sphere->setColor(osg::Vec4(0.2f, 0.6f, 0.8f, 1.0f)); // 街景主题色
        geode->addDrawable(sphere.get());
        m_viewer->setSceneData(geode.get());

        // 使用定时器刷新 OSG 渲染
        QTimer* timer = new QTimer(this);
        connect(timer, &QTimer::timeout, this, [this]() {
            this->update();
        });
        timer->start(16); // 约 60 FPS
    }

protected:
    virtual void paintGL() override {
        if (m_viewer) {
            m_viewer->frame();
        }
    }

    virtual void resizeGL(int width, int height) override {
        if (m_window.valid()) {
            m_window->resized(x(), y(), width, height);
            m_viewer->getCamera()->setViewport(0, 0, width, height);
            m_viewer->getCamera()->setProjectionMatrixAsPerspective(30.0f, static_cast<double>(width) / static_cast<double>(height), 1.0, 1000.0);
        }
    }

    // 鼠标事件转发给 OSG
    virtual void mousePressEvent(QMouseEvent* event) override {
        if (m_window.valid()) m_window->getEventQueue()->mouseButtonPress(event->x(), event->y(), event->button());
    }
    virtual void mouseReleaseEvent(QMouseEvent* event) override {
        if (m_window.valid()) m_window->getEventQueue()->mouseButtonRelease(event->x(), event->y(), event->button());
    }
    virtual void mouseMoveEvent(QMouseEvent* event) override {
        if (m_window.valid()) m_window->getEventQueue()->mouseMotion(event->x(), event->y());
    }
    virtual void wheelEvent(QWheelEvent* event) override {
        if (m_window.valid()) m_window->getEventQueue()->mouseScroll(event->angleDelta().y() > 0 ? osgGA::GUIEventAdapter::SCROLL_UP : osgGA::GUIEventAdapter::SCROLL_DOWN);
    }

private:
    osg::ref_ptr<osgViewer::Viewer> m_viewer;
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> m_window;
};


// ==========================================================
// 2. 主窗口类设计
// ==========================================================
class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    MainWindow(QWidget *parent = nullptr) : QMainWindow(parent)
    {
        setWindowTitle(QString::fromUtf8("城市街景要素智能化处理系统 - [零样本语义分割与矢量化]"));
        resize(1280, 800);

        setupActions();
        setupMenus();
        setupToolBars();
        setupDockWidgets();
        setupCentralWidget();
        setupStatusBar();
    }

private:
    void setupActions() {
        // 数据操作
        actImportPCL = new QAction(QString::fromUtf8("导入街景点云(PCL)"), this);
        actImportImg = new QAction(QString::fromUtf8("导入全景图像"), this);
        
        // 核心算法操作 (对应论文主要内容)
        actLoadVL = new QAction(QString::fromUtf8("加载多模态大模型(Qwen VL)"), this);
        actRunSAM = new QAction(QString::fromUtf8("SAM掩码提取(Zero-shot)"), this);
        actVectorize = new QAction(QString::fromUtf8("几何约束矢量化"), this);
        actExportGIS = new QAction(QString::fromUtf8("导出GIS格式(GeoDataset)"), this);
    }

    void setupMenus() {
        QMenuBar* menuBar = this->menuBar();
        
        QMenu* fileMenu = menuBar->addMenu(QString::fromUtf8("文件(&F)"));
        fileMenu->addAction(actImportPCL);
        fileMenu->addAction(actImportImg);
        fileMenu->addSeparator();
        fileMenu->addAction(actExportGIS);

        QMenu* aiMenu = menuBar->addMenu(QString::fromUtf8("大模型认知(&M)"));
        aiMenu->addAction(actLoadVL);
        aiMenu->addAction(actRunSAM);

        QMenu* processMenu = menuBar->addMenu(QString::fromUtf8("结构化处理(&P)"));
        processMenu->addAction(actVectorize);
    }

    void setupToolBars() {
        QToolBar* mainToolBar = addToolBar(QString::fromUtf8("主要工具栏"));
        mainToolBar->addAction(actImportPCL);
        mainToolBar->addAction(actLoadVL);
        mainToolBar->addAction(actRunSAM);
        mainToolBar->addAction(actVectorize);
    }

    void setupDockWidgets() {
        // --- 左侧：图层树控件 ---
        QDockWidget* leftDock = new QDockWidget(QString::fromUtf8("工程图层树"), this);
        leftDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        
        QTreeWidget* layerTree = new QTreeWidget(leftDock);
        layerTree->setHeaderLabel(QString::fromUtf8("场景结构 (OSG SceneGraph)"));
        
        // 模拟图层节点
        QTreeWidgetItem* rootNode = new QTreeWidgetItem(layerTree, QStringList(QString::fromUtf8("Root: 街景场景")));
        new QTreeWidgetItem(rootNode, QStringList(QString::fromUtf8("原始点云 (KD-Tree)")));
        new QTreeWidgetItem(rootNode, QStringList(QString::fromUtf8("大模型 BBox 定位")));
        new QTreeWidgetItem(rootNode, QStringList(QString::fromUtf8("SAM 分割掩码")));
        new QTreeWidgetItem(rootNode, QStringList(QString::fromUtf8("建筑物矢量拓扑")));
        rootNode->setExpanded(true);
        
        leftDock->setWidget(layerTree);
        addDockWidget(Qt::LeftDockWidgetArea, leftDock);

        // --- 右侧：属性面板 ---
        QDockWidget* rightDock = new QDockWidget(QString::fromUtf8("要素属性面板"), this);
        rightDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        
        QTableWidget* propTable = new QTableWidget(5, 2, rightDock);
        propTable->setHorizontalHeaderLabels(QStringList() << QString::fromUtf8("属性名") << QString::fromUtf8("属性值"));
        propTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        propTable->verticalHeader()->setVisible(false);
        
        // 模拟属性数据
        propTable->setItem(0, 0, new QTableWidgetItem(QString::fromUtf8("要素类别")));
        propTable->setItem(0, 1, new QTableWidgetItem(QString::fromUtf8("建筑物立面")));
        propTable->setItem(1, 0, new QTableWidgetItem(QString::fromUtf8("识别模型")));
        propTable->setItem(1, 1, new QTableWidgetItem(QString::fromUtf8("Qwen 2.5 VL")));
        propTable->setItem(2, 0, new QTableWidgetItem(QString::fromUtf8("顶点数量")));
        propTable->setItem(2, 1, new QTableWidgetItem(QString::fromUtf8("1,245")));
        propTable->setItem(3, 0, new QTableWidgetItem(QString::fromUtf8("拓扑状态")));
        propTable->setItem(3, 1, new QTableWidgetItem(QString::fromUtf8("闭合 (Closed)")));
        propTable->setItem(4, 0, new QTableWidgetItem(QString::fromUtf8("置信度")));
        propTable->setItem(4, 1, new QTableWidgetItem(QString::fromUtf8("98.5%")));

        rightDock->setWidget(propTable);
        addDockWidget(Qt::RightDockWidgetArea, rightDock);
    }

    void setupCentralWidget() {
        // 中央设置为 OSG 渲染视口
        OSGWidget* osgWidget = new OSGWidget(this);
        setCentralWidget(osgWidget);
    }

    void setupStatusBar() {
        statusBar()->showMessage(QString::fromUtf8("系统就绪 - 等待加载街景数据..."));
    }

    // 动作声明
    QAction* actImportPCL;
    QAction* actImportImg;
    QAction* actLoadVL;
    QAction* actRunSAM;
    QAction* actVectorize;
    QAction* actExportGIS;
};

// ==========================================================
// 3. Main 函数入口
// ==========================================================
int main(int argc, char *argv[])
{
    // 针对高分屏适配
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}

#include "main.moc" // 如果你在单个文件里编译包含 Q_OBJECT 的类，需要包含 moc 文件