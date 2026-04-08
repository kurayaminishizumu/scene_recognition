#include "inputoutput.h"
#include <QFileDialog>
#include <QMessageBox>
#include <QFileInfo>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QTextStream>

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

bool exportGeoJSON(QWidget* parent, const QList<QPolygonF>& polygons) {
    if (polygons.isEmpty()) {
        QMessageBox::warning(parent, QString::fromUtf8("提示"), QString::fromUtf8("没有可导出的矢量数据！"));
        return false;
    }

    // 明确限定为 GeoJSON 格式导出，并提供默认文件名
    QString filePath = QFileDialog::getSaveFileName(
        parent,
        QString::fromUtf8("导出结构化矢量数据"),
        "output_vector.geojson", 
        QString::fromUtf8("GeoJSON format (*.geojson)")
    );

    if (filePath.isEmpty()) {
        return false;
    }

    QJsonArray featuresArray;
    for (const auto& poly : polygons) {
        QJsonArray coordinatesArray;
        QJsonArray ringArray;
        for (int i = 0; i < poly.size(); ++i) {
            QJsonArray point;
            point.append(poly[i].x());
            point.append(poly[i].y());
            ringArray.append(point);
        }
        
        // GeoJSON 要求多边形闭合 (首尾点相同)
        if (poly.size() > 0 && poly.first() != poly.last()) {
            QJsonArray point;
            point.append(poly.first().x());
            point.append(poly.first().y());
            ringArray.append(point);
        }
        
        coordinatesArray.append(ringArray);

        QJsonObject geometry;
        geometry["type"] = "Polygon";
        geometry["coordinates"] = coordinatesArray;

        QJsonObject properties;
        properties["name"] = "Extracted Feature";

        QJsonObject feature;
        feature["type"] = "Feature";
        feature["geometry"] = geometry;
        feature["properties"] = properties;

        featuresArray.append(feature);
    }

    QJsonObject featureCollection;
    featureCollection["type"] = "FeatureCollection";
    featureCollection["features"] = featuresArray;

    QJsonDocument doc(featureCollection);
    QFile file(filePath);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QMessageBox::critical(parent, QString::fromUtf8("错误"), QString::fromUtf8("无法打开文件进行写入！"));
        return false;
    }

    QTextStream out(&file);
    out << doc.toJson(QJsonDocument::Indented);
    file.close();

    QMessageBox::information(parent, 
        QString::fromUtf8("导出成功"), 
        QString::fromUtf8("2D矢量多边形数据已成功保存至：\n") + filePath);
        
    return true;
}

} // namespace IOHelper
