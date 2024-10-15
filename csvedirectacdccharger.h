/*********************************************************************
 * Copyright EoF Software Labs. All Rights Reserved.
 * Copyright EoF Software Labs Authors.
 * Written by B. Eschrich (bjoern.eschrich@gmail.com)
 * SPDX-License-Identifier: MIT License
 **********************************************************************/
#pragma once
#include <QMap>
#include <QObject>
#include <QSerialPort>
#include <QSharedData>
#include <csvedirect.h>

class CSVeDirectAcDcCharger: public QObject
{
    Q_OBJECT

public:
    class TVedConfig: public QSharedData
    {
    public:
        QString m_portName = "";
        QSerialPort::BaudRate m_baudRate = QSerialPort::BaudRate::Baud19200;
        QSerialPort::DataBits m_dataBits = QSerialPort::DataBits::Data8;
        QSerialPort::StopBits m_stopBits = QSerialPort::StopBits::OneStop;
        QSerialPort::Parity m_parity = QSerialPort::Parity::NoParity;
        QSerialPort::FlowControl m_flow = QSerialPort::FlowControl::NoFlowControl;
        bool m_enabled = false;

        explicit TVedConfig()
            : QSharedData()
        {
        }

        TVedConfig(const TVedConfig& other)
            : QSharedData()
        {
            if (&other == this) {
                return;
            }
            ref = other.ref;
            m_portName = other.m_portName;
            m_baudRate = other.m_baudRate;
            m_dataBits = other.m_dataBits;
            m_stopBits = other.m_stopBits;
            m_parity = other.m_parity;
            m_flow = other.m_flow;
        }

        inline const TVedConfig& operator=(const TVedConfig& other)
        {
            if (&other == this) {
                return (*this);
            }
            m_portName = other.m_portName;
            m_baudRate = other.m_baudRate;
            m_dataBits = other.m_dataBits;
            m_stopBits = other.m_stopBits;
            m_parity = other.m_parity;
            m_flow = other.m_flow;
            return (*this);
        }
    };

    typedef struct {
        uint m_counter;
    } TStateData;

    explicit CSVeDirectAcDcCharger(QObject* parent = nullptr);

    ~CSVeDirectAcDcCharger();

    bool startVEDirect();
    void stopVEDirect();

    void setPowerSupply();
    void setBatteryCharger();
    void sendGetRegister(quint16 regid);
    void sendSetRegister(quint16 regid, const QString& value);
    void sendSetRegister(quint16 regid, quint8 value);
    void sendSetRegister(quint16 regid, quint16 value);
    void sendSetRegister(quint16 regid, quint32 value);
    void sendPing();

    const TVedConfig& configOut() const;
    const TVedConfig& configIn() const;

    const QMap<quint16, QPair<float, QVariant>>& values() const;

    void setConfigOut(const CSVeDirectAcDcCharger::TVedConfig& newConfigOut);
    void setConfigIn(const CSVeDirectAcDcCharger::TVedConfig& newConfigIn);

signals:
    void dataChanged(uint regid, const QPair<float, QVariant>&);

protected:
    bool open();
    void close();
    bool isOpen() const;

private slots:
    void onVedEchoInbound(const char c);

private:
    QSerialPort m_portCharger;
    TVedConfig m_configCharger;

    QSerialPort m_portCerbo;
    TVedConfig m_configCerbo;

    CSVeParser m_parserCharger;
    CSVeParser m_parserCerbo;

    QMap<quint16, QPair<float, QVariant>> m_values;

    TStateData m_stateData;
    QList<CSVEDirect::ved_t> m_queue;

private:
    inline void setupDefaults();
    inline void connectEvents();
    inline bool openInputPort();
    inline bool openOutputPort();
    inline void restartPorts();
    inline void setRegister(quint16 regid, float scale, const QVariant& value);
    inline void veHandleInput(CSVeParser* parser, QSerialPort* input, QSerialPort* output);
    inline void veChargerSetTextField(const QString& field, const QByteArray& value);
    inline void veSendCommandQueue();
    inline void veSendToCharger(CSVEDirect::ved_t* ve_out);
    inline void veSendToCerboGx(CSVEDirect::ved_t* ve_out);
    inline void veSendFrameTo(CSVEDirect::ved_t* ved, QSerialPort* port);
    inline bool veDoSetData(const CSVeParser::TVeHexFrame& frame);
    inline bool veUpdateData(const CSVeParser::TVeHexFrame& frame);
};
Q_DECLARE_METATYPE(CSVeDirectAcDcCharger::TVedConfig)
