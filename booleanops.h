#pragma once
#include <QVector>
#include <QPointF>
#include "inputpolygon.h"
#include "geometrymodel.h"

namespace Boolean2D {

struct PrepContext {
    Geometry::PolygonTopo topoA;
    Geometry::PolygonTopo topoB;
    QVector<Geometry::AtomicSegment> atoms;
};

Geometry::PolygonTopo makeTopoFromInput(const InputPolygon& poly, double epsClose = 1e-9);

PrepContext prepare(const InputPolygon& polyA, const InputPolygon& polyB, double epsGeom = 1e-3, double epsParam = 1e-3);

QVector<QVector<QPointF>> computeAdditionSegments(const PrepContext& ctx, const InputPolygon& polyA, const InputPolygon& polyB);

QVector<QVector<QPointF>> computeIntersectionSegments(const PrepContext& ctx, const InputPolygon& polyA, const InputPolygon& polyB);

QVector<QVector<QPointF>> computeSubtractionABSegments(const PrepContext& ctx, const InputPolygon& polyA, const InputPolygon& polyB);

QVector<QVector<QPointF>> computeSubtractionBASegments(const PrepContext& ctx, const InputPolygon& polyA, const InputPolygon& polyB);

}
