#include "mainwindow.h"
#include "glwidget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    m_glWidget = new GLWidget(this);
    setCentralWidget(m_glWidget);
    resize(1440, 900);
    setWindowTitle("BlackHole Kerr Geodesics Lab");
}

MainWindow::~MainWindow() {}
