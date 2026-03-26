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
    void requestAIInference(const QString &prompt) {
        QPixmap currentPix = m_imageWidget->currentPixmap();
        if (currentPix.isNull()) {
            QMessageBox::warning(this, "错误", "请先导入街景图像！");
            return;
        }

        // 任务2：输入后立即更新属性面板的要素类别
        updatePropertyTable(prompt, -1.0); 

        statusBar()->showMessage(QString::fromUtf8("AI 正在识别中..."));

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
                    // 解析坐标
                    QJsonArray bboxArr = response["bbox"].toArray();
                    QRect rect(bboxArr[0].toInt(), bboxArr[1].toInt(), 
                               bboxArr[2].toInt() - bboxArr[0].toInt(), 
                               bboxArr[3].toInt() - bboxArr[1].toInt());
                    
                    // 解析 Mask
                    QImage maskImg;
                    maskImg.loadFromData(QByteArray::fromBase64(response["mask_base64"].toString().toLatin1()), "PNG");

                    m_imageWidget->setDetectionResult(rect, maskImg);
                    
                    // 任务3：更新识别置信度为百分比
                    double conf = response["confidence"].toDouble();
                    updatePropertyTable(prompt, conf);

                    statusBar()->showMessage(QString::fromUtf8("识别成功！"));
                }
            } else {
                QMessageBox::critical(this, "错误", "后端服务未响应");
            }
            reply->deleteLater();
        });
    }


    // --- 新增：解析后端返回的 JSON ---
    void parseAIResponse(const QByteArray &data) {
        QJsonDocument jsonDoc = QJsonDocument::fromJson(data);
        QJsonObject jsonObj = jsonDoc.object();

        if (jsonObj["status"].toString() == "success") {
            // 解析 BBox: [xmin, ymin, xmax, ymax]
            QJsonArray bboxArr = jsonObj["bbox"].toArray();
            if (bboxArr.size() == 4) {
                QRect rect(bboxArr[0].toInt(), bboxArr[1].toInt(), 
                           bboxArr[2].toInt() - bboxArr[0].toInt(), 
                           bboxArr[3].toInt() - bboxArr[1].toInt());
                
                // 解析 Mask Base64
                QString maskB64 = jsonObj["mask_base64"].toString();
                QByteArray maskData = QByteArray::fromBase64(maskB64.toLatin1());
                QImage maskImg;
                maskImg.loadFromData(maskData, "PNG");

                // 将结果更新到 ImageWidget 进行绘制
                // 假设 ImageWidget 有这个接口来接收 BBox 和 Mask
                m_imageWidget->setDetectionResult(rect, maskImg);
                
                statusBar()->showMessage(QString::fromUtf8("识别成功！置信度: ") + QString::number(jsonObj["confidence"].toDouble()));
            }
        } else {
            statusBar()->showMessage(QString::fromUtf8("识别失败: ") + jsonObj["message"].toString());
        }
    }
    void setupActions() {
        // 数据操作
        actImportImg = new QAction(QString::fromUtf8("导入街景图像"), this);
        actImportPCL = new QAction(QString::fromUtf8("导入街景点云(PCL) [延后]"), this);
        
        // 核心算法
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
        // --- 右侧：要素属性面板 (将 m_propTable 设为成员变量) ---
        QDockWidget* rightDock = new QDockWidget(QString::fromUtf8("要素属性面板"), this);
        m_propTable = new QTableWidget(5, 2, rightDock); // 使用成员变量
        m_propTable->setHorizontalHeaderLabels(QStringList() << QString::fromUtf8("属性名") << QString::fromUtf8("属性值"));
        m_propTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        m_propTable->verticalHeader()->setVisible(false);
        
        // 初始化表格内容
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
            // 设置第一列不可编辑
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
            QString path = QFileDialog::getOpenFileName(this, "Open Image", "", "Images (*.png *.jpg)");
            if(!path.isEmpty()) {
                m_imageWidget->loadImage(path);
                // 加载新图时清空属性表
                updatePropertyTable(QString::fromUtf8("待识别"), 0.0);
            }
        });

        // 识别动作
        connect(actLoadVL, &QAction::triggered, this, [this]() {
            bool ok;
            QString text = QInputDialog::getText(this, "Input", "Prompt:", QLineEdit::Normal, "traffic sign", &ok);
            if (ok && !text.isEmpty()) {
                requestAIInference(text);
            }
        });
    }

    QTableWidget* m_propTable;
    QNetworkAccessManager* m_networkManager; // 新增网络管理器
    ImageWidget* m_imageWidget;
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