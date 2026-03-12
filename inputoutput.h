#ifndef INPUTOUTPUT_H
#define INPUTOUTPUT_H

#include <QString>
#include <QWidget>

namespace IOHelper {

    // 导入点云数据弹窗 (PCD, PLY, LAS 等)
    // 参数 parent 用于让弹窗依附于主窗口，避免弹窗成为独立的游离窗口
    QString importPointCloud(QWidget* parent);

    // 导入全景图像弹窗 (JPG, PNG 等)
    QString importImage(QWidget* parent);

    // 导出GIS矢量数据弹窗 (GeoJSON, SHP 等)
    QString exportGISData(QWidget* parent);

}

#endif // INPUTOUTPUT_H