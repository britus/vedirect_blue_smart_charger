#include "ui_mainwindow.h"
#include <QClipboard>
#include <QDataStream>
#include <QDebug>
#include <QDir>
#include <QFile>
#include <QFileDialog>
#include <QFileInfoList>
#include <QMessageBox>
#include <QThread>
#include <mainwindow.h>

Q_DECLARE_METATYPE(QSerialPort::SerialPortError)
Q_DECLARE_METATYPE(QSerialPort::StopBits)
Q_DECLARE_METATYPE(QSerialPort::DataBits)
Q_DECLARE_METATYPE(QSerialPort::BaudRate)
Q_DECLARE_METATYPE(QSerialPort::FlowControl)
Q_DECLARE_METATYPE(QSerialPort::Parity)
Q_DECLARE_METATYPE(QSerialPortInfo)

#ifndef BIT
#define BIT(x) (1 << (x))
#endif

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_chr(this)
    , m_config()
{
    ui->setupUi(this);

    ui->btnClose->setEnabled(false);
    ui->btnOpen->setEnabled(true);

    ui->tableView->setModel(&m_model);

    qRegisterMetaType<QSerialPort::SerialPortError>();
    qRegisterMetaType<QSerialPort::FlowControl>();
    qRegisterMetaType<QSerialPort::BaudRate>();
    qRegisterMetaType<QSerialPort::DataBits>();
    qRegisterMetaType<QSerialPort::StopBits>();
    qRegisterMetaType<QSerialPort::Parity>();

    setupDefaults();
    initializeUI();
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::onDataChanged(uint regid, const QPair<float, QVariant>& kv)
{
    m_model.beginUpdate();
    m_model.updateRegister(regid, kv);
    m_model.endUpdate();
}

void MainWindow::on_tableView_doubleClicked(const QModelIndex& index)
{
    quint16 regid = m_model.toRegId(index);
    if (regid == 0xffff) {
        return;
    }
    ui->edRegister->setValue(regid);

    QClipboard* clipboard = QGuiApplication::clipboard();
    clipboard->setText(tr("0x%1").arg(regid, 4, 16, QChar('0')));
}

void MainWindow::on_btnOpen_clicked()
{
    m_chr.setConfigIn(m_config);
    m_chr.startVEDirect();
}

void MainWindow::on_btnClose_clicked()
{
    m_chr.stopVEDirect();
}

void MainWindow::on_btnWriteReg_clicked()
{
    m_chr.sendSetRegister(ui->edRegister->value(), ui->edValue->value());
}

inline void MainWindow::setupDefaults()
{
    m_config = m_chr.configIn();

    /* device queries */
    m_chr.sendPing();
    m_chr.sendGetRegister(0x0104);
    m_chr.sendGetRegister(0xEDDE);
    m_chr.sendGetRegister(0x0140);
    m_chr.sendGetRegister(0x0143);
    m_chr.sendGetRegister(0x200F);
    m_chr.sendGetRegister(0x010C);
    m_chr.sendGetRegister(0x1070);
    m_chr.sendGetRegister(0x2001);
    m_chr.setBatteryCharger();
    m_chr.sendSetRegister(0xEDF1, 4);
}

inline void MainWindow::initializeUI()
{
    const QList<QSerialPortInfo> ports = QSerialPortInfo::availablePorts();

    ui->cbxComPort->clear();
    ui->cbxComPort->setCurrentIndex(-1);
    for (int i = 0; i < ports.count(); i++) {
        ui->cbxComPort->addItem(ports[i].portName(), QVariant::fromValue(ports[i]));
        if (ports[i].portName().contains(m_config.m_portName)) {
            ui->cbxComPort->setCurrentIndex(i);
        }
    }
    connect(ui->cbxComPort, qOverload<int>(&QComboBox::activated), this, [this](int index) {
        QSerialPortInfo spi = ui->cbxComPort->itemData(index).value<QSerialPortInfo>();
        if (!spi.isNull()) {
            m_config.m_portName = spi.portName();
        }
    });

    typedef struct {
        QString name;
        QSerialPort::BaudRate baud;
    } TBaudRates;

    TBaudRates baudRates[8] = {
       {tr("1200"), QSerialPort::Baud1200},
       {tr("2400"), QSerialPort::Baud2400},
       {tr("4800"), QSerialPort::Baud4800},
       {tr("9600"), QSerialPort::Baud9600},
       {tr("19200"), QSerialPort::Baud19200},
       {tr("38400"), QSerialPort::Baud38400},
       {tr("57600"), QSerialPort::Baud57600},
       {tr("115200"), QSerialPort::Baud115200},
    };

    ui->cbxBaudRate->clear();
    ui->cbxBaudRate->setCurrentIndex(-1);
    for (int i = 0; i < 8; i++) {
        ui->cbxBaudRate->addItem(baudRates[i].name, QVariant::fromValue(baudRates[i].baud));
        if (m_config.m_baudRate == baudRates[i].baud) {
            ui->cbxBaudRate->setCurrentIndex(i);
        }
    }
    connect(ui->cbxBaudRate, qOverload<int>(&QComboBox::activated), this, [this](int index) {
        m_config.m_baudRate = ui->cbxBaudRate->itemData(index).value<QSerialPort::BaudRate>();
    });

    typedef struct {
        QString name;
        QSerialPort::DataBits bits;
    } TDataBits;

    TDataBits dataBits[4] = {
       {tr("5 Bits"), QSerialPort::Data5},
       {tr("6 Bits"), QSerialPort::Data6},
       {tr("7 Bits"), QSerialPort::Data7},
       {tr("8 Bits"), QSerialPort::Data8},
    };

    ui->cbxDataBits->clear();
    ui->cbxDataBits->setCurrentIndex(-1);
    for (int i = 0; i < 4; i++) {
        ui->cbxDataBits->addItem(dataBits[i].name, QVariant::fromValue(dataBits[i].bits));
        if (m_config.m_dataBits == dataBits[i].bits) {
            ui->cbxDataBits->setCurrentIndex(i);
        }
    }
    connect(ui->cbxDataBits, qOverload<int>(&QComboBox::activated), this, [this](int index) {
        m_config.m_dataBits = ui->cbxDataBits->itemData(index).value<QSerialPort::DataBits>();
    });

    typedef struct {
        QString name;
        QSerialPort::StopBits bits;
    } TStopBits;

    TStopBits stopBits[3] = {
       {tr("One Stop"), QSerialPort::OneStop},
       {tr("One and Half"), QSerialPort::OneAndHalfStop},
       {tr("Two Stop"), QSerialPort::TwoStop},
    };

    ui->cbxStopBits->clear();
    ui->cbxStopBits->setCurrentIndex(-1);
    for (int i = 0; i < 3; i++) {
        ui->cbxStopBits->addItem(stopBits[i].name, QVariant::fromValue(stopBits[i].bits));
        if (m_config.m_stopBits == stopBits[i].bits) {
            ui->cbxStopBits->setCurrentIndex(i);
        }
    }
    connect(ui->cbxStopBits, qOverload<int>(&QComboBox::activated), this, [this](int index) {
        m_config.m_stopBits = ui->cbxStopBits->itemData(index).value<QSerialPort::StopBits>();
    });

    typedef struct {
        QString name;
        QSerialPort::Parity parity;
    } TParity;

    TParity parity[5] = {
       {tr("No Parity"), QSerialPort::NoParity},
       {tr("Event Parity"), QSerialPort::EvenParity},
       {tr("Odd Parity"), QSerialPort::OddParity},
       {tr("Space Parity"), QSerialPort::SpaceParity},
       {tr("Mark Parity"), QSerialPort::MarkParity},
    };

    ui->cbxParity->clear();
    ui->cbxParity->setCurrentIndex(-1);
    for (int i = 0; i < 5; i++) {
        ui->cbxParity->addItem(parity[i].name, QVariant::fromValue(parity[i].parity));
        if (m_config.m_parity == parity[i].parity) {
            ui->cbxParity->setCurrentIndex(i);
        }
    }
    connect(ui->cbxParity, qOverload<int>(&QComboBox::activated), this, [this](int index) {
        m_config.m_parity = ui->cbxParity->itemData(index).value<QSerialPort::Parity>();
    });

    typedef struct {
        QString name;
        QSerialPort::FlowControl flowctl;
    } TFlowControl;

    TFlowControl flowctl[3] = {
       {tr("No Flow Control"), QSerialPort::NoFlowControl},
       {tr("Software Control"), QSerialPort::SoftwareControl},
       {tr("Hardware Control"), QSerialPort::HardwareControl},
    };

    ui->cbxFlowCtrl->clear();
    ui->cbxFlowCtrl->setCurrentIndex(-1);
    for (int i = 0; i < 3; i++) {
        ui->cbxFlowCtrl->addItem(flowctl[i].name, QVariant::fromValue(flowctl[i].flowctl));
        if (m_config.m_flow == flowctl[i].flowctl) {
            ui->cbxFlowCtrl->setCurrentIndex(i);
        }
    }
    connect(ui->cbxFlowCtrl, qOverload<int>(&QComboBox::activated), this, [this](int index) {
        m_config.m_flow = ui->cbxFlowCtrl->itemData(index).value<QSerialPort::FlowControl>();
    });

    connect(&m_chr, &CSVeDirectAcDcCharger::dataChanged, this, &MainWindow::onDataChanged);
}
