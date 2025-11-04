#pragma once
#include <QMainWindow>
#include <QPoint>
#include <QVector>
#include <QPointF>

class QSplitter;
class QWidget;
class Canvas2D;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    MainWindow(int windowWidth, int windowHeight, QPoint windowTopLeft,
               QWidget* parent = nullptr);

    ~MainWindow() override;

    void showWindow(int windowWidth, int windowHeight, QPoint windowTopLeft);

    void setPolygonAVisual(const QVector<QVector<QPointF>>& loops);
    void setPolygonBVisual(const QVector<QVector<QPointF>>& loops);
    void setCanvasPolygons(const QVector<QVector<QPointF>>& loops);
    void clearPolygonAVisual();
    void clearPolygonBVisual();
    void clearAllPolygonsVisual();

signals:
    void polygonASelected(const QString& path);
    void polygonBSelected(const QString& path);

    void polygonACleared();
    void polygonBCleared();
    void allPolygonsCleared();

    void requestAddition();
    void requestIntersection();
    void requestSubtractionAB();
    void requestSubtractionBA();
    void requestReset();

private slots:
    void onReadPolygonA();
    void onClearPolygonA();
    void onReadPolygonB();
    void onClearPolygonB();
    void onClearAllPolygons();

    void onAdditionClicked();
    void onIntersectionClicked();
    void onSubtractionABClicked();
    void onSubtractionBAClicked();
    void onResetClicked();

private:
    void initSplitter(QWidget* left, QWidget* right);
    void initPane(QWidget* left, QWidget* right);
    void initLeftPanelUI(QWidget* left);

private:
    QSplitter* windowSplitter = nullptr;
    QWidget*   leftPart       = nullptr;
    Canvas2D*  rightPart      = nullptr;

    QString currentFilePathA;
    QString currentFilePathB;
};
