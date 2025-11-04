#include "inputpolygon.h"

#include <QFile>
#include <QIODevice>
#include <QTextStream>
#include <QRegularExpression>
#include <QtGlobal>
#include <QDebug>

static inline bool almostSame(const QPointF& a, const QPointF& b, qreal eps = 1e-3) {
    return qAbs(a.x() - b.x()) <= eps &&
           qAbs(a.y() - b.y()) <= eps;
}

void InputPolygon::clearPolygon() noexcept {
    outer.clear();
    holes.clear();
}

bool InputPolygon::loadData(const QString& filePath, QString* error) {
    clearPolygon();

    QFile loadFile(filePath);
    if (!loadFile.open(QIODevice::ReadOnly | QIODevice::Text)) {
        if (error) {
            *error = QStringLiteral(
                         "ERROR: FAIL TO OPEN FILE %1. (%2)."
                         ).arg(filePath, loadFile.errorString());
        }
        return false;
    }
    QTextStream textStream(&loadFile);
    int lineCount = 0;
    QVector<QPointF> currentLoop;
    auto flushCurrentLoop = [&]() {
        if (currentLoop.isEmpty())
            return;
        if (currentLoop.size() >= 2 &&
            almostSame(currentLoop.first(), currentLoop.last())) {
            currentLoop.pop_back();
        }
        if (outer.isEmpty()) {
            outer = currentLoop;
        } else {
            holes.push_back(currentLoop);
        }
        currentLoop.clear();
    };
    static const QRegularExpression sep("[,\\s]+");
    while (!textStream.atEnd()) {
        QString line = textStream.readLine();
        ++lineCount;
        if (line.isNull()) {
            break;
        }
        line = line.trimmed();
        if (line.isEmpty()) {
            continue;
        }
        if (line.startsWith('#')) {
            if (line.startsWith("#loop", Qt::CaseInsensitive)) {
                flushCurrentLoop();
            }
            continue;
        }
        const QStringList parts = line.split(sep, Qt::SkipEmptyParts);
        if (parts.size() < 2) {
            if (error) {
                *error = QStringLiteral(
                             "ERROR: WRONG FORMAT AT LINE %1."
                             ).arg(lineCount);
            }
            clearPolygon();
            return false;
        }
        bool okX = false;
        bool okY = false;
        const qreal x = parts[0].toDouble(&okX);
        const qreal y = parts[1].toDouble(&okY);
        if (!(okX && okY)) {
            if (error) {
                *error = QStringLiteral(
                             "ERROR: INVALID VALUE AT LINE %1."
                             ).arg(lineCount);
            }
            clearPolygon();
            return false;
        }
        currentLoop.push_back(QPointF(x, y));
    }
    flushCurrentLoop();
    if (outer.isEmpty()) {
        if (error) {
            *error = QStringLiteral(
                         "ERROR: NO OUTER LOOP FOUND IN FILE %1."
                         ).arg(filePath);
        }
        clearPolygon();
        return false;
    }
    qDebug() << "[inputPolygon] outer points:" << outer.size();
    qDebug() << "[inputPolygon] holes:" << holes.size();
    for (int i = 0; i < holes.size(); ++i) {
        qDebug() << "    hole" << i << "points:" << holes[i].size();
    }
    return true;
}
