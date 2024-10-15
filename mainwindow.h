#pragma once
#include <QList>
#include <QMainWindow>
#include <QMap>
#include <QSerialPort>
#include <QSerialPortInfo>
#include <QTimer>
#include <cschargerdatamodel.h>
#include <csvedirect.h>
#include <csvedirectacdccharger.h>

QT_BEGIN_NAMESPACE

namespace Ui {
class MainWindow;
}

QT_END_NAMESPACE

class MainWindow: public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget* parent = nullptr);
    ~MainWindow();

private slots:
    void onDataChanged(uint regid, const QPair<float, QVariant>&);
    void on_btnOpen_clicked();
    void on_btnClose_clicked();
    void on_tableView_doubleClicked(const QModelIndex& index);

    void on_btnWriteReg_clicked();

private:
    Ui::MainWindow* ui;

    CSVeDirectAcDcCharger m_chr;
    CSVeDirectAcDcCharger::TVedConfig m_config;
    CSChargerDataModel m_model;
    QList<CSVEDirect::ved_t> m_queue;

private:
    inline void setupDefaults();
    inline void initializeUI();
};
