#ifndef INPUTOUTPUT_H
#define INPUTOUTPUT_H

#include <QString>
#include <QWidget>

namespace IOHelper {
    
    // 仅保留 2D 图像导入
    QString importImage(QWidget* parent);
    
    // 明确命名为导出 GeoJSON（取代原有的 exportGISData）
    QString exportGeoJSON(QWidget* parent);

}

#endif // INPUTOUTPUT_H