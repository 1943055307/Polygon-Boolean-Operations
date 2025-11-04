#include <QApplication>
#include <QScreen>
#include <QDebug>
#include "mainwindow.h"
#include "inputpolygon.h"
#include "booleanops.h"

int windowWidth;
int windowHeight;
QPoint windowTopLeft;

void initParameters() {
    windowWidth = 0;
    windowHeight = 0;
    windowTopLeft = QPoint(0, 0);
}

void initWindow() {
    QScreen* availableScreen = QGuiApplication::primaryScreen();
    QRect availableArea = availableScreen->availableGeometry();
    const double windowWidthRatio  = 0.8;
    const double windowHeightRatio = 0.8;
    windowWidth  = int(windowWidthRatio  * availableArea.width());
    windowHeight = int(windowHeightRatio * availableArea.height());
    windowTopLeft = availableArea.center() - QPoint(windowWidth / 2, windowHeight / 2);
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    InputPolygon polygonA;
    InputPolygon polygonB;

    initParameters();
    initWindow();

    MainWindow mainWin(windowWidth, windowHeight, windowTopLeft);
    mainWin.showWindow(windowWidth, windowHeight, windowTopLeft);

    QObject::connect(&mainWin, &MainWindow::polygonASelected,
                     [&](const QString& path){
                         qInfo().noquote() << "[main] load A from:" << path;
                         QString err;
                         if (!polygonA.loadData(path, &err)) {
                             qWarning().noquote() << "[main] Failed to load A:" << err;
                             return;
                         }
                         qInfo() << "[main] polygonA outer points:"
                                 << polygonA.outerPointCount()
                                 << "holes:" << polygonA.holeLoops().size();
                         QVector<QVector<QPointF>> loopsA;
                         loopsA.push_back(polygonA.outerLoop());
                         for (const auto& h : polygonA.holeLoops()) {
                             loopsA.push_back(h);
                         }
                         mainWin.setPolygonAVisual(loopsA);
                     });

    QObject::connect(&mainWin, &MainWindow::polygonBSelected,
                     [&](const QString& path){
                         qInfo().noquote() << "[main] load B from:" << path;
                         QString err;
                         if (!polygonB.loadData(path, &err)) {
                             qWarning().noquote() << "[main] Failed to load B:" << err;
                             return;
                         }
                         qInfo() << "[main] polygonB outer points:"
                                 << polygonB.outerPointCount()
                                 << "holes:" << polygonB.holeLoops().size();
                         QVector<QVector<QPointF>> loopsB;
                         loopsB.push_back(polygonB.outerLoop());
                         for (const auto& h : polygonB.holeLoops()) {
                             loopsB.push_back(h);
                         }
                         mainWin.setPolygonBVisual(loopsB);
                     });

    QObject::connect(&mainWin, &MainWindow::polygonACleared,
                     [&](){
                         qInfo().noquote() << "[main] polygonA cleared";
                         polygonA.clearPolygon();
                         mainWin.clearPolygonAVisual();
                     });

    QObject::connect(&mainWin, &MainWindow::polygonBCleared,
                     [&](){
                         qInfo().noquote() << "[main] polygonB cleared";
                         polygonB.clearPolygon();
                         mainWin.clearPolygonBVisual();
                     });

    QObject::connect(&mainWin, &MainWindow::allPolygonsCleared,
                     [&](){
                         qInfo().noquote() << "[main] all polygons cleared";
                         polygonA.clearPolygon();
                         polygonB.clearPolygon();
                         mainWin.clearAllPolygonsVisual();
                     });

    QObject::connect(&mainWin, &MainWindow::requestAddition,
                     [&](){
                         if (polygonA.outerLoop().isEmpty() || polygonB.outerLoop().isEmpty()) {
                             qWarning().noquote() << "Need Two Polygons";
                             return;
                         }
                         qInfo().noquote() << "[main] Addition() now running";
                         auto ctx = Boolean2D::prepare(polygonA, polygonB, 1e-3, 1e-9);
                         auto resSegments = Boolean2D::computeAdditionSegments(ctx, polygonA, polygonB);
                         mainWin.setCanvasPolygons(resSegments);
                     });

    QObject::connect(&mainWin, &MainWindow::requestIntersection,
                     [&](){
                         if (polygonA.outerLoop().isEmpty() || polygonB.outerLoop().isEmpty()) {
                             qWarning().noquote() << "Need Two Polygons";
                             return;
                         }
                         qInfo().noquote() << "[main] Intersection() now running";
                         auto ctx = Boolean2D::prepare(polygonA, polygonB, 1e-3, 1e-9);
                         auto resSegments = Boolean2D::computeIntersectionSegments(ctx, polygonA, polygonB);
                         mainWin.setCanvasPolygons(resSegments);
                     });

    QObject::connect(&mainWin, &MainWindow::requestSubtractionAB,
                     [&](){
                         if (polygonA.outerLoop().isEmpty() || polygonB.outerLoop().isEmpty()) {
                             qWarning().noquote() << "Need Two Polygons";
                             return;
                         }
                         qInfo().noquote() << "[main] Subtraction(A-B) now running";
                         auto ctx = Boolean2D::prepare(polygonA, polygonB, 1e-3, 1e-9);
                         auto resSegments = Boolean2D::computeSubtractionABSegments(ctx, polygonA, polygonB);
                         mainWin.setCanvasPolygons(resSegments);
                     });

    QObject::connect(&mainWin, &MainWindow::requestSubtractionBA,
                     [&](){
                         if (polygonA.outerLoop().isEmpty() || polygonB.outerLoop().isEmpty()) {
                             qWarning().noquote() << "Need Two Polygons";
                             return;
                         }
                         qInfo().noquote() << "[main] Subtraction(B-A) now running";
                         auto ctx = Boolean2D::prepare(polygonA, polygonB, 1e-3, 1e-9);
                         auto resSegments = Boolean2D::computeSubtractionBASegments(ctx, polygonA, polygonB);
                         mainWin.setCanvasPolygons(resSegments);
                     });

    QObject::connect(&mainWin, &MainWindow::requestReset,
                     [&](){
                         qInfo().noquote() << "[main] Reset() should run here";
                     });

    return app.exec();
}
