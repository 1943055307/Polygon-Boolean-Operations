#pragma once
#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QMatrix4x4>
#include <QVector>
#include <QPointF>
#include <QRectF>

class Canvas2D : public QOpenGLWidget, protected QOpenGLFunctions
{
    Q_OBJECT
public:
    explicit Canvas2D(QWidget* parent = nullptr);
    ~Canvas2D() override;

    void setPolygonA(const QVector<QVector<QPointF>>& loops);
    void clearPolygonA();

    void setPolygonB(const QVector<QVector<QPointF>>& loops);
    void clearPolygonB();

    void setResultSegments(const QVector<QVector<QPointF>>& segs);
    void clearResultSegments();

    void clearAll();

protected:
    void initializeGL() override;
    void resizeGL(int w, int h) override;
    void paintGL() override;

private:
    GLuint program_   = 0;
    GLint  loc_mvp_   = -1;
    GLint  loc_color_ = -1;
    GLint  loc_pos_   = -1;

    float axisWidth_ = 1.0f;
    float polyWidth_ = 4.0f;

    QMatrix4x4 mvp_;

    QVector<QVector<QPointF>> polyA_;
    QVector<QVector<QPointF>> polyB_;
    QVector<QVector<QPointF>> polyRes_;

    double halfExtent_ = 2.0;
    double viewHalfX_  = 2.0;
    double viewHalfY_  = 2.0;

    void recomputeViewRange();

    static bool computeLoopsBBox(const QVector<QVector<QPointF>>& loops, QRectF& outBBox);

    GLuint compileShader(GLenum type, const char* src);
    GLuint linkProgram(GLuint vs, GLuint fs);

    void updateViewportAndMVP(int w, int h);

    void drawSegment(const QPointF& a, const QPointF& b, const QVector4D& rgba);

    void drawPolyline(const QVector<QPointF>& pts, bool closed, const QVector4D& rgba);

    void drawGridAndAxes();
};
