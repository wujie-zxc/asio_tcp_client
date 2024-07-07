#include "mainwindow.h"
#include "ui_mainwindow.h"
#include "screwtcpclient.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    client_ = std::make_shared<ScrewTcpClient>();
    client_->connect("192.168.226.131", "8891");
}

MainWindow::~MainWindow()
{
    client_->disconnect();
    delete ui;
}

