#ifndef INPUTOUTPUT_H
#define INPUTOUTPUT_H

#include <QString>
#include <QWidget>
#include <QList>
#include <QPolygonF>

namespace IOHelper {
    QString importImage(QWidget* parent);
    bool exportGeoJSON(QWidget* parent, const QList<QPolygonF>& polygons);

}

#endif // INPUTOUTPUT_H
