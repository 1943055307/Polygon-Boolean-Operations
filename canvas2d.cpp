#include "canvas2d.h"
#include <QtMath>
#include <cmath>
#include <algorithm>
#include <QDebug>

static inline double absmax4(double a, double b, double c, double d) {
    double m = std::fabs(a);
    m = std::max(m, std::fabs(b));
    m = std::max(m, std::fabs(c));
    m = std::max(m, std::fabs(d));
    return m;
}


Canvas2D::Canvas2D(QWidget* parent)
    : QOpenGLWidget(parent)
{
    setAutoFillBackground(false);
}

Canvas2D::~Canvas2D() {
    makeCurrent();
    if (program_) {
        glDeleteProgram(program_);
        program_ = 0;
    }
    doneCurrent();
}


void Canvas2D::setPolygonA(const QVector<QVector<QPointF>>& loops) {
    polyA_ = loops;
    recomputeViewRange();
    update();
}

void Canvas2D::clearPolygonA() {
    polyA_.clear();
    recomputeViewRange();
    update();
}

void Canvas2D::setPolygonB(const QVector<QVector<QPointF>>& loops) {
    polyB_ = loops;
    recomputeViewRange();
    update();
}

void Canvas2D::clearPolygonB() {
    polyB_.clear();
    recomputeViewRange();
    update();
}

void Canvas2D::setResultSegments(const QVector<QVector<QPointF>>& segs) {
    polyRes_ = segs;
    recomputeViewRange();
    update();
}

void Canvas2D::clearResultSegments() {
    polyRes_.clear();
    recomputeViewRange();
    update();
}

void Canvas2D::clearAll() {
    polyA_.clear();
    polyB_.clear();
    polyRes_.clear();
    recomputeViewRange();
    update();
}


bool Canvas2D::computeLoopsBBox(const QVector<QVector<QPointF>>& loops,
                                QRectF& outBBox)
{
    bool first = true;
    double minx=0, miny=0, maxx=0, maxy=0;

    for (const auto& loop : loops) {
        for (const auto& p : loop) {
            if (first) {
                minx = maxx = p.x();
                miny = maxy = p.y();
                first = false;
            } else {
                if (p.x() < minx) minx = p.x();
                if (p.x() > maxx) maxx = p.x();
                if (p.y() < miny) miny = p.y();
                if (p.y() > maxy) maxy = p.y();
            }
        }
    }

    if (first) {
        return false;
    }

    outBBox = QRectF(QPointF(minx, miny),
                     QPointF(maxx, maxy));
    return true;
}

void Canvas2D::recomputeViewRange() {
    QRectF boxA, boxB, boxR;
    bool hasA = computeLoopsBBox(polyA_,  boxA);
    bool hasB = computeLoopsBBox(polyB_,  boxB);
    bool hasR = computeLoopsBBox(polyRes_, boxR);

    if (!hasA && !hasB && !hasR) {
        halfExtent_ = 2.0;
        return;
    }

    auto mergeMin = [](double a, double b){ return std::min(a,b); };
    auto mergeMax = [](double a, double b){ return std::max(a,b); };

    bool first = true;
    double minx=0,miny=0,maxx=0,maxy=0;

    auto consider = [&](const QRectF& bx){
        if (first) {
            minx = bx.left();
            miny = bx.top();
            maxx = bx.right();
            maxy = bx.bottom();
            first = false;
        } else {
            minx = mergeMin(minx, bx.left());
            miny = mergeMin(miny, bx.top());
            maxx = mergeMax(maxx, bx.right());
            maxy = mergeMax(maxy, bx.bottom());
        }
    };

    if (hasA) consider(boxA);
    if (hasB) consider(boxB);
    if (hasR) consider(boxR);

    auto absmax4 = [](double a,double b,double c,double d){
        double m = std::fabs(a);
        m = std::max(m, std::fabs(b));
        m = std::max(m, std::fabs(c));
        m = std::max(m, std::fabs(d));
        return m;
    };

    double maxAbs = absmax4(minx, maxx, miny, maxy);

    double padding   = 1.2;
    double minHalf   = 2.0;
    double candidate = std::max(minHalf, maxAbs * padding);

    halfExtent_ = candidate;
}


void Canvas2D::initializeGL() {
    initializeOpenGLFunctions();

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    const char* vsSrc = R"(
        #version 330 core
        layout(location = 0) in vec2 a_pos;
        uniform mat4 u_mvp;
        void main() {
            gl_Position = u_mvp * vec4(a_pos, 0.0, 1.0);
        }
    )";

    const char* fsSrc = R"(
        #version 330 core
        uniform vec4 u_color;
        out vec4 FragColor;
        void main() {
            FragColor = u_color;
        }
    )";

    GLuint vs = compileShader(GL_VERTEX_SHADER, vsSrc);
    GLuint fs = compileShader(GL_FRAGMENT_SHADER, fsSrc);
    program_  = linkProgram(vs, fs);
    glDeleteShader(vs);
    glDeleteShader(fs);

    glUseProgram(program_);
    loc_mvp_   = glGetUniformLocation(program_, "u_mvp");
    loc_color_ = glGetUniformLocation(program_, "u_color");
    loc_pos_   = 0; // layout(location=0)

    recomputeViewRange();
    updateViewportAndMVP(width(), height());
}

void Canvas2D::resizeGL(int w, int h) {
    updateViewportAndMVP(w, h);
}

void Canvas2D::paintGL() {
    updateViewportAndMVP(width(), height());

    const float dpr = float(devicePixelRatioF());
    const int fbW = int(width()  * dpr);
    const int fbH = int(height() * dpr);
    glViewport(0, 0, fbW, fbH);

    glClearColor(190.0f/255.0f,
                 190.0f/255.0f,
                 190.0f/255.0f,
                 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);

    glUseProgram(program_);
    glUniformMatrix4fv(loc_mvp_, 1, GL_FALSE, mvp_.constData());

    glLineWidth(axisWidth_);
    drawGridAndAxes();

    glLineWidth(polyWidth_);

    const QVector4D red   (1.0f,   0.0f,   0.0f,   1.0f); // A
    const QVector4D green (0.0f,   0.63f,  0.0f,   1.0f); // B
    const QVector4D yellow(1.0f,   1.0f,   0.0f,   1.0f); // Result

    for (const auto& loop : polyA_) {
        drawPolyline(loop, /*closed=*/true, red);
    }

    for (const auto& loop : polyB_) {
        drawPolyline(loop, /*closed=*/true, green);
    }

    for (const auto& seg : polyRes_) {
        drawPolyline(seg, /*closed=*/false, yellow);
    }

    glLineWidth(axisWidth_);
}



GLuint Canvas2D::compileShader(GLenum type, const char* src) {
    GLuint sh = glCreateShader(type);
    glShaderSource(sh, 1, &src, nullptr);
    glCompileShader(sh);

    GLint ok = 0;
    glGetShaderiv(sh, GL_COMPILE_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetShaderiv(sh, GL_INFO_LOG_LENGTH, &len);
        QByteArray log;
        log.resize(len+1);
        glGetShaderInfoLog(sh, len, nullptr, log.data());
        qWarning("[Canvas2D] Shader compile error:\n%s", log.constData());
    }
    return sh;
}

GLuint Canvas2D::linkProgram(GLuint vs, GLuint fs) {
    GLuint prog = glCreateProgram();
    glAttachShader(prog, vs);
    glAttachShader(prog, fs);
    glLinkProgram(prog);

    GLint ok = 0;
    glGetProgramiv(prog, GL_LINK_STATUS, &ok);
    if (!ok) {
        GLint len = 0;
        glGetProgramiv(prog, GL_INFO_LOG_LENGTH, &len);
        QByteArray log;
        log.resize(len+1);
        glGetProgramInfoLog(prog, len, nullptr, log.data());
        qWarning("[Canvas2D] Program link error:\n%s", log.constData());
    }
    return prog;
}

void Canvas2D::updateViewportAndMVP(int w, int h) {
    double he = halfExtent_;
    if (he < 1e-6) he = 1.0;

    double aspect = (h > 0) ? double(w) / double(h) : 1.0;

    if (aspect >= 1.0) {
        viewHalfY_ = he;
        viewHalfX_ = he * aspect;
    } else {
        viewHalfX_ = he;
        viewHalfY_ = he / aspect;
    }

    mvp_.setToIdentity();
    mvp_.ortho(
        float(-viewHalfX_), float(viewHalfX_),
        float(-viewHalfY_), float(viewHalfY_),
        -1.0f, 1.0f
        );
}

void Canvas2D::drawSegment(const QPointF& a,
                           const QPointF& b,
                           const QVector4D& rgba)
{
    GLfloat verts[4] = {
        GLfloat(a.x()), GLfloat(a.y()),
        GLfloat(b.x()), GLfloat(b.y())
    };

    glUniform4f(loc_color_, rgba.x(), rgba.y(), rgba.z(), rgba.w());

    GLuint vbo = 0;
    glGenBuffers(1, &vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(verts), verts, GL_STATIC_DRAW);

    glEnableVertexAttribArray(GLuint(loc_pos_));
    glVertexAttribPointer(GLuint(loc_pos_), 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glDrawArrays(GL_LINES, 0, 2);

    glDisableVertexAttribArray(GLuint(loc_pos_));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &vbo);
}

void Canvas2D::drawPolyline(const QVector<QPointF>& pts,
                            bool closed,
                            const QVector4D& rgba)
{
    if (pts.size() < 2) return;

    GLenum mode = closed ? GL_LINE_LOOP : GL_LINE_STRIP;

    QVector<GLfloat> buf;
    buf.reserve(pts.size() * 2);
    for (const QPointF& p : pts) {
        buf.push_back(GLfloat(p.x()));
        buf.push_back(GLfloat(p.y()));
    }

    glUniform4f(loc_color_, rgba.x(), rgba.y(), rgba.z(), rgba.w());

    GLuint vbo = 0;
    glGenBuffers(1, &vbo);

    glBindBuffer(GL_ARRAY_BUFFER, vbo);
    glBufferData(GL_ARRAY_BUFFER,
                 buf.size() * sizeof(GLfloat),
                 buf.constData(),
                 GL_STATIC_DRAW);

    glEnableVertexAttribArray(GLuint(loc_pos_));
    glVertexAttribPointer(GLuint(loc_pos_), 2, GL_FLOAT, GL_FALSE, 0, (void*)0);

    glDrawArrays(mode, 0, GLsizei(pts.size()));

    glDisableVertexAttribArray(GLuint(loc_pos_));
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glDeleteBuffers(1, &vbo);
}

void Canvas2D::drawGridAndAxes() {
    const QVector4D gridColor(220.0f/255.0f,
                              220.0f/255.0f,
                              220.0f/255.0f,
                              1.0f);
    const QVector4D axisColor(0.0f, 0.0f, 0.0f, 1.0f);

    const double hx = viewHalfX_;
    const double hy = viewHalfY_;

    const int minTickX = int(std::floor(-hx));
    const int maxTickX = int(std::ceil( hx));
    const int minTickY = int(std::floor(-hy));
    const int maxTickY = int(std::ceil( hy));

    const int spanX = maxTickX - minTickX + 1;
    const int spanY = maxTickY - minTickY + 1;
    const bool tooDense = (spanX > 200 || spanY > 200);

    if (tooDense) {
        drawSegment(QPointF(-hx, 0.0), QPointF(hx, 0.0), axisColor);
        drawSegment(QPointF(0.0,-hy), QPointF(0.0, hy), axisColor);

        const double tickLen = 0.07;
        int tickLimit = 10;
        for (int i = -tickLimit; i <= tickLimit; ++i) {
            double t = double(i);
            drawSegment(QPointF(t, -tickLen),
                        QPointF(t,  tickLen),
                        axisColor);
            drawSegment(QPointF(-tickLen, t),
                        QPointF( tickLen, t),
                        axisColor);
        }
        return;
    }

    for (int gx = minTickX; gx <= maxTickX; ++gx) {
        drawSegment(QPointF(gx, -hy),
                    QPointF(gx,  hy),
                    gridColor);
    }
    for (int gy = minTickY; gy <= maxTickY; ++gy) {
        drawSegment(QPointF(-hx, gy),
                    QPointF( hx, gy),
                    gridColor);
    }

    drawSegment(QPointF(-hx, 0.0),
                QPointF( hx, 0.0),
                axisColor);
    drawSegment(QPointF(0.0,-hy),
                QPointF(0.0, hy),
                axisColor);

    const double tickLen = 0.07;
    for (int gx = minTickX; gx <= maxTickX; ++gx) {
        drawSegment(QPointF(gx, -tickLen),
                    QPointF(gx,  tickLen),
                    axisColor);
    }
    for (int gy = minTickY; gy <= maxTickY; ++gy) {
        drawSegment(QPointF(-tickLen, gy),
                    QPointF( tickLen, gy),
                    axisColor);
    }
}
