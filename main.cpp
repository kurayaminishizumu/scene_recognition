#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QNetworkRequest>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QBuffer>
#include <QInputDialog>
#include <QApplication>
#include <QMainWindow>
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
        
        // 初始化网络管理器
        m_networkManager = new QNetworkAccessManager(this); 

        setupActions();
        setupToolBars();
        setupDockWidgets();
        setupCentralWidget();
        setupStatusBar();
        connectSignals(); 
    }

private:
    void updatePropertyTable(const QString& label, double confidence = -1.0) {
        if (!m_propTable) return;

        // 更新要素类别 (第0行, 第1列)
        if (!label.isEmpty()) {
            m_propTable->item(0, 1)->setText(label);
        }

        // 更新置信度为百分比 (第4行, 第1列)
        if (confidence >= 0) {
            QString confStr = QString::number(confidence * 100.0, 'f', 1) + "%";
            m_propTable->item(4, 1)->setText(confStr);
        } else {
            m_propTable->item(4, 1)->setText(QString::fromUtf8("计算中..."));
        }
    }

    // --- VLM 零样本多目标识别请求 ---
    void requestAIInference(const QString &prompt) {
        QPixmap currentPix = m_imageWidget->currentPixmap();
        if (currentPix.isNull()) {
            QMessageBox::warning(this, QString::fromUtf8("错误"), QString::fromUtf8("请先导入街景图像！"));
            return;
        }

        updatePropertyTable(prompt, -1.0); 
        statusBar()->showMessage(QString::fromUtf8("AI 正在全景识别中，请稍候..."));

        QImage img = currentPix.toImage();
        QByteArray ba;
        QBuffer buffer(&ba);
        buffer.open(QIODevice::WriteOnly);
        img.save(&buffer, "PNG");
        QString base64Image = QString::fromLatin1(ba.toBase64().data());

        QJsonObject jsonObj;
        jsonObj["image_base64"] = base64Image;
        jsonObj["prompt"] = prompt;
        
        QNetworkRequest request(QUrl("http://127.0.0.1:8000/api/extract_element"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(jsonObj).toJson());

        connect(reply, &QNetworkReply::finished, this, [this, reply, prompt]() {
            if (reply->error() == QNetworkReply::NoError) {
                QJsonObject response = QJsonDocument::fromJson(reply->readAll()).object();
                if (response["status"].toString() == "success") {
                    
                    // 解析多目标坐标 bboxes: [[x1,y1,x2,y2], [x1,y1,x2,y2]...]
                    QJsonArray bboxesArr = response["bboxes"].toArray();
                    m_currentBBoxArray = bboxesArr; // 保存完整的 JSON 数组供 SAM 备用
                    m_currentRects.clear();         // 清空上次的矩形列表
                    
                    for (int i = 0; i < bboxesArr.size(); ++i) {
                        QJsonArray singleBox = bboxesArr[i].toArray();
                        if (singleBox.size() == 4) {
                            QRect rect(singleBox[0].toInt(), singleBox[1].toInt(), 
                                       singleBox[2].toInt() - singleBox[0].toInt(), 
                                       singleBox[3].toInt() - singleBox[1].toInt());
                            m_currentRects.append(rect);
                        }
                    }

                    m_imageWidget->setDetectionResult(m_currentRects);
                    
                    // 更新识别置信度为百分比
                    double conf = response["confidence"].toDouble();
                    updatePropertyTable(prompt, conf);

                    statusBar()->showMessage(QString::fromUtf8("识别成功！共发现 ") + QString::number(m_currentRects.size()) + QString::fromUtf8(" 个目标"));
                } else {
                    statusBar()->showMessage(QString::fromUtf8("未找到目标: ") + response["message"].toString());
                    m_imageWidget->clearDetection();
                }
            } else {
                QMessageBox::critical(this, QString::fromUtf8("错误"), QString::fromUtf8("后端服务未响应"));
            }
            reply->deleteLater();
        });
    }

    // --- MobileSAM 高精度掩码分割请求 ---
  // --- 修改后：针对特定索引的 SAM 分割请求 ---
    void requestSAMSegmentation(int index) {
        // 这里的 index 是通过点击传进来的
        if (m_currentBBoxArray.isEmpty() || index >= m_currentBBoxArray.size()) return;

        QPixmap currentPix = m_imageWidget->currentPixmap();
        if (currentPix.isNull()) return;

        statusBar()->showMessage(QString(QString::fromUtf8("正在提取目标 Obj %1 的掩码...")).arg(index + 1));

        // 构造 Base64 (逻辑同前)
        QImage img = currentPix.toImage();
        QByteArray ba; QBuffer buffer(&ba); buffer.open(QIODevice::WriteOnly);
        img.save(&buffer, "PNG");
        QString base64Image = QString::fromLatin1(ba.toBase64().data());

        // 构造 JSON：只发送被点中的那个 BBox 坐标
        QJsonObject jsonObj;
        jsonObj["image_base64"] = base64Image;
        jsonObj["bbox"] = m_currentBBoxArray[index].toArray(); 

        QNetworkRequest request(QUrl("http://127.0.0.1:8000/api/run_sam"));
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QNetworkReply* reply = m_networkManager->post(request, QJsonDocument(jsonObj).toJson());

        connect(reply, &QNetworkReply::finished, this, [this, reply, index]() {
            if (reply->error() == QNetworkReply::NoError) {
                QJsonObject response = QJsonDocument::fromJson(reply->readAll()).object();
                if (response["status"].toString() == "success") {
                    QImage maskImg;
                    maskImg.loadFromData(QByteArray::fromBase64(response["mask_base64"].toString().toLatin1()), "PNG");

                    // 显示结果：只更新当前被点中目标的 Mask，保持绿框不变
                    m_imageWidget->addMask(maskImg);
                    
                    // 更新属性面板
                    m_propTable->item(0, 1)->setText(QString(QString::fromUtf8("已识别: Obj %1")).arg(index + 1));
                    statusBar()->showMessage(QString(QString::fromUtf8("目标 Obj %1 分割完成")).arg(index + 1));
                }
            }
            reply->deleteLater();
        });
    }

    void setupActions() {
        actImportImg = new QAction(QString::fromUtf8("导入街景图像"), this);
        actImportPCL = new QAction(QString::fromUtf8("导入街景点云(PCL) [延后]"), this);
        actLoadVL = new QAction(QString::fromUtf8("加载VLM模型(获取BBox)"), this);
        actRunSAM = new QAction(QString::fromUtf8("SAM掩码分割(Mask)"), this);
        actVectorize = new QAction(QString::fromUtf8("OpenCV多边形矢量化"), this);
        actExportGIS = new QAction(QString::fromUtf8("导出GeoJSON"), this);
    }

    void setupToolBars() {
        QToolBar* mainToolBar = addToolBar(QString::fromUtf8("主要工具栏"));
        mainToolBar->addAction(actImportImg);
        mainToolBar->addAction(actLoadVL);
        mainToolBar->addAction(actRunSAM);
        mainToolBar->addAction(actVectorize);
    }

    void setupDockWidgets() {
        // --- 左侧：图层树 ---
        QDockWidget* leftDock = new QDockWidget(QString::fromUtf8("图像图层树"), this);
        leftDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
        
        QTreeWidget* layerTree = new QTreeWidget(leftDock);
        layerTree->setHeaderLabel(QString::fromUtf8("2D 图像处理流层级"));
        
        QTreeWidgetItem* rootNode = new QTreeWidgetItem(layerTree, QStringList(QString::fromUtf8("Root: 街景图像")));
        new QTreeWidgetItem(rootNode, QStringList(QString::fromUtf8("原始全景图 (RGB)")));
        new QTreeWidgetItem(rootNode, QStringList(QString::fromUtf8("VLM 目标边界框 (BBox)")));
        new QTreeWidgetItem(rootNode, QStringList(QString::fromUtf8("SAM 像素级掩码 (Mask)")));
        new QTreeWidgetItem(rootNode, QStringList(QString::fromUtf8("OpenCV 矢量多边形")));
        rootNode->setExpanded(true);
        
        leftDock->setWidget(layerTree);
        addDockWidget(Qt::LeftDockWidgetArea, leftDock);

        // --- 右侧：要素属性面板 ---
        QDockWidget* rightDock = new QDockWidget(QString::fromUtf8("要素属性面板"), this);
        m_propTable = new QTableWidget(5, 2, rightDock); 
        m_propTable->setHorizontalHeaderLabels(QStringList() << QString::fromUtf8("属性名") << QString::fromUtf8("属性值"));
        m_propTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_propTable->verticalHeader()->setVisible(false);
        
        QStringList propNames = { 
            QString::fromUtf8("要素类别"), 
            QString::fromUtf8("识别模型"), 
            QString::fromUtf8("简化顶点数"), 
            QString::fromUtf8("像素面积"), 
            QString::fromUtf8("识别置信度") 
        };
        QStringList propValues = { 
            QString::fromUtf8("待识别"), 
            QString::fromUtf8("Owl-ViT + SAM"), 
            QString::fromUtf8("待计算"), 
            QString::fromUtf8("待计算"), 
            QString::fromUtf8("0.0%") 
        };

        for(int i=0; i<5; ++i) {
            m_propTable->setItem(i, 0, new QTableWidgetItem(propNames[i]));
            m_propTable->setItem(i, 1, new QTableWidgetItem(propValues[i]));
            m_propTable->item(i, 0)->setFlags(m_propTable->item(i, 0)->flags() & ~Qt::ItemIsEditable);
        }

        rightDock->setWidget(m_propTable);
        addDockWidget(Qt::RightDockWidgetArea, rightDock);
    }

    void setupCentralWidget() {
        m_imageWidget = new ImageWidget(this);
        setCentralWidget(m_imageWidget);
    }

    void setupStatusBar() {
        statusBar()->showMessage(QString::fromUtf8("系统就绪 - 等待加载街景图像..."));
    }
    
    void connectSignals() {
        // 导入图像
        connect(actImportImg, &QAction::triggered, this, [this]() {
            QString path = QFileDialog::getOpenFileName(this, QString::fromUtf8("选择街景图像"), "", "Images (*.png *.jpg *.jpeg)");
            if(!path.isEmpty()) {
                m_imageWidget->loadImage(path);
                updatePropertyTable(QString::fromUtf8("待识别"), 0.0);
                m_currentBBoxArray = QJsonArray(); // 清空历史识别数据
                m_currentRects.clear();
            }
        });

        // 识别动作 (VLM)
        connect(actLoadVL, &QAction::triggered, this, [this]() {
            bool ok;
            QString text = QInputDialog::getText(this, QString::fromUtf8("大模型认知提示词"),
                                                 QString::fromUtf8("请输入要识别的要素名称:"), QLineEdit::Normal,
                                                 QString::fromUtf8("traffic sign"), &ok);
            if (ok && !text.isEmpty()) {
                requestAIInference(text);
            }
        });

        // [核心新增] 连接 ImageWidget 的点击信号到 SAM 请求
        connect(m_imageWidget, &ImageWidget::boxClicked, this, [this](int index, QRect rect) {
            // 当用户点击绿框时，自动触发该目标的 SAM 分割
            requestSAMSegmentation(index);
        });
        
        // 修改原有的 actRunSAM 动作：默认分割第一个，或者提示用户点击
        connect(actRunSAM, &QAction::triggered, this, [this]() {
             QMessageBox::information(this, QString::fromUtf8("操作提示"), 
                                      QString::fromUtf8("请直接在图像中点击您想分割的绿色目标框！"));
        });
    }

    // --- 成员变量定义 ---
    QTableWidget* m_propTable;
    QNetworkAccessManager* m_networkManager;
    ImageWidget* m_imageWidget;
    
    // 用于存储 VLM 返回的坐标列表，供 SAM 备用
    QJsonArray m_currentBBoxArray; 
    QList<QRect> m_currentRects;   

    QAction* actImportImg;
    QAction* actImportPCL;
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
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication app(argc, argv);

    MainWindow window;
    window.show();

    return app.exec();
}

#include "main.moc"