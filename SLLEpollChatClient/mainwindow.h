#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QTcpSocket>

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT
    
public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();
    
private slots:
    void handleConnectButton();
    void handleSendButton();
    void socket_Read_Data();
    void socket_Disconnected();
    
private:
    Ui::MainWindow *ui;
    QTcpSocket *m_pSocket;
};
#endif // MAINWINDOW_H
