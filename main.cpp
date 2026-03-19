/*
 * 课题：城市街景导航地图要素的零样本语义分割与矢量化方法研究
 * 描述：Qt 主界面框架 - [2D图像识别与矢量化核心版]
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
#include <QTimer>
#include <QMouseEvent>
#include <QPixmap>
#include <QPainter>
#include <QMessageBox>
#include <QFileDialog>
#include "inputoutput.h" 
#include "imagewidget.h"

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
        connectSignals(); 
    }

private:
    void setupActions() {
        // 数据操作
        actImportImg = new QAction(QString::fromUtf8("导入街景图像"), this);
        actImportPCL = new QAction(QString::fromUtf8("导入街景点云(PCL) [延后]"), this);
        
        // 核心算法 (针对2D新计划)
        actLoadVL = new QAction(QString::fromUtf8("加载VLM模型(获取BBox)"), this);
        actRunSAM = new QAction(QString::fromUtf8("SAM掩码分割(Mask)"), this);
        actVectorize = new QAction(QString::fromUtf8("OpenCV多边形矢量化"), this);
        actExportGIS = new QAction(QString::fromUtf8("导出GeoJSON"), this);
    }

    void setupMenus() {
        QMenuBar* menuBar = this->menuBar();
        
        QMenu* fileMenu = menuBar->addMenu(QString::fromUtf8("文件(&F)"));
        fileMenu->addAction(actImportImg);
        fileMenu->addAction(actImportPCL);
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
        mainToolBar->addAction(actImportImg);
        mainToolBar->addAction(actLoadVL);
        mainToolBar->addAction(actRunSAM);
        mainToolBar->addAction(actVectorize);
    }

    void setupDockWidgets() {
        // --- 左侧：图层树控件 ---
        QDockWidget* leftDock = new QDockWidget(QString::fromUtf8("图像图层树"), this);
        leftDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        
        QTreeWidget* layerTree = new QTreeWidget(leftDock);
        layerTree->setHeaderLabel(QString::fromUtf8("2D 图像处理流层级"));
        
        // 按照新计划更新图层节点
        QTreeWidgetItem* rootNode = new QTreeWidgetItem(layerTree, QStringList(QString::fromUtf8("Root: 街景图像")));
        new QTreeWidgetItem(rootNode, QStringList(QString::fromUtf8("原始全景图 (RGB)")));
        new QTreeWidgetItem(rootNode, QStringList(QString::fromUtf8("VLM 目标边界框 (BBox)")));
        new QTreeWidgetItem(rootNode, QStringList(QString::fromUtf8("SAM 像素级掩码 (Mask)")));
        new QTreeWidgetItem(rootNode, QStringList(QString::fromUtf8("OpenCV 矢量多边形")));
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
        
        // 模拟 2D 矢量化后的属性数据
        propTable->setItem(0, 0, new QTableWidgetItem(QString::fromUtf8("要素类别")));
        propTable->setItem(0, 1, new QTableWidgetItem(QString::fromUtf8("交通标志 (Traffic Sign)")));
        propTable->setItem(1, 0, new QTableWidgetItem(QString::fromUtf8("识别模型")));
        propTable->setItem(1, 1, new QTableWidgetItem(QString::fromUtf8("llava-phi3 + SAM")));
        propTable->setItem(2, 0, new QTableWidgetItem(QString::fromUtf8("简化顶点数")));
        propTable->setItem(2, 1, new QTableWidgetItem(QString::fromUtf8("8 (approxPolyDP)")));
        propTable->setItem(3, 0, new QTableWidgetItem(QString::fromUtf8("像素面积")));
        propTable->setItem(3, 1, new QTableWidgetItem(QString::fromUtf8("4,520 px²")));
        propTable->setItem(4, 0, new QTableWidgetItem(QString::fromUtf8("识别置信度")));
        propTable->setItem(4, 1, new QTableWidgetItem(QString::fromUtf8("96.2%")));

        rightDock->setWidget(propTable);
        addDockWidget(Qt::RightDockWidgetArea, rightDock);
    }

    void setupCentralWidget() {
        // 新计划中明确中央视口为二维图像显示区
        m_imageWidget = new ImageWidget(this);
        setCentralWidget(m_imageWidget);
    }

    void setupStatusBar() {
        statusBar()->showMessage(QString::fromUtf8("系统就绪 - 等待加载街景图像..."));
    }
    
    void connectSignals() {
        // 1. 导入图像
        connect(actImportImg, &QAction::triggered, this, [this]() {
            // 这里为了演示代码能独立运行，使用了 QFileDialog。
            // 实际您可以换回：QString path = IOHelper::importImage(this);
            QString path = QFileDialog::getOpenFileName(this, QString::fromUtf8("选择街景图像"), "", "Images (*.png *.xpm *.jpg *.jpeg)");
            if(!path.isEmpty()) {
                m_imageWidget->loadImage(path);
                statusBar()->showMessage(QString::fromUtf8("当前加载图像: ") + path);
            }
        });

        // 2. 导入点云 (提示延后)
        connect(actImportPCL, &QAction::triggered, this, [this]() {
            QMessageBox::information(this, QString::fromUtf8("提示"), 
                QString::fromUtf8("三维点云处理功能已延后规划，当前版本专注于 2D 图像的语义分割与 OpenCV 矢量化流程。"));
        });

        // 3. 导出GIS数据
        connect(actExportGIS, &QAction::triggered, this, [this]() {
            // 模拟导出操作
            QString path = QFileDialog::getSaveFileName(this, QString::fromUtf8("导出 GeoJSON"), "output.geojson", "GeoJSON (*.geojson)");
            if(!path.isEmpty()) {
                statusBar()->showMessage(QString::fromUtf8("GeoJSON 数据已准备导出至: ") + path);
            }
        });
    }
    
    QAction* actImportImg;
    QAction* actImportPCL;
    QAction* actLoadVL;
    QAction* actRunSAM;
    QAction* actVectorize;
    QAction* actExportGIS;

    ImageWidget* m_imageWidget;     // 唯一的中央二维图像视口
};

// ==========================================================
// 3. Main 函数入口
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