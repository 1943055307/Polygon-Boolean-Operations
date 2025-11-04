#include "mainwindow.h"
#include "Canvas2D.h"

#include <QSplitter>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QFrame>
#include <QStandardPaths>
#include <QFileDialog>
#include <QStatusBar>
#include <QFileInfo>
#include <QtGlobal>
#include <QDebug>

void MainWindow::initSplitter(QWidget* left, QWidget* right) {
    windowSplitter = new QSplitter(Qt::Horizontal, this);
    windowSplitter->setChildrenCollapsible(false);
    windowSplitter->addWidget(left);
    windowSplitter->addWidget(right);

    const double w = double(this->width());
    const double widthRate = 0.5;
    const double leftRate  = widthRate / (widthRate + 1.0);

    int initialLeftWidth  = int(leftRate * w);
    int initialRightWidth = int(w - initialLeftWidth);
    if (initialLeftWidth  <= 0) initialLeftWidth  = 200;
    if (initialRightWidth <= 0) initialRightWidth = 400;

    const int handleWidth = qMax(1, int(w * 0.01));
    windowSplitter->setHandleWidth(handleWidth);
    windowSplitter->setStyleSheet("QSplitter::handle { background:#FFFACD; }");

    windowSplitter->setSizes({initialLeftWidth, initialRightWidth});

    windowSplitter->setStretchFactor(0, 1);
    windowSplitter->setStretchFactor(1, int(1.0 / widthRate));

    setCentralWidget(windowSplitter);
}

void MainWindow::initPane(QWidget* left, QWidget* right) {
    left->setObjectName("leftPane");
    left->setStyleSheet("#leftPane { background: #2F4F4F; }");

    right->setObjectName("rightPane");
}

void MainWindow::initLeftPanelUI(QWidget* left) {
    auto *rootLayout = qobject_cast<QVBoxLayout*>(left->layout());
    if (!rootLayout) {
        rootLayout = new QVBoxLayout(left);
        rootLayout->setContentsMargins(8, 8, 8, 8);
        rootLayout->setSpacing(6);
        left->setLayout(rootLayout);
    }

    static const char* kBtnStyle =
        "QPushButton {"
        "   background: #CDC8B1;"
        "   color: #000000;"
        "   border: 3px solid #000000;"
        "   border-radius: 3px;"
        "   font-size: 16px;"
        "   font-weight: bold;"
        "   font-family: 'Courier New';"
        "}"
        "QPushButton:hover {"
        "   background: #8B8878;"
        "}";

    auto styleBtn = [&](QPushButton* b) {
        b->setStyleSheet(kBtnStyle);
        b->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Preferred);
    };

    auto makeRowTwoButtons = [&](const QString& textL,
                                 const QString& textR,
                                 void (MainWindow::*slotL)(),
                                 void (MainWindow::*slotR)()) -> QWidget*
    {
        QWidget *row = new QWidget(left);
        row->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

        auto *hl = new QHBoxLayout(row);
        hl->setContentsMargins(0,0,0,0);
        hl->setSpacing(0);

        QPushButton *btnL = new QPushButton(textL, row);
        QPushButton *btnR = new QPushButton(textR, row);
        styleBtn(btnL);
        styleBtn(btnR);

        hl->addStretch(2);
        hl->addWidget(btnL, 9);
        hl->addStretch(1);
        hl->addWidget(btnR, 6);
        hl->addStretch(2);

        connect(btnL, &QPushButton::clicked, this, slotL);
        connect(btnR, &QPushButton::clicked, this, slotR);

        return row;
    };

    auto makeRowOneButton = [&](const QString& text,
                                void (MainWindow::*slot)()) -> QWidget*
    {
        QWidget *row = new QWidget(left);
        row->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

        auto *hl = new QHBoxLayout(row);
        hl->setContentsMargins(0,0,0,0);
        hl->setSpacing(0);

        QPushButton *btn = new QPushButton(text, row);
        styleBtn(btn);

        hl->addStretch(1);
        hl->addWidget(btn, 8);
        hl->addStretch(1);

        connect(btn, &QPushButton::clicked, this, slot);

        return row;
    };

    auto makeRowSubButtons = [&]() -> QWidget*
    {
        QWidget *row = new QWidget(left);
        row->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Expanding);

        auto *hl = new QHBoxLayout(row);
        hl->setContentsMargins(0,0,0,0);
        hl->setSpacing(0);

        QPushButton *btnAB = new QPushButton(tr("Subtraction (A - B)"), row);
        QPushButton *btnBA = new QPushButton(tr("Subtraction (B - A)"), row);
        styleBtn(btnAB);
        styleBtn(btnBA);

        hl->addStretch(4);
        hl->addWidget(btnAB, 15);
        hl->addStretch(2);
        hl->addWidget(btnBA, 15);
        hl->addStretch(4);

        connect(btnAB, &QPushButton::clicked, this, &MainWindow::onSubtractionABClicked);
        connect(btnBA, &QPushButton::clicked, this, &MainWindow::onSubtractionBAClicked);

        return row;
    };

    QWidget* rowA = makeRowTwoButtons(
        tr("Read Polygon A"), tr("Clear A"),
        &MainWindow::onReadPolygonA,
        &MainWindow::onClearPolygonA
        );

    QWidget* rowB = makeRowTwoButtons(
        tr("Read Polygon B"), tr("Clear B"),
        &MainWindow::onReadPolygonB,
        &MainWindow::onClearPolygonB
        );

    QWidget* rowClearAll = makeRowOneButton(
        tr("Clear All Polygons"),
        &MainWindow::onClearAllPolygons
        );

    QFrame *sep = new QFrame(left);
    sep->setFrameShape(QFrame::HLine);
    sep->setFrameShadow(QFrame::Sunken);
    sep->setFixedHeight(2);
    sep->setStyleSheet("background-color:#CCCCCC;");

    QWidget* rowI = makeRowOneButton(
        tr("Intersection"),
        &MainWindow::onIntersectionClicked
        );

    QWidget* rowAd = makeRowOneButton(
        tr("Addition"),
        &MainWindow::onAdditionClicked
        );

    QWidget* rowS = makeRowSubButtons();

    QWidget* rowR = makeRowOneButton(
        tr("Reset"),
        &MainWindow::onResetClicked
        );

    rootLayout->addWidget(rowA,        1);
    rootLayout->addWidget(rowB,        1);
    rootLayout->addWidget(rowClearAll, 1);

    rootLayout->addWidget(sep);

    rootLayout->addWidget(rowAd,        1);
    rootLayout->addWidget(rowI,        1);
    rootLayout->addWidget(rowS,        1);
    rootLayout->addWidget(rowR,        1);
}

void MainWindow::onReadPolygonA() {
    const QString startDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString filter   = tr("All Files (*);;Text (*.txt);;JSON (*.json);;DXF (*.dxf)");
    const QString path     = QFileDialog::getOpenFileName(this, tr("Read Polygon A"), startDir, filter);
    if (path.isEmpty()) return;

    currentFilePathA = path;
    qInfo().noquote() << "[UI] Polygon A loaded:" << path;
    statusBar()->showMessage(tr("Polygon A loaded: %1").arg(QFileInfo(path).fileName()), 3000);

    emit polygonASelected(path);
}

void MainWindow::onReadPolygonB() {
    const QString startDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation);
    const QString filter   = tr("All Files (*);;Text (*.txt);;JSON (*.json);;DXF (*.dxf)");
    const QString path     = QFileDialog::getOpenFileName(this, tr("Read Polygon B"), startDir, filter);
    if (path.isEmpty()) return;

    currentFilePathB = path;
    qInfo().noquote() << "[UI] Polygon B loaded:" << path;
    statusBar()->showMessage(tr("Polygon B loaded: %1").arg(QFileInfo(path).fileName()), 3000);

    emit polygonBSelected(path);
}

void MainWindow::setPolygonAVisual(const QVector<QVector<QPointF>>& loops) {
    if (rightPart) {
        rightPart->setPolygonA(loops);
    }
}

void MainWindow::setPolygonBVisual(const QVector<QVector<QPointF>>& loops) {
    if (rightPart) {
        rightPart->setPolygonB(loops);
    }
}

void MainWindow::clearPolygonAVisual() {
    if (rightPart) {
        rightPart->clearPolygonA();
    }
}

void MainWindow::clearPolygonBVisual() {
    if (rightPart) {
        rightPart->clearPolygonB();
    }
}

void MainWindow::clearAllPolygonsVisual() {
    if (rightPart) {
        rightPart->clearAll();
    }
}

void MainWindow::setCanvasPolygons(const QVector<QVector<QPointF>>& loops) {
    if (rightPart) {
        rightPart->setResultSegments(loops);
    }
}

void MainWindow::onClearPolygonA() {
    currentFilePathA.clear();
    qInfo().noquote() << "[UI] Polygon A cleared";
    statusBar()->showMessage(tr("Polygon A cleared"), 2000);

    clearPolygonAVisual();
    emit polygonACleared();
}

void MainWindow::onClearPolygonB() {
    currentFilePathB.clear();
    qInfo().noquote() << "[UI] Polygon B cleared";
    statusBar()->showMessage(tr("Polygon B cleared"), 2000);

    clearPolygonBVisual();
    emit polygonBCleared();
}

void MainWindow::onClearAllPolygons() {
    currentFilePathA.clear();
    currentFilePathB.clear();
    qInfo().noquote() << "[UI] All polygons cleared";
    statusBar()->showMessage(tr("All polygons cleared"), 2000);

    clearAllPolygonsVisual();
    emit allPolygonsCleared();
}

void MainWindow::onIntersectionClicked() {
    qInfo().noquote() << "[UI] Intersection requested";
    statusBar()->showMessage(tr("Intersection() requested"), 2000);
    emit requestIntersection();
}

void MainWindow::onAdditionClicked() {
    qInfo().noquote() << "[UI] Addition requested";
    statusBar()->showMessage(tr("Addition() requested"), 2000);
    emit requestAddition();
}

void MainWindow::onSubtractionABClicked() {
    qInfo().noquote() << "[UI] Subtraction (A-B) requested";
    statusBar()->showMessage(tr("Subtraction(A-B) requested"), 2000);
    emit requestSubtractionAB();
}

void MainWindow::onSubtractionBAClicked() {
    qInfo().noquote() << "[UI] Subtraction (B-A) requested";
    statusBar()->showMessage(tr("Subtraction(B-A) requested"), 2000);
    emit requestSubtractionBA();
}

void MainWindow::onResetClicked() {
    if (rightPart) {
        rightPart->clearResultSegments();
    }

    if (statusBar()) {
        statusBar()->showMessage(tr("Reset: cleared result overlay"), 2000);
    }

    emit requestReset();
}


void MainWindow::showWindow(int windowWidth, int windowHeight, QPoint windowTopLeft) {
    resize(windowWidth, windowHeight);
    move(windowTopLeft);
    show();
}


MainWindow::MainWindow(int windowWidth, int windowHeight, QPoint windowTopLeft,
                       QWidget* parent)
    : QMainWindow(parent)
{
    resize(windowWidth, windowHeight);
    move(windowTopLeft);
    windowSplitter = nullptr;
    leftPart  = new QWidget(this);
    rightPart = new Canvas2D(this);
    auto *leftLayout = new QVBoxLayout(leftPart);
    leftLayout->setContentsMargins(8, 8, 8, 8);
    leftLayout->setSpacing(6);
    leftPart->setLayout(leftLayout);
    initPane(leftPart, rightPart);
    initSplitter(leftPart, rightPart);
    initLeftPanelUI(leftPart);
}

MainWindow::~MainWindow() {
}
