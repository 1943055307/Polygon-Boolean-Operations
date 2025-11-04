#pragma once
#include <QVector>
#include <QPointF>
#include <QtGlobal>

namespace Geometry {

struct Vertex {
    QPointF pos;
    bool    isIntersection = false; // intersection point
};

struct LoopTopo {
    QVector<int> loopVertices; // index
    bool isHole = false; // false : outer contour, true : hole
};

struct PolygonTopo {
    QVector<Vertex>   verts;
    QVector<LoopTopo> loops; // outer coutour + holes
};

struct RawEdge {
    int  loopId;
    int  vStart; // index in verts
    int  vEnd; // index in verts
    bool fromA; // true : from A, false : from B
};

struct OverlapInterval {
    double t0; // start t on segment
    double t1; // end t on segment
};

struct EdgeWork {
    RawEdge edge;
    QVector<double>          cutParams; // begin with {0.0 , 1.0}
    QVector<OverlapInterval> overlaps;
};

struct AtomicSegment {
    QPointF p0;
    QPointF p1;
    bool    fromA; // true : from A, false : from B
    bool    coincidentWithOther; // on-on candidate
    int     loopId;
};

enum class IntersectType {
    None,
    Point, // single point
    Overlap // overlap
};

struct SegmentIntersection {
    IntersectType type = IntersectType::None;

    // Point
    double tA = 0.0;
    double tB = 0.0;
    QPointF P;

    // Overlap
    double tA0 = 0.0;
    double tA1 = 0.0;
    double tB0 = 0.0;
    double tB1 = 0.0;
};

QVector<RawEdge> buildRawEdges(const PolygonTopo& poly, bool fromA);

SegmentIntersection intersectSegments(
    const QPointF& A0, const QPointF& A1,
    const QPointF& B0, const QPointF& B1,
    double epsGeom
    );

QVector<AtomicSegment> computeAtomicSegments(
    const PolygonTopo& polyA,
    const PolygonTopo& polyB,
    double epsGeom   = 1e-3,
    double epsParam  = 1e-9
    );

}
