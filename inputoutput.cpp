#include "inputoutput.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>

namespace IOHelper {

QString importImage(QWidget* parent) {
    // 针对 2D 图像的导入对话框
    QString filePath = QFileDialog::getOpenFileName(
        parent,
        QString::fromUtf8("导入街景图像"),
        "",
        QString::fromUtf8("Image Files (*.png *.jpg *.jpeg *.bmp *.tif);;All Files (*.*)")
    );

    if (!filePath.isEmpty()) {
        QFileInfo fileInfo(filePath);
        QString msg = QString::fromUtf8("成功获取图像路径：\n") + filePath + 
                      QString::fromUtf8("\n\n文件大小: ") + QString::number(fileInfo.size() / 1024) + " KB";
                      
        QMessageBox::information(parent, QString::fromUtf8("导入成功"), msg);
    }
    return filePath;
}

QString exportGeoJSON(QWidget* parent) {
    // 明确限定为 GeoJSON 格式导出，并提供默认文件名
    QString filePath = QFileDialog::getSaveFileName(
        parent,
        QString::fromUtf8("导出结构化矢量数据"),
        "output_vector.geojson", 
        QString::fromUtf8("GeoJSON format (*.geojson)")
    );

    if (!filePath.isEmpty()) {
        QMessageBox::information(parent, 
            QString::fromUtf8("导出准备就绪"), 
            QString::fromUtf8("2D矢量多边形数据将保存至：\n") + filePath);
    }
    return filePath;
}

} // namespace IOHelper