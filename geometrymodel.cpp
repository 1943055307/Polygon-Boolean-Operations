#include "geometrymodel.h"

#include <algorithm>
#include <cmath>

namespace Geometry {


static inline double dot2d(const QPointF& a, const QPointF& b) {
    return a.x()*b.x() + a.y()*b.y();
}

static inline double cross2d(const QPointF& a, const QPointF& b) {
    return a.x()*b.y() - a.y()*b.x();
}

static inline QPointF lerpPoint(const QPointF& a, const QPointF& b, double t) {
    return QPointF(
        a.x() + (b.x() - a.x()) * t,
        a.y() + (b.y() - a.y()) * t
        );
}

static bool intervalIntersection(double a0, double a1, double b0, double b1, double& lo, double& hi) {
    if (a0 > a1) std::swap(a0, a1);
    if (b0 > b1) std::swap(b0, b1);
    lo = std::max(a0, b0);
    hi = std::min(a1, b1);
    return (hi >= lo);
}

static void injectSelfCollinearCuts(const PolygonTopo& poly, const QVector<RawEdge>& rawEdges, QVector<EdgeWork>& work, double epsGeom, double epsParam) {
    const int m = rawEdges.size();
    for (int i = 0; i < m; ++i) {
        const auto& ei = rawEdges[i];
        const QPointF A0 = poly.verts[ei.vStart].pos;
        const QPointF A1 = poly.verts[ei.vEnd  ].pos;
        for (int j = i + 1; j < m; ++j) {
            const auto& ej = rawEdges[j];
            const QPointF B0 = poly.verts[ej.vStart].pos;
            const QPointF B1 = poly.verts[ej.vEnd  ].pos;
            SegmentIntersection inter = intersectSegments(A0, A1, B0, B1, epsGeom);
            if (inter.type == IntersectType::Overlap) {
                work[i].cutParams.push_back(inter.tA0);
                work[i].cutParams.push_back(inter.tA1);
                work[j].cutParams.push_back(inter.tB0);
                work[j].cutParams.push_back(inter.tB1);
            } else if (inter.type == IntersectType::Point) {
                work[i].cutParams.push_back(inter.tA);
                work[j].cutParams.push_back(inter.tB);
            }
        }
    }
}

QVector<RawEdge> buildRawEdges(const PolygonTopo& poly, bool fromA) {
    QVector<RawEdge> edges;
    for (int lid = 0; lid < poly.loops.size(); ++lid) {
        const auto& loop = poly.loops[lid];
        const auto& lv   = loop.loopVertices;
        const int n = lv.size();
        if (n < 2) continue;
        for (int i = 0; i < n; ++i) {
            RawEdge e;
            e.loopId = lid;
            e.vStart = lv[i];
            e.vEnd   = lv[(i+1) % n];
            e.fromA  = fromA;
            edges.push_back(e);
        }
    }
    return edges;
}

SegmentIntersection intersectSegments(const QPointF& A0, const QPointF& A1, const QPointF& B0, const QPointF& B1, double epsGeom) {
    SegmentIntersection out;
    QPointF r = A1 - A0;
    QPointF s = B1 - B0;
    double rxs = cross2d(r, s);
    QPointF diff = B0 - A0;
    double diffxr = cross2d(diff, r);
    if (std::fabs(rxs) > epsGeom) {
        double t = cross2d(diff, s) / rxs;
        double u = cross2d(diff, r) / rxs;
        if (t >= -epsGeom && t <= 1.0+epsGeom &&
            u >= -epsGeom && u <= 1.0+epsGeom) {
            if (t < 0.0) t = 0.0;
            if (t > 1.0) t = 1.0;
            if (u < 0.0) u = 0.0;
            if (u > 1.0) u = 1.0;
            out.type = IntersectType::Point;
            out.tA = t;
            out.tB = u;
            out.P = A0 + r * t;
        }
        return out;
    }
    if (std::fabs(diffxr) > epsGeom) {
        return out;
    }
    double rr = dot2d(r, r);
    double ss = dot2d(s, s);
    auto paramOnA = [&](const QPointF& P)->double {
        if (rr < epsGeom) return 0.0;
        return dot2d(P - A0, r) / rr;
    };
    double tA_for_B0 = paramOnA(B0);
    double tA_for_B1 = paramOnA(B1);
    double tA_lo, tA_hi;
    if (!intervalIntersection(0.0, 1.0, tA_for_B0, tA_for_B1, tA_lo, tA_hi)) {
        return out;
    }
    auto paramOnB = [&](const QPointF& P)->double {
        if (ss < epsGeom) return 0.0;
        return dot2d(P - B0, s) / ss;
    };
    double tB_for_A0 = paramOnB(A0);
    double tB_for_A1 = paramOnB(A1);
    double tB_lo, tB_hi;
    if (!intervalIntersection(0.0, 1.0, tB_for_A0, tB_for_A1, tB_lo, tB_hi)) {
        return out;
    }
    double lenA = tA_hi - tA_lo;
    double lenB = tB_hi - tB_lo;
    if (lenA <= epsGeom && lenB <= epsGeom) {
        out.type = IntersectType::Point;
        double tA_mid = 0.5 * (tA_lo + tA_hi);
        double tB_mid = 0.5 * (tB_lo + tB_hi);
        out.tA = tA_mid;
        out.tB = tB_mid;
        out.P  = lerpPoint(A0, A1, tA_mid);
        return out;
    }
    if (tA_lo > tA_hi) std::swap(tA_lo, tA_hi);
    if (tB_lo > tB_hi) std::swap(tB_lo, tB_hi);
    out.type = IntersectType::Overlap;
    out.tA0  = tA_lo;
    out.tA1  = tA_hi;
    out.tB0  = tB_lo;
    out.tB1  = tB_hi;
    return out;
}

static QVector<AtomicSegment> explodeEdgeWork(const EdgeWork& ew, const PolygonTopo& poly,double epsParam) {
    QVector<AtomicSegment> out;
    if (ew.cutParams.isEmpty()) return out;
    QVector<double> params = ew.cutParams;
    std::sort(params.begin(), params.end());
    params.erase(std::unique(params.begin(), params.end(), [epsParam](double a, double b){
        return std::fabs(a - b) < epsParam;
    }),
    params.end());
    const QPointF P0 = poly.verts[ew.edge.vStart].pos;
    const QPointF P1 = poly.verts[ew.edge.vEnd ].pos;
    auto isInOverlap = [&](double t0, double t1)->bool {
        for (const auto& ov : ew.overlaps) {
            double a = ov.t0;
            double b = ov.t1;
            if (a > b) std::swap(a,b);
            if (t0 >= a - epsParam && t1 <= b + epsParam) {
                return true;
            }
        }
        return false;
    };
    for (int k = 0; k+1 < params.size(); ++k) {
        double tLo = params[k];
        double tHi = params[k+1];
        if (tHi - tLo < epsParam) {
            continue;
        }
        QPointF A = lerpPoint(P0, P1, tLo);
        QPointF B = lerpPoint(P0, P1, tHi);
        AtomicSegment seg;
        seg.p0 = A;
        seg.p1 = B;
        seg.fromA = ew.edge.fromA;
        seg.loopId = ew.edge.loopId;
        seg.coincidentWithOther = isInOverlap(tLo, tHi);
        out.push_back(seg);
    }
    return out;
}

QVector<AtomicSegment> computeAtomicSegments(const PolygonTopo& polyA, const PolygonTopo& polyB, double epsGeom, double epsParam) {
    QVector<RawEdge> rawA = buildRawEdges(polyA, /*fromA=*/true);
    QVector<RawEdge> rawB = buildRawEdges(polyB, /*fromA=*/false);
    QVector<EdgeWork> workA, workB;
    workA.reserve(rawA.size());
    workB.reserve(rawB.size());
    for (const auto& e : rawA) { EdgeWork w; w.edge = e; w.cutParams = {0.0, 1.0}; workA.push_back(w); }
    for (const auto& e : rawB) { EdgeWork w; w.edge = e; w.cutParams = {0.0, 1.0}; workB.push_back(w); }
    injectSelfCollinearCuts(polyA, rawA, workA, epsGeom, epsParam);
    injectSelfCollinearCuts(polyB, rawB, workB, epsGeom, epsParam);
    for (int i = 0; i < workA.size(); ++i) {
        for (int j = 0; j < workB.size(); ++j) {
            const QPointF A0 = polyA.verts[ workA[i].edge.vStart ].pos;
            const QPointF A1 = polyA.verts[ workA[i].edge.vEnd   ].pos;
            const QPointF B0 = polyB.verts[ workB[j].edge.vStart ].pos;
            const QPointF B1 = polyB.verts[ workB[j].edge.vEnd   ].pos;
            SegmentIntersection inter = intersectSegments(A0, A1, B0, B1, epsGeom);
            if (inter.type == IntersectType::None) continue;
            if (inter.type == IntersectType::Point) {
                workA[i].cutParams.push_back(inter.tA);
                workB[j].cutParams.push_back(inter.tB);
            } else if (inter.type == IntersectType::Overlap) {
                workA[i].cutParams.push_back(inter.tA0);
                workA[i].cutParams.push_back(inter.tA1);
                workA[i].overlaps.push_back({ inter.tA0, inter.tA1 });

                workB[j].cutParams.push_back(inter.tB0);
                workB[j].cutParams.push_back(inter.tB1);
                workB[j].overlaps.push_back({ inter.tB0, inter.tB1 });
            }
        }
    }
    QVector<AtomicSegment> allSegs;
    allSegs.reserve(workA.size() * 2 + workB.size() * 2);
    for (const auto& ew : workA) {
        QVector<AtomicSegment> parts = explodeEdgeWork(ew, polyA, epsParam);
        for (const auto& seg : parts) {
            allSegs.push_back(seg);
        }
    }
    for (const auto& ew : workB) {
        QVector<AtomicSegment> parts = explodeEdgeWork(ew, polyB, epsParam);
        for (const auto& seg : parts) {
            allSegs.push_back(seg);
        }
    }
    return allSegs;
}

}
