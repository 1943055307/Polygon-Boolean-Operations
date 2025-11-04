#pragma once
#include <QVector>
#include <QPointF>
#include <QString>

class InputPolygon {
public:
    InputPolygon() = default;
    ~InputPolygon() = default;

    bool loadData(const QString& filePath, QString* error = nullptr);
    void clearPolygon() noexcept;
    bool checkEmpty() const noexcept { return outer.isEmpty(); }
    int outerPointCount() const noexcept { return outer.size(); }
    const QVector<QPointF>& outerLoop() const noexcept { return outer; }
    const QVector<QVector<QPointF>>& holeLoops() const noexcept { return holes; }

private:
    QVector<QPointF> outer;
    QVector<QVector<QPointF>> holes;
};
