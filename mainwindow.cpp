#include "mainwindow.h"
#include "glwidget.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    GLWidget* glWidget = new GLWidget(this);
    setCentralWidget(glWidget);
}

MainWindow::~MainWindow() {}
