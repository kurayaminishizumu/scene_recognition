#include "inputoutput.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>

namespace IOHelper {

QString importPointCloud(QWidget* parent) {
    // 弹出文件选择框，指定后缀过滤
    QString filePath = QFileDialog::getOpenFileName(
        parent,
        QString::fromUtf8("导入街景点云数据"),
        "",
        QString::fromUtf8("Point Cloud Files (*.pcd *.ply *.las);;All Files (*.*)")
    );

    // 如果用户没有取消选择
    if (!filePath.isEmpty()) {
        QFileInfo fileInfo(filePath);
        QString msg = QString::fromUtf8("成功获取点云路径：\n") + filePath + 
                      QString::fromUtf8("\n\n文件大小: ") + QString::number(fileInfo.size() / 1024 / 1024) + " MB";
                      
        QMessageBox::information(parent, QString::fromUtf8("导入成功"), msg);
    }
    return filePath;
}

QString importImage(QWidget* parent) {
    QString filePath = QFileDialog::getOpenFileName(
        parent,
        QString::fromUtf8("导入街景全景图像"),
        "",
        QString::fromUtf8("Images (*.png *.jpg *.jpeg *.tif);;All Files (*.*)")
    );

    if (!filePath.isEmpty()) {
        QMessageBox::information(parent, 
            QString::fromUtf8("导入成功"), 
            QString::fromUtf8("成功获取图像路径：\n") + filePath);
    }
    return filePath;
}

QString exportGISData(QWidget* parent) {
    // 导出使用 getSaveFileName
    QString filePath = QFileDialog::getSaveFileName(
        parent,
        QString::fromUtf8("导出结构化矢量数据"),
        "",
        QString::fromUtf8("GeoJSON (*.geojson);;ESRI Shapefile (*.shp)")
    );

    if (!filePath.isEmpty()) {
        QMessageBox::information(parent, 
            QString::fromUtf8("导出准备就绪"), 
            QString::fromUtf8("矢量数据将保存至：\n") + filePath);
    }
    return filePath;
}

} // namespace IOHelper