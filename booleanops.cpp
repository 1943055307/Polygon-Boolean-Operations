#include "booleanops.h"
#include <cmath>

static QVector<QPointF> normalizeLoop(const QVector<QPointF>& inLoop, double epsClose) {
    if (inLoop.size() >= 2) {
        const QPointF& a0 = inLoop.front();
        const QPointF& a1 = inLoop.back();
        double dx = a0.x() - a1.x();
        double dy = a0.y() - a1.y();
        if (dx*dx + dy*dy < epsClose*epsClose) {
            QVector<QPointF> tmp = inLoop;
            tmp.pop_back();
            return tmp;
        }
    }
    return inLoop;
}

static bool pointInSimpleLoop(const QVector<QPointF>& loop, const QPointF& p, double eps = 1e-9) {
    const int n = loop.size();
    if (n < 3) return false;
    for (int i = 0; i < n; ++i) {
        QPointF a = loop[i];
        QPointF b = loop[(i+1) % n];
        QPointF ap = p - a;
        QPointF ab = b - a;
        double cross = ap.x() * ab.y() - ap.y() * ab.x();
        if (std::fabs(cross) < eps) {
            double dot = ap.x() * ab.x() + ap.y() * ab.y();
            if (dot >= -eps) {
                double ab2 = ab.x() * ab.x() + ab.y() * ab.y();
                if (dot <= ab2 + eps) {
                    return true;
                }
            }
        }
    }
    bool inside = false;
    for (int i = 0; i < n; ++i) {
        const QPointF& a = loop[i];
        const QPointF& b = loop[(i+1)%n];
        bool condY = ((a.y() > p.y()) != (b.y() > p.y()));
        if (condY) {
            double t = (p.y() - a.y()) / (b.y() - a.y());
            double xHit = a.x() + t * (b.x() - a.x());
            if (xHit >= p.x() - eps) {
                inside = !inside;
            }
        }
    }
    return inside;
}

static bool pointInPolygonWithHoles(const InputPolygon& poly, const QPointF& p) {
    if (!pointInSimpleLoop(poly.outerLoop(), p)) return false;
    for (const auto& h : poly.holeLoops()) {
        if (pointInSimpleLoop(h, p)) return false;
    }
    return true;
}

static bool coincidentOpposite(const Geometry::AtomicSegment& seg, const InputPolygon& polyA, const InputPolygon& polyB) {
    QPointF mid(0.5 * (seg.p0.x() + seg.p1.x()), 0.5 * (seg.p0.y() + seg.p1.y()));
    QPointF dir(seg.p1.x() - seg.p0.x(), seg.p1.y() - seg.p0.y());
    QPointF n(dir.y(), -dir.x());
    double nlen = std::hypot(n.x(), n.y());
    if (nlen < 1e-12) {
        return false;
    }
    n.setX(n.x() / nlen);
    n.setY(n.y() / nlen);
    const double epsProbe = 1e-4;
    QPointF pPlus(mid.x() + epsProbe * n.x(), mid.y() + epsProbe * n.y());
    QPointF pMinus(mid.x() - epsProbe * n.x(), mid.y() - epsProbe * n.y());
    bool inA_plus = pointInPolygonWithHoles(polyA, pPlus);
    bool inA_minus = pointInPolygonWithHoles(polyA, pMinus);
    bool inB_plus = pointInPolygonWithHoles(polyB, pPlus);
    bool inB_minus = pointInPolygonWithHoles(polyB, pMinus);
    bool oppCase1 = inA_plus && !inB_plus && !inA_minus && inB_minus;
    bool oppCase2 = !inA_plus && inB_plus && inA_minus && !inB_minus;
    return (oppCase1 || oppCase2);
}

static QVector<QVector<QPointF>> segmentsToPolylines(
    const QVector<Geometry::AtomicSegment>& segs) {
    QVector<QVector<QPointF>> out;
    out.reserve(segs.size());
    for (const auto& s : segs) {
        QVector<QPointF> line;
        line.push_back(s.p0);
        line.push_back(s.p1);
        out.push_back(line);
    }
    return out;
}

namespace Boolean2D {
Geometry::PolygonTopo makeTopoFromInput(const InputPolygon& poly, double epsClose) {
    Geometry::PolygonTopo topo;
    topo.verts.clear();
    topo.loops.clear();
    auto appendLoop = [&](const QVector<QPointF>& rawLoopPts) {
        QVector<QPointF> L = normalizeLoop(rawLoopPts, epsClose);
        if (L.size() < 3)
            return;
        Geometry::LoopTopo loopTopo;
        loopTopo.loopVertices.reserve(L.size());
        for (const QPointF& pt : L) {
            Geometry::Vertex v;
            v.pos = pt;
            int idx = topo.verts.size();
            topo.verts.push_back(v);
            loopTopo.loopVertices.push_back(idx);
        }
        topo.loops.push_back(loopTopo);
    };
    appendLoop(poly.outerLoop());
    for (const auto& h : poly.holeLoops()) {
        appendLoop(h);
    }
    return topo;
}

PrepContext prepare(const InputPolygon& polyA, const InputPolygon& polyB, double epsGeom, double epsParam) {
    PrepContext ctx;
    ctx.topoA = makeTopoFromInput(polyA);
    ctx.topoB = makeTopoFromInput(polyB);
    ctx.atoms = Geometry::computeAtomicSegments(ctx.topoA, ctx.topoB, epsGeom, epsParam);
    return ctx;
}

static QVector<Geometry::AtomicSegment> classifyForAddition(const PrepContext& ctx, const InputPolygon& polyA, const InputPolygon& polyB) {
    QVector<Geometry::AtomicSegment> keep;
    keep.reserve(ctx.atoms.size());
    for (const auto& seg : ctx.atoms) {
        if (seg.coincidentWithOther) {
            bool opp = coincidentOpposite(seg, polyA, polyB);
            if (!opp) {
                if (seg.fromA) keep.push_back(seg);
            }
            continue;
        }
        QPointF mid(0.5 * (seg.p0.x() + seg.p1.x()), 0.5 * (seg.p0.y() + seg.p1.y()));
        bool inA = pointInPolygonWithHoles(polyA, mid);
        bool inB = pointInPolygonWithHoles(polyB, mid);
        bool useIt = false;
        if (seg.fromA) {
            if (!inB) useIt = true;
        } else {
            if (!inA) useIt = true;
        }
        if (useIt) keep.push_back(seg);
    }
    return keep;
}

static QVector<Geometry::AtomicSegment> classifyForIntersection(const PrepContext& ctx, const InputPolygon& polyA, const InputPolygon& polyB) {
    QVector<Geometry::AtomicSegment> keep;
    keep.reserve(ctx.atoms.size());
    for (const auto& seg : ctx.atoms) {
        if (seg.coincidentWithOther) {
            bool opp = coincidentOpposite(seg, polyA, polyB);
            if (!opp) {
                if (seg.fromA)
                    keep.push_back(seg);
            }
            continue;
        }
        QPointF mid(0.5 * (seg.p0.x() + seg.p1.x()), 0.5 * (seg.p0.y() + seg.p1.y()));
        bool inA = pointInPolygonWithHoles(polyA, mid);
        bool inB = pointInPolygonWithHoles(polyB, mid);
        bool useIt = false;
        if (seg.fromA) {
            if (inB) useIt = true;
        } else {
            if (inA) useIt = true;
        }
        if (useIt) keep.push_back(seg);
    }
    return keep;
}

static QVector<Geometry::AtomicSegment> classifyForSubAB(const PrepContext& ctx, const InputPolygon& polyA, const InputPolygon& polyB) {
    QVector<Geometry::AtomicSegment> keep;
    keep.reserve(ctx.atoms.size());
    for (const auto& seg : ctx.atoms) {
        if (seg.coincidentWithOther) {
            bool opp = coincidentOpposite(seg, polyA, polyB);
            if (opp) {
                if (seg.fromA)
                    keep.push_back(seg);
            }
            continue;
        }
        QPointF mid(0.5 * (seg.p0.x() + seg.p1.x()), 0.5 * (seg.p0.y() + seg.p1.y()));
        bool inA = pointInPolygonWithHoles(polyA, mid);
        bool inB = pointInPolygonWithHoles(polyB, mid);
        bool useIt = false;
        if (seg.fromA) {
            if (seg.loopId > 0) {
                if (!inB) useIt = true;
            } else {
                if (inA && !inB) useIt = true;
            }
        } else {
            if (seg.loopId > 0) {
                if (inA && !inB) useIt = true;
            } else {
                if (inA && inB) useIt = true;
            }
        }
        if (useIt) keep.push_back(seg);
    }
    return keep;
}

static QVector<Geometry::AtomicSegment> classifyForSubBA(const PrepContext& ctx, const InputPolygon& polyA, const InputPolygon& polyB) {
    QVector<Geometry::AtomicSegment> keep;
    keep.reserve(ctx.atoms.size());
    for (const auto& seg : ctx.atoms) {
        if (seg.coincidentWithOther) {
            bool opp = coincidentOpposite(seg, polyA, polyB);
            if (opp) {
                if (!seg.fromA)
                    keep.push_back(seg);
            }
            continue;
        }
        QPointF mid(0.5*(seg.p0.x()+seg.p1.x()), 0.5*(seg.p0.y()+seg.p1.y()));
        bool inA = pointInPolygonWithHoles(polyA, mid);
        bool inB = pointInPolygonWithHoles(polyB, mid);
        bool useIt = false;
        if (!seg.fromA) {
            if (seg.loopId > 0) {
                if (!inA) useIt = true;
            } else {
                if (inB && !inA) useIt = true;
            }
        } else {
            if (seg.loopId > 0) {
                if (inB && !inA) useIt = true;
            } else {
                if (inA && inB) useIt = true;
            }
        }
        if (useIt) keep.push_back(seg);
    }
    return keep;
}

QVector<QVector<QPointF>> computeAdditionSegments(const PrepContext& ctx, const InputPolygon& polyA, const InputPolygon& polyB) {
    auto kept = classifyForAddition(ctx, polyA, polyB);
    return segmentsToPolylines(kept);
}

QVector<QVector<QPointF>> computeIntersectionSegments(const PrepContext& ctx, const InputPolygon& polyA, const InputPolygon& polyB) {
    auto kept = classifyForIntersection(ctx, polyA, polyB);
    return segmentsToPolylines(kept);
}

QVector<QVector<QPointF>> computeSubtractionABSegments(const PrepContext& ctx, const InputPolygon& polyA, const InputPolygon& polyB) {
    auto kept = classifyForSubAB(ctx, polyA, polyB);
    return segmentsToPolylines(kept);
}

QVector<QVector<QPointF>> computeSubtractionBASegments(const PrepContext& ctx, const InputPolygon& polyA, const InputPolygon& polyB) {
    auto kept = classifyForSubBA(ctx, polyA, polyB);
    return segmentsToPolylines(kept);
}

}
