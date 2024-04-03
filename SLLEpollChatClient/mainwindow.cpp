#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    m_pSocket = new QTcpSocket(this);

    // 使用connect方式连接信号槽
    connect(ui->pushButton_Connect, &QPushButton::clicked, this, &MainWindow::handleConnectButton);
    connect(ui->pushButton_Send, &QPushButton::clicked, this, &MainWindow::handleSendButton);
    connect(m_pSocket, &QTcpSocket::readyRead, this, &MainWindow::socket_Read_Data);
    connect(m_pSocket, &QTcpSocket::disconnected, this, &MainWindow::socket_Disconnected);

    ui->pushButton_Send->setEnabled(false);
    ui->lineEdit_IP->setText("124.220.15.250");
    ui->lineEdit_Port->setText("8081");
}

MainWindow::~MainWindow()
{
    delete ui;
   \
    if (m_pSocket != nullptr)
    {
        delete m_pSocket;
        m_pSocket = nullptr;
    }
}

void MainWindow::handleConnectButton()
{
    if (ui->pushButton_Connect->text() == tr("连接"))
    {
        QString IP;
        int port;

        // 获取IP地址
        IP = ui->lineEdit_IP->text();
        // 获取端口号
        port = ui->lineEdit_Port->text().toInt();
        // 取消已有的连接
        m_pSocket->abort();
        // 连接服务器
        m_pSocket->connectToHost(IP, port);
        // 等待连接成功
        if (!m_pSocket->waitForConnected(30000))
        {
            qDebug() << "连接失败!";
            return;
        }
        qDebug() << "连接成功!";
        // 发送按键可用状态
        ui->pushButton_Send->setEnabled(true);
        // 修改按键文字
        ui->pushButton_Connect->setText("断开连接");
    }
    else
    {
        // 断开连接
        m_pSocket->disconnectFromHost();
        // 修改按键文字
        ui->pushButton_Connect->setText("连接");
        ui->pushButton_Send->setEnabled(false);
    }
}

void MainWindow::handleSendButton()
{
    QString message = ui->textEdit_Send->toPlainText();
    qDebug() << "发送: " << message;
    ui->textEdit_Send->clear();

    m_pSocket->write(message.toUtf8());
    m_pSocket->flush(); // 调用 flush()

    // 添加标签"[发送]"到接收文本框 
    ui->textEdit_Recv->append("[发送] " + message);
}

void MainWindow::socket_Read_Data()
{
    QByteArray buffer;
    //读取缓冲区数据
    buffer = m_pSocket->readAll();
    if(!buffer.isEmpty())
    {
        QString receivedData = QString::fromUtf8(buffer);
        //刷新显示，并添加标签"[接收]"
        ui->textEdit_Recv->append("[接收] " + receivedData);
    }
}

void MainWindow::socket_Disconnected()
{
    //发送按键失能
    ui->pushButton_Send->setEnabled(false);
    //修改按键文字
    ui->pushButton_Connect->setText("连接");
    qDebug() << "断开连接!";
}
