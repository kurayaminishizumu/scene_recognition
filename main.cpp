/*
 * 课题：城市街景导航地图要素的零样本语义分割与矢量化方法研究
 * 作者：QZL(模拟生成界面工程)
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
#include <QStackedWidget>
#include <QPixmap>
#include <QPainter>

#include <osgViewer/Viewer>
#include <osg/Node>
#include <osg/Geode>
#include <osg/Geometry>
#include <osg/ShapeDrawable>
#include <osgGA/TrackballManipulator>
#include <osgViewer/GraphicsWindow>

#include "inputoutput.h" 
#include "osgwidget.h" 

class ImageWidget : public QWidget
{
public:
    explicit ImageWidget(QWidget* parent = nullptr) : QWidget(parent) {
        // 设置深色背景，衬托图像
        setStyleSheet("background-color: #2b2b2b;");
    }

    void loadImage(const QString& filePath) {
        m_pixmap = QPixmap(filePath);
        update(); // 触发重绘
    }

protected:
    void paintEvent(QPaintEvent* event) override {
        QWidget::paintEvent(event);
        if (m_pixmap.isNull()) return;

        QPainter painter(this);
        painter.setRenderHint(QPainter::SmoothPixmapTransform);
        
        // 等比例缩放图像以适应当前窗口大小
        QPixmap scaledPixmap = m_pixmap.scaled(size(), Qt::KeepAspectRatio, Qt::SmoothTransformation);
        
        // 计算居中坐标
        int x = (width() - scaledPixmap.width()) / 2;
        int y = (height() - scaledPixmap.height()) / 2;
        
        painter.drawPixmap(x, y, scaledPixmap);
    }

private:
    QPixmap m_pixmap;
};
// ==========================================================
// 主窗口类设计
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
        connectSignals(); 
    }

private:
    void setupActions() {
        // 数据操作
        actImportPCL = new QAction(QString::fromUtf8("导入街景点云(PCL)"), this);
        actImportImg = new QAction(QString::fromUtf8("导入全景图像"), this);
        
        // 核心算法
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
        m_centralStack = new QStackedWidget(this);

        // 实例化两个视口
        m_osgWidget = new OSGWidget(this);
        m_imageWidget = new ImageWidget(this);

        // 添加到堆叠容器 (索引0为OSG，索引1为Image)
        m_centralStack->addWidget(m_osgWidget);
        m_centralStack->addWidget(m_imageWidget);

        // 默认显示 OSG 视口
        m_centralStack->setCurrentWidget(m_osgWidget);

        setCentralWidget(m_centralStack);
    }

    void setupStatusBar() {
        statusBar()->showMessage(QString::fromUtf8("系统就绪 - 等待加载街景数据..."));
    }
    
    void connectSignals() {
        // 1. 导入点云
        connect(actImportPCL, &QAction::triggered, this, [this]() {
            QString path = IOHelper::importPointCloud(this);
            if(!path.isEmpty()) {
                // 切换回 OSG 三维视口
                m_centralStack->setCurrentWidget(m_osgWidget);
                statusBar()->showMessage(QString::fromUtf8("当前加载点云: ") + path);
            }
        });

        // 2. 导入图像
        connect(actImportImg, &QAction::triggered, this, [this]() {
            QString path = IOHelper::importImage(this);
            if(!path.isEmpty()) {
                // 将图像加载到 ImageWidget
                m_imageWidget->loadImage(path);
                // 将中央视口切换为二维图像视口
                m_centralStack->setCurrentWidget(m_imageWidget);
                
                statusBar()->showMessage(QString::fromUtf8("当前加载图像: ") + path);
            }
        });

        // 3. 导出GIS数据 (保持不变)
        connect(actExportGIS, &QAction::triggered, this, [this]() {
            QString path = IOHelper::exportGISData(this);
            if(!path.isEmpty()) {
                statusBar()->showMessage(QString::fromUtf8("数据已导出至: ") + path);
            }
        });
    }
    
    QAction* actImportPCL;
    QAction* actImportImg;
    QAction* actLoadVL;
    QAction* actRunSAM;
    QAction* actVectorize;
    QAction* actExportGIS;

    QStackedWidget* m_centralStack; // 堆叠容器
    OSGWidget* m_osgWidget;         // 三维视口
    ImageWidget* m_imageWidget;     // 二维图像视口
};

// ==========================================================
// Main 函数入口
// ==========================================================
int main(int argc, char *argv[])
{
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}

#include "main.moc"