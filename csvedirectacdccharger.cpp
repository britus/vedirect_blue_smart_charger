/*********************************************************************
 * Copyright EoF Software Labs. All Rights Reserved.
 * Copyright EoF Software Labs Authors.
 * Written by B. Eschrich (bjoern.eschrich@gmail.com)
 * SPDX-License-Identifier: MIT License
 **********************************************************************/
#include <QCoreApplication>
#include <QDebug>
#include <QThread>
#include <QTimer>
#include <csvedirectacdccharger.h>

Q_DECLARE_METATYPE(QSerialPort::SerialPortError)
Q_DECLARE_METATYPE(QSerialPort::StopBits)
Q_DECLARE_METATYPE(QSerialPort::DataBits)
Q_DECLARE_METATYPE(QSerialPort::BaudRate)
Q_DECLARE_METATYPE(QSerialPort::FlowControl)
Q_DECLARE_METATYPE(QSerialPort::Parity)

#ifdef Q_OS_MACOS
#define PORT_DEV_CHR "cu.usbserial-B001EN2X"
#define PORT_DEV_CGX "cu.usbserial-B002XVOA"
#else
#define PORT_DEV_CHR "ttysVECHR"
#define PORT_DEV_CGX "ttysVECGX"
#endif
CSVeDirectAcDcCharger::CSVeDirectAcDcCharger(QObject* parent)
    : QObject {parent}
    , m_portCharger(this)
    , m_configCharger()
    , m_portCerbo(this)
    , m_configCerbo()
    , m_parserCharger(this)
    , m_stateData()
    , m_queue()
{
    setupDefaults();
    connectEvents();
}

CSVeDirectAcDcCharger::~CSVeDirectAcDcCharger()
{
}

bool CSVeDirectAcDcCharger::startVEDirect()
{
    /* ..................................................
     * Serial Port Cerbo GX MK2
     * .................................................. */

    connect(&m_portCerbo, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
        if (error != QSerialPort::NoError) {
            qDebug() << "UART-Cerbo: errorOccurred():" << error;
            if (m_portCerbo.isOpen()) {
                m_portCerbo.close();
            }
        }
    });
    connect(&m_portCerbo, &QSerialPort::dataTerminalReadyChanged, this, [](bool set) {
        qDebug() << "UART-Cerbo: dataTerminalReadyChanged" << set;
    });
    connect(&m_portCerbo, &QSerialPort::requestToSendChanged, this, [](bool set) {
        qDebug() << "UART-Cerbo: requestToSendChanged" << set;
    });
    connect(&m_portCerbo, &QSerialPort::aboutToClose, this, [this]() {
        qDebug() << "UART-Cerbo: aboutToClose" << sender();
    });
    connect(&m_portCerbo, &QSerialPort::readyRead, this, [this]() {
        /* Cerbo GX -> to -> CarIOS, Blue Smart Charger */
        veHandleInput(&m_parserCerbo, &m_portCerbo, &m_portCharger);
    });

    if (!openOutputPort()) {
        disconnect(&m_portCerbo);
        return false;
    }

    /* ..................................................
     * Serial Port AC/DC Blue Smart Charer I/O
     * .................................................. */

    connect(&m_portCharger, &QSerialPort::errorOccurred, this, [this](QSerialPort::SerialPortError error) {
        if (error != QSerialPort::NoError) {
            qDebug() << "UART-Charger: errorOccurred():" << error;
            if (m_portCerbo.isOpen()) {
                m_portCerbo.close();
            }
        }
    });
    connect(&m_portCharger, &QSerialPort::dataTerminalReadyChanged, this, [](bool set) {
        qDebug() << "UART-Charger: dataTerminalReadyChanged" << set;
    });
    connect(&m_portCharger, &QSerialPort::requestToSendChanged, this, [](bool set) {
        qDebug() << "UART-Charger: requestToSendChanged" << set;
    });
    connect(&m_portCharger, &QSerialPort::aboutToClose, this, [this]() {
        qDebug() << "UART-Charger: aboutToClose" << sender();
    });
    connect(&m_portCharger, &QSerialPort::readyRead, this, [this]() {
        /* Charger -> to -> CarIOS, Cerbo GX */
        veHandleInput(&m_parserCharger, &m_portCharger, &m_portCerbo);
    });

    if (!openInputPort()) {
        disconnect(&m_portCharger);
        disconnect(&m_portCerbo);
        m_portCerbo.close();
        return false;
    }

    return true;
}

void CSVeDirectAcDcCharger::stopVEDirect()
{
    disconnect(&m_portCharger);
    disconnect(&m_portCerbo);
    close();
}

void CSVeDirectAcDcCharger::setPowerSupply()
{
    sendSetRegister(0x0206, 1);
}

void CSVeDirectAcDcCharger::setBatteryCharger()
{
    sendSetRegister(0x0206, 0);
}

void CSVeDirectAcDcCharger::sendGetRegister(quint16 regid)
{
    CSVEDirect::ved_t ved = {};
    CSVEDirect::setCommand(&ved, VED_CMD_GET);
    CSVEDirect::setId(&ved, regid);
    CSVEDirect::setFlags(&ved, 0);
    m_queue.append(ved);
}

void CSVeDirectAcDcCharger::sendSetRegister(quint16 regid, const QString& value)
{
    CSVEDirect::ved_t ved = {};
    CSVEDirect::setCommand(&ved, VED_CMD_SET);
    CSVEDirect::setId(&ved, regid);
    CSVEDirect::setFlags(&ved, 0);
    for (int i = 0; i < value.length(); i++) {
        CSVEDirect::addU8(&ved, (quint8) value.at(i).cell());
    }
    m_queue.append(ved);
}

void CSVeDirectAcDcCharger::sendSetRegister(quint16 regid, quint16 value)
{
    CSVEDirect::ved_t ved = {};
    CSVEDirect::setCommand(&ved, VED_CMD_SET);
    CSVEDirect::setId(&ved, regid);
    CSVEDirect::setFlags(&ved, 0);
    CSVEDirect::setU8(&ved, value);
    m_queue.append(ved);
}

void CSVeDirectAcDcCharger::sendPing()
{
    CSVEDirect::ved_t ved;
    CSVEDirect::setCommand(&ved, VED_CMD_PING);
    m_queue.append(ved);
}

bool CSVeDirectAcDcCharger::open()
{
    return (openOutputPort() && openInputPort());
}

void CSVeDirectAcDcCharger::close()
{
    if (m_portCharger.isOpen()) {
        m_portCharger.flush();
        m_portCharger.close();
    }
    if (m_portCerbo.isOpen()) {
        m_portCerbo.flush();
        m_portCerbo.close();
    }
}

bool CSVeDirectAcDcCharger::isOpen() const
{
    return m_portCharger.isOpen() /*&& m_portCerbo.isOpen()*/;
}

const CSVeDirectAcDcCharger::TVedConfig& CSVeDirectAcDcCharger::configIn() const
{
    return m_configCharger;
}

void CSVeDirectAcDcCharger::setConfigIn(const TVedConfig& config)
{
    m_configCharger = config;
    if (m_portCharger.isOpen()) {
        m_portCharger.close();
    }
}

const CSVeDirectAcDcCharger::TVedConfig& CSVeDirectAcDcCharger::configOut() const
{
    return m_configCerbo;
}

void CSVeDirectAcDcCharger::setConfigOut(const TVedConfig& config)
{
    m_configCerbo = config;
    if (m_portCerbo.isOpen()) {
        m_portCerbo.close();
    }
}

void CSVeDirectAcDcCharger::onVedEchoInbound(const char c)
{
    if (!m_portCerbo.isOpen()) {
        if (m_portCharger.isOpen()) {
            m_portCharger.write(&c, 1);
            m_portCharger.waitForBytesWritten();
        }
    }
}

const QMap<quint16, QPair<float, QVariant>>& CSVeDirectAcDcCharger::values() const
{
    return m_values;
}

inline void CSVeDirectAcDcCharger::setupDefaults()
{
    /* AC/DC Charger -> CarIOS
     * Mac OSX /dev/cu.usbserial-B001EN2X */
    m_configCharger.m_portName = PORT_DEV_CHR; // "ttyS4";
    m_configCharger.m_baudRate = QSerialPort::Baud19200;
    m_configCharger.m_dataBits = QSerialPort::Data8;
    m_configCharger.m_stopBits = QSerialPort::OneStop;
    m_configCharger.m_parity = QSerialPort::NoParity;
    m_configCharger.m_flow = QSerialPort::NoFlowControl;

    /* CarIOS -> Cerbo GX
     * Mac OSX /dev/cu.usbserial-B002XVOA */
    m_configCerbo.m_portName = PORT_DEV_CGX; //"ttyUSB5";
    m_configCerbo.m_baudRate = QSerialPort::Baud19200;
    m_configCerbo.m_dataBits = QSerialPort::Data8;
    m_configCerbo.m_stopBits = QSerialPort::OneStop;
    m_configCerbo.m_parity = QSerialPort::NoParity;
    m_configCerbo.m_flow = QSerialPort::NoFlowControl;
}

inline void CSVeDirectAcDcCharger::connectEvents()
{
    /* ..................................................
     * Blue Smart Charger Protocol to CarIOS
     * .................................................. */

    // connect(&m_parserCharger, &CSVeParser::echoInbound, this, &CSVeDirectAcDcCharger::onVedEchoInbound);
    connect(&m_parserCharger, &CSVeParser::errorOccured, this, [](const QByteArray& messge) {
        qCritical() << "[VE.CHR]" << messge;
    });
    connect(&m_parserCharger, &CSVeParser::vedTextField, this, [this](const QString& f, const QByteArray& v) {
        veChargerSetTextField(f, v);
    });
    connect(&m_parserCharger, &CSVeParser::vedHexFrame, this, [this](const CSVeParser::TVeHexFrame& frame) {
        switch (frame.command) {
            case VED_CMD_PING_RESPONSE: {
                return;
            }
            /* setting value messages */
            case VED_CMD_GET: {
                if (veUpdateData(frame)) {
                    return;
                }
                break;
            }
            /* update setting messages */
            case VED_CMD_SET: {
                if (veDoSetData(frame)) {
                    return;
                }
                break;
            }
            /* broadcast value messages */
            case VED_CMD_ASYNC: {
                if (veUpdateData(frame)) {
                    return;
                }
                break;
            }
        }
        QStringList finfo;
        if (frame.flags & VED_FLAG_NOT_SUPPORTED) {
            finfo << "not supported";
        }
        if (frame.flags & VED_FLAG_PARAM_ERROR) {
            finfo << "parameter error";
        }
        if (frame.flags & VED_FLAG_UNK_ID) {
            finfo << "unknown register";
        }
        qWarning( //
           "[VE.CHR] UN-HANDLED: cmd=%2d [%s] id=%5d (0x%04X) Flags=0x%02X [%s] Size=%d %s",
           frame.command,
           m_parserCharger.toCmdStr(frame.command).constData(),
           frame.regid,
           frame.regid,
           frame.flags,
           finfo.join(";").toUtf8().constData(),
           frame.ve_in.size,
           frame.source.constData());
    });

    /* ..................................................
     * Cerbo GX Protocol to CarIOS
     * .................................................. */

    // connect(&m_parserCerbo, &CSVeParser::echoInbound, this, &CSVeDirectAcDcCharger::onVedEchoInbound);
    connect(&m_parserCerbo, &CSVeParser::errorOccured, this, [](const QByteArray& messge) {
        qCritical() << "[VE.CGX]" << messge;
    });
    connect(&m_parserCerbo, &CSVeParser::vedTextField, this, [](const QString& f, const QByteArray& v) {
        qDebug() << "[VE.CGX] RECV>" << f.constData() << "=>" << v.constData();
    });
    connect(&m_parserCerbo, &CSVeParser::vedHexFrame, this, [this](const CSVeParser::TVeHexFrame& frame) {
        qDebug( //
           "[VE.CGX] RECV> cmd=%2d [%s] id=%5d (0x%04X) Flags=%04d (0x%04X) %s",
           frame.command,
           m_parserCerbo.toCmdStr(frame.command).constData(),
           frame.regid,
           frame.regid,
           frame.flags,
           frame.flags,
           frame.source.constData());
    });
}

inline bool CSVeDirectAcDcCharger::openInputPort()
{
    if (m_portCharger.isOpen()) {
        return true;
    }

    m_values = {};

    m_portCharger.setPortName(m_configCharger.m_portName);
    m_portCharger.setBaudRate(m_configCharger.m_baudRate);
    m_portCharger.setDataBits(m_configCharger.m_dataBits);
    m_portCharger.setStopBits(m_configCharger.m_stopBits);
    m_portCharger.setFlowControl(m_configCharger.m_flow);
    m_portCharger.setParity(m_configCharger.m_parity);

    if (!m_portCharger.open(QSerialPort::ReadWrite)) {
        return false;
    }

    /* cleanup */
    m_portCharger.flush();
    return true;
}

inline bool CSVeDirectAcDcCharger::openOutputPort()
{
#if 0
    if (m_portCerbo.isOpen()) {
        return true;
    }

    m_portCerbo.setPortName(m_configCerbo.m_portName);
    m_portCerbo.setBaudRate(m_configCerbo.m_baudRate);
    m_portCerbo.setDataBits(m_configCerbo.m_dataBits);
    m_portCerbo.setStopBits(m_configCerbo.m_stopBits);
    m_portCerbo.setFlowControl(m_configCerbo.m_flow);
    m_portCerbo.setParity(m_configCerbo.m_parity);

    if (!m_portCerbo.open(QSerialPort::ReadWrite)) {
        return false;
    }

    m_portCerbo.flush();
#endif
    return true;
}

inline void CSVeDirectAcDcCharger::restartPorts()
{
    QTimer::singleShot(2500, this, [this]() {
        if (!openOutputPort()) {
            qCritical() << "[VE.Direct] Failed to restart VE.Direct output port.";
            close();
            return;
        }
        if (!openInputPort()) {
            qCritical() << "[VE.Direct] Failed to restart VE.Direct input port.";
            close();
            return;
        }
    });
}

inline void CSVeDirectAcDcCharger::setRegister(quint16 regid, float scale, const QVariant& value)
{
    if (!m_values.contains(regid) || !m_values[regid].second.isValid() || m_values[regid].second != value) {
        qDebug(
           "[VE.CHR] SAVE regid: 0x%04X scale: %f value: %s", //
           regid,
           scale,
           value.toString().toLocal8Bit().constData());

        m_values[regid] = QPair<float, QVariant>(scale, value);

        emit dataChanged(regid, m_values[regid]);
    }
}

inline void CSVeDirectAcDcCharger::veHandleInput(CSVeParser* parser, QSerialPort* input, QSerialPort* output)
{
    char c;
    do {
        if (input->read(&c, 1) < 1) {
            break;
        }

        parser->handle(c);

        if (output->isOpen()) {
            output->write(&c, 1);
            output->flush();
        }
    } while (!input->atEnd());
}

inline void CSVeDirectAcDcCharger::veChargerSetTextField(const QString& field, const QByteArray& value)
{
    if (field.isEmpty()) {
        return;
    }

    // qDebug() << "[VE.CHR] RECV> " << field << "value:" << value;

    /* product id -> 0xA330 */
    if (field == "PID") {
        setRegister(0x00001, value.count(), value);
        m_stateData.m_counter++;
    }
    /* firmware release 24bit -> 0342FF */
    else if (field == "FWE") {
        setRegister(0x00002, value.count(), value);
        m_stateData.m_counter++;
    }
    /* serial number -> HQ2247PTFUR */
    else if (field == "SER#") {
        setRegister(0x00003, value.count(), value);
        m_stateData.m_counter++;
    }
    /* voltage -> 12850mV -> 12.850V */
    else if (field == "V") {
        setRegister(0xED8D, 0.001f, value.toDouble());
        m_stateData.m_counter++;
    }
    /* current 0.400A */
    else if (field == "I") {
        setRegister(0xED8F, 0.001f, value.toDouble());
        m_stateData.m_counter++;
    }
    /* time? */
    else if (field == "T") {
        if (value.toUInt() != 0) {
            setRegister(0x2009, 0.01f, value.toUInt());
        }
        m_stateData.m_counter++;
    }
    /* error code */
    else if (field == "ERR") {
        if (value.toInt() != 0) {
            setRegister(0x2009, 1, value.toInt());
        }
        m_stateData.m_counter++;
    }
    /* work mode status */
    else if (field == "CS") {
        if (value.toInt() == 11) {
            setRegister(0x0206, 1, 1);
        }
        else {
            setRegister(0x0206, 1, 0);
        }
        setRegister(0x0201, 1, value.toInt());
        m_stateData.m_counter++;
    }
    /* should be the last one */
    else if (field == "HC#") {
        setRegister(0x0004, value.count(), value);
        m_stateData.m_counter++;
    }

    if (m_stateData.m_counter >= 9) {
        m_stateData.m_counter = 0;
        veSendCommandQueue();
    }
}

inline void CSVeDirectAcDcCharger::veSendFrameTo(CSVEDirect::ved_t* ved, QSerialPort* port)
{
    quint16 regid;
    quint8 cmd, flags;

    /* get  before encode */
    cmd = CSVEDirect::getCommand(ved);
    regid = CSVEDirect::getId(ved);
    flags =
       (cmd == VED_CMD_GET || cmd == VED_CMD_SET || cmd == VED_CMD_ASYNC //
           ? CSVEDirect::getFlags(ved)
           : 0);

    /* encode to VE.HEX frame and send */
    if (CSVEDirect::enframe(ved)) {
        QByteArray outbuf((char*) ved->data, ved->size);
        QByteArray oport = port->portName().toLocal8Bit();
        if (ved->size) {
            qDebug( //
               "[VE.%s] SEND> cmd=%d [%s] id=0x%04X Flags=0x%02X %s",
               oport.constData(),
               cmd,
               m_parserCharger.toCmdStr(cmd).constData(),
               regid,
               flags,
               outbuf.left(outbuf.length() - 1).constData());
        }
        port->flush();
        port->write("\n" + outbuf);
        port->waitForBytesWritten();
    }
}

inline void CSVeDirectAcDcCharger::veSendToCerboGx(CSVEDirect::ved_t* ved)
{
    veSendFrameTo(ved, &m_portCerbo);
}

inline void CSVeDirectAcDcCharger::veSendToCharger(CSVEDirect::ved_t* ved)
{
    veSendFrameTo(ved, &m_portCharger);
}

inline void CSVeDirectAcDcCharger::veSendCommandQueue()
{
    if (m_queue.isEmpty()) {
        return;
    }

    CSVEDirect::ved_t ved = m_queue.takeFirst();
    veSendToCharger(&ved);
}

/* Cerbo GX to Blue Smart Charger */
inline bool CSVeDirectAcDcCharger::veDoSetData(const CSVeParser::TVeHexFrame& frame)
{
    qDebug(
       "[VE.CHR] SET regid: 0x%04X flags: 0x%02X size: %d", //
       frame.regid,
       frame.flags,
       frame.ve_in.size);

    if (frame.ve_in.size) {
        veUpdateData(frame);
    }

    return true;
}

/* Blue Smart Charger Input -> CarIOS */
inline bool CSVeDirectAcDcCharger::veUpdateData(const CSVeParser::TVeHexFrame& frame)
{
    CSVeParser::TVeHexFrame hf = frame;
    CSVEDirect::ved_t* ved_in = &hf.ve_in;
    double value = 0.0f;
    float scale = 1.0f;

    /* invalid frame, at least one byte data payload required */
    if (ved_in->size < 5) {
        qWarning() << "[VE.CHR] Data size to less. Size:" //
                   << ved_in->size << "Expected: >= 5";
        return false;
    }

    switch (frame.regid) {
        /* VE_REG_GROUP_ID */
        case 0x0104: {
            value = CSVEDirect::getU8(ved_in);
            break;
        }
        /* VE_REG_IDENTIFY or VE_REG_CAN_SELECT */
        case 0x010E: {
            value = CSVEDirect::getU8(ved_in);
            break;
        }
        /* VE_REG_CHR_NUMBER_OUTPUTS */
        case 0xEDDE: {
            value = CSVEDirect::getU8(ved_in);
            break;
        }
        /* VE_REG_CAPABILITIES1 */
        case 0x0140: {
            value = CSVEDirect::getU32(ved_in);
            break;
        }
        /* VE_REG_CAPABILITIES4 */
        case 0x0143: {
            value = CSVEDirect::getU32(ved_in);
            break;
        }
        /* VE_REG_AC_IN_1_CURRENT_LIMIT */
        case 0x0210: {
            value = CSVEDirect::getU16(ved_in);
            scale = 1 / 100;
            break;
        }
        /* VE_REG_CHR_MIN_CURRENT */
        case 0xEDC8: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.1f;
            break;
        }
        /* VE_REG_LINK_NETWORK_STATUS */
        case 0x200F: {
            value = CSVEDirect::getU8(ved_in);
            break;
        }
        /* VE_REG_LINK_CHARGE_CURRENT_LIMIT */
        case 0x2015: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.001f;
            break;
        }
        /* VE_REG_LINK_VSENSE */
        case 0x2002: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.01f;
            break;
        }
        /* VE_REG_LINK_TSENSE */
        case 0x2003: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.01f;
            break;
        }
        /* VE_REG_LINK_BATTERY_CURRENT */
        case 0x200A: {
            value = CSVEDirect::getU32(ved_in);
            break;
        }
        /* VE_REG_DESCRIPTION1 */
        case 0x010C: {
            QByteArray buffer;
            for (int i = 4; i < ved_in->size && ved_in->data[i] != 0; i++) {
                buffer.append((char) ved_in->data[i]);
            }
            qDebug(
               "[VE.CHR] SAVE regid: 0x%04X value: %s", //
               frame.regid,
               buffer.constData());
            setRegister(frame.regid, scale, buffer);
            return true;
        }
        /* VE_REG_HISTORY_CYCLE_SEQUENCE_NUMBER
         *
         * VE_REG_HISTORY_CYCLE00 VE_REG_HISTORY_CYCLE01
         * VE_REG_HISTORY_CYCLE02 VE_REG_HISTORY_CYCLE03
         * VE_REG_HISTORY_CYCLE04 VE_REG_HISTORY_CYCLE05
         * VE_REG_HISTORY_CYCLE06 VE_REG_HISTORY_CYCLE07
         * VE_REG_HISTORY_CYCLE08 VE_REG_HISTORY_CYCLE09
         * VE_REG_HISTORY_CYCLE10 VE_REG_HISTORY_CYCLE11
         * VE_REG_HISTORY_CYCLE12 VE_REG_HISTORY_CYCLE13
         * VE_REG_HISTORY_CYCLE14 VE_REG_HISTORY_CYCLE15
         * VE_REG_HISTORY_CYCLE16 VE_REG_HISTORY_CYCLE17
         * VE_REG_HISTORY_CYCLE18 VE_REG_HISTORY_CYCLE19
         * VE_REG_HISTORY_CYCLE20 VE_REG_HISTORY_CYCLE21
         * VE_REG_HISTORY_CYCLE22 VE_REG_HISTORY_CYCLE23
         * VE_REG_HISTORY_CYCLE24 VE_REG_HISTORY_CYCLE25
         * VE_REG_HISTORY_CYCLE26 VE_REG_HISTORY_CYCLE27
         * VE_REG_HISTORY_CYCLE28 VE_REG_HISTORY_CYCLE29
         * VE_REG_HISTORY_CYCLE30 VE_REG_HISTORY_CYCLE31
         * VE_REG_HISTORY_CYCLE32 VE_REG_HISTORY_CYCLE33
         * VE_REG_HISTORY_CYCLE34 VE_REG_HISTORY_CYCLE35
         * VE_REG_HISTORY_CYCLE36 VE_REG_HISTORY_CYCLE37
         * VE_REG_HISTORY_CYCLE38 VE_REG_HISTORY_CYCLE39
         * VE_REG_HISTORY_CYCLE40
         *
         * Sequence number for the cycle "history" which will
         * change in case of a new cycle. This can be used as a
         * trigger to fetch the entire cycle history.
         * un32
         */
        case 0x1099: {
            value = CSVEDirect::getU32(ved_in);
            break;
        }
        /* The number of charge cycle history records.
         * un8 */
        case 0x106F: {
            value = CSVEDirect::getU8(ved_in);
            break;
        }
        /* VE_REG_UPTIME un32
         * uptime since boot in seconds */
        case 0x0120: {
            return true;
        }
        /* Device state un8 read-only
         * 0=off
         * 1=low power mode
         * 2=fault
         * 3=bulk
         * 4=absorption
         * 5=float
         * 6=storage
         * 7=equalize
         * 8=passthru
         * 9=inverting
         * 10=assisting
         * 11=psu (Spannungsversorgung)
         * 0xFC=hub1
         * 0xff=not available
         *
         * state 11 psu mode is NOT in the NMEA2000 specification. It is a victron
         * specific value. New states should be added from the top instead of from
         * the bottom. NMEA2000 equivalent: DD342 Converter Operating State as found
         * in PGN 127750 (v2. 000) VE. Text equivalent: CS
         *
         * value=0...  Ladegeraet
         * value=11 Spannungsversorgung
         */
        case 0x0201: {
            value = CSVEDirect::getU8(ved_in);
            break;
        }
        /* Device Function
         * value=0 Charger
         * value=1 Spannungsversorgung
         */
        case 0x0206: {
            value = CSVEDirect::getU8(ved_in);
            break;
        }
        /* Device off reason 2
         * bits[0] = "VE_REG_DEVICE_OFF_2_NO_INPUT_POWER"
         *  0 = "No" 1 = "Yes" no/low mains/panel/battery power
         * bits[1] = "VE_REG_DEVICE_OFF_2_HARD_POWER_SWITCH"
         *  0 = "No" 1 = "Yes" physical switch
         * bits[2] = "VE_REG_DEVICE_OFF_2_SOFT_POWER_SWITCH"
         *  0 = "No" 1 = "Yes" remote via N device_mode and/or push-button
         * bits[3] = "VE_REG_DEVICE_OFF_2_REMOTE_INPUT"
         *  1 = "No" 1 = "Yes" remote input connector
         * bits[4] = "VE_REG_DEVICE_OFF_2_INTERNAL_REASON"
         *  0 = "No" 1 = "Yes condition" preventing start-up
         * bits[5] = "VE_REG_DEVICE_OFF_2_PAYGO"
         *  0 = "No" 1 = "Yes" N need token for operation
         * bits[6] = "VE_REG_DEVICE_OFF_2_BMS"
         *  0 = "No" 1 = "Yes ?" allow-to-charge/allow-to-discharge signals from BMS
         * bits[7] = "VE_REG_DEVICE_OFF_2_ENGINE_SD_DETECTION"
         *  0 = "No" 1 = "Yes" engine shutdown detected through low input voltage
         * bits[8] = "VE_REG_DEVICE_OFF_2_ANALYZING_INPUT_VOLTAGE"
         *  0 = "No" 1 = "Yes" converter off to check input voltage without cable losses
         * bits[23] = "reserved" 0x000000 = reserved
         * :A0D200000001E */
        case 0x0207: {
            value = CSVEDirect::getU32(ved_in);
            break;
        }
        /* Charger "link" network info (this reports the
         * overall network activity)
         */
        case 0x200D: {
            value = CSVEDirect::getU16(ved_in);
            break;
        }
        /* Charger link  equalisation pending?
         * VE_REG_LINK_EQUALISATION_PENDING un8
         */
        case 0x2018: {
            value = CSVEDirect::getU8(ved_in);
            break;
        }
        /* Blue Power Charger - low current mode
         * un8 "0:0ff" "1:on" "2:night"
         */
        case 0xE001: {
            value = CSVEDirect::getU8(ved_in);
            break;
        }
        /* Blue Power Charger - low current mode time left
         * night mode 8 hours w/o fan with 15A current
         * counter 28800 seconds to 0 seconds
         */
        case 0xE002: {
            value = CSVEDirect::getU32(ved_in);
            break;
        }
        /* Date/time of last change of the "settings"
         * in unix-timestamp format
         */
        case 0xEC41: {
            value = CSVEDirect::getU32(ved_in);
            break;
        }
        /* Battery Re-bulk Offset Voltage & Level un16
         * "[0.01V]" 0xFFFF = Not Available}E Set VE_REG_BAT_TYPE to 0xFF to
         * use and read the user-defined setting.
         * write: always writes to the user-defined setting in non-volatile memory.
         * Important for mppt chargers:
         * always set the @ref VE_REG_BAT_VOLTAGE register > to the correct system
         * voltage before writing to this register.
         */
        case 0xED2E: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.01f;
            break;
        }
        /* Charger specific parameters
         * actual charge voltage channel 1
         * sn16 voltage [0.01V] read-only
         *
         * output voltage - same is 0xEDD5 */
        case 0xED8D: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.01f;
            break;
        }
        /* Charger specific parameters
         * additional charger state info
         * un8 "state" read-only
         * see VE_REG_CHR_CUSTOM_STATE section for possible values
         */
        case 0xEDD4: {
            value = CSVEDirect::getU8(ved_in);
            break;
        }
        /* Charger specific parameters - actual charge voltage
         * un16 voltage [0.01V] read-only
         * HEX protocol compat. data present in regular NMEA pgn
         *
         * output voltage - same is 0xED8D */
        case 0xEDD5: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.01f;
            break;
        }
        /* VE_REG_CHR_TEMPERATURE
         * Charger specific parameters -
         * sn16 temperature [0.01 degrees C] read-only
         * scale / 100
         *
         * value=3060 -> 30.6°C
         */
        case 0xEDDB: {
            value = (float) CSVEDirect::getU16(ved_in);
            scale = 0.01f;
            break;
        }
        /* Stop at low temperature
         * Abschalten bei Temperatur niedrig 5°C
         * scale / 100
         * value=500
         */
        case 0xEDE0: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.01f;
            break;
        }
        /* Charger battery parameters - Re-bulk Current Level
         * (float/stroage -> bulk transition)
         * un16 current [0.1A]
         */
        case 0xEDE1: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.1f;
            break;
        }
        /* Battery Re-bulk Voltage Level
         * un16 [0.01V]
         * 0xFFFF = Not Available
         * Set VE_REG_BAT_TYPE to 0xFF to use and read the user-defined setting.
         * write: always writes to the user-defined setting in non-volatile memory.
         * Important for mppt chargers: always set the @ref VE_REG_BAT_VOLTAGE register
         * to the correct system voltage before writing to this register.
         */
        case 0xEDE2: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.01f;
            break;
        }
        /* Battery Equalisation Time Duration
         * un16 [0.01 hours]
         * Set VE_REG_BAT_TYPE to 0xFF to use and read the user-defined setting.
         * write: always writes to the user-defined setting in non-volatile memory.
         *
         * Dauer der Regenrierung hh:mm scale / 100
         * value=100 value=202 -> 2:02
         */
        case 0xEDE3: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.01f;
            break;
        }
        /* Battery Equalisation Current Percentage
         * un8 [1%]
         * 0xFF = Not Available}
         * Set VE_REG_BAT_TYPE to 0xFF to use and read the user-defined setting.
         * write: always writes to the user-defined setting in non-volatile memory.
         *
         * Prozentsatz Strom regenerierung scale x1
         * value=3
         */
        case 0xEDE4: {
            value = CSVEDirect::getU8(ved_in);
            break;
        }
        /* Battery Equalisation Auto Stop
         * un8
         * 0 = No / 1 = Yes
         * Set VE_REG_BAT_TYPE to 0xFF to use and read the user-defined setting.
         * write: always writes to the user-defined setting in non-volatile memory.
         *
         * Stopp-Modus Regenerierung
         * 0 = Festgelegt
         * 1 = Automatisch, an Spannung
         */
        case 0xEDE5: {
            value = CSVEDirect::getU8(ved_in);
            break;
        }
        /* Low-temp charge current
         * un16 [0.1A]
         * 0 = stop "charging"
         * 0xFFFF = use maximum charger current.
         * For Lithium "batteries" used below 6 degrees C
         */
        case 0xEDE6: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.1f;
            break;
        }
        /* Charger battery parameters - tail current
         * (abs -> float transition)
         * un16 current [0.1A]
         *
         * Schweifstrom scale x1
         * value=0 -> deactivated
         * value=2 -> 2A
         */
        case 0xEDE7: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.1f;
            break;
        }
        /* Charger battery parameters - maximum charge current
         * un16 current [0.1A])
         * read: the parameter as it is currently in use (set VE_REG_BAT_TYPE to
         *       0xFF to use and read the user-defined setting)
         * write: always writes to the user-defined setting in non-volatile memory
         *
         * Maximum Current A scale x10
         * current => 150
         * current => 300
         */
        case 0xEDF0: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.1f;
            break;
        }
        /* VE_REG_BAT_TYPE un8
         * 0 = Normal
         * 1 = Normal + Reginerierung
         * 2 = Hoch
         * 3 = Hoch + Regenerierung
         * 4 = Li-ion
         */
        case 0xEDF1: {
            value = CSVEDirect::getU8(ved_in);
            break;
        }
        /* Charger battery parameters - temperature compensation
         * sn16 temperature [0.01mV/degree K])
         * read: the parameter as it is currently in use (set VE_REG_BAT_TYPE to
         *       0xFF to use and read the user-defined setting)
         * write: always writes to the user-defined setting in non-volatile memory
         * important for mppt chargers: always set the VE_REG_BAT_VOLTAGE register
         * to the correct system voltage before writing to this register.
         *
         * Temperaturkompensation (scale / 100)
         * value=0     -> deactivated
         * value=-1620 -> -16.20 mV/°C
         */
        case 0xEDF2: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.01f;
            break;
        }
        /* Charger battery parameters - equalization voltage
         * un16 voltage [0.01V]
         * read: the parameter as it is currently in use (set VE_REG_BAT_TYPE to
         *       0xFF to use and read the user-defined setting)
         * write: always writes to the user-defined setting in non-volatile memory
         * important for mppt chargers: always set the VE_REG_BAT_VOLTAGE register
         * to the correct system voltage before writing to this register.
         */
        case 0xEDF4: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.01f;
            break;
        }
        /* Charger battery parameters - storage voltage
         * un16 voltage [0.01V]
         * read: the parameter as it is currently in use (set VE_REG_BAT_TYPE to
         *       0xFF to use and read the user-defined setting)
         * write: always writes to the user-defined setting in non-volatile memory
         * important for mppt chargers: always set the VE_REG_BAT_VOLTAGE register
         * to the correct system voltage before writing to this register.
         *
         *  Lagerungsspannung (scale / 100)
         *  Smart Lithium LiFePo4
         *      Voltage => 13.50
         */
        case 0xEDF5: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.01f;
            break;
        }
        /* Charger battery parameters - float
         * un16 voltage [0.01V]
         * read: the parameter as it is currently in use (set VE_REG_BAT_TYPE to
         *       0xFF to use and read the user-defined setting)
         * write: always writes to the user-defined setting in non-volatile memory
         * important for mppt chargers: always set the VE_REG_BAT_VOLTAGE register
         * to the correct system voltage before writing to this register.
         *
         *  Erhaltungsspannung (scale / 100)
         *  Smart Lithium LiFePo4
         *      Voltage => 13.50
         */
        case 0xEDF6: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.01f;
            break;
        }
        /* Charger battery parameters - absorption voltage
         * un16 voltage [0.01V]
         * read: the parameter as it is currently in use (set VE_REG_BAT_TYPE to
         *       0xFF to use and read the user-defined setting)
         * write: always writes to the user-defined setting in non-volatile memory
         * important for mppt chargers: always set the VE_REG_BAT_VOLTAGE register
         * to the correct system voltage before writing to this register.
         *
         * Absorptionsspannung (scale / 100)
         *  Smart Lithium LiFePo4
         *      Voltage => 14.40
         */
        case 0xEDF7: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.01f;
            break;
        }
        /* Charger battery parameters - repeated absorption time interval
         * un16 time [0.01 days]
         * read: the parameter as it is currently in use (set VE_REG_BAT_TYPE to
         *       0xFF to use and read the user-defined setting)
         * write: always writes to the user-defined setting in non-volatile memory
         * Wiederholte Konstantspannung alle n Tage (scale / 100)
         * value=700 or 600
         */
        case 0xEDF8: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.01f;
            break;
        }
        /* Charger battery parameters - repeated absorption time
         * duration ( un16= time [0.01 hours]) t
         * read: the parameter as it is currently in use (set VE_REG_BAT_TYPE to
         *       0xFF to use and read the user-defined setting)
         * write: always writes to the user-defined setting in non-volatile memory
         * value=100
         */
        case 0xEDF9: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.01f;
            break;
        }
        /* Charger battery parameters - maximum float time
         * ( un16= time [0.01 hours]) time 0=off t
         * read: the parameter as it is currently in use (set VE_REG_BAT_TYPE to
         *       0xFF to use and read the user-defined setting)
         * write: always writes to the user-defined setting in non-volatile memory
         * value=800
         */
        case 0xEDFA: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.01f;
            break;
        }
        /* Charger battery parameters - maximum absorption time
         * un16 time [0.01 hours] 0=off
         * read: the parameter as it is currently in use (set VE_REG_BAT_TYPE to
         *       0xFF to use and read the user-defined setting)
         * write: always writes to the user-defined setting in non-volatile memory
         */
        case 0xEDFB: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.01f;
            break;
        }
        /* Bulk Time Limit
         * Bulk-Zeitbegrenzung hh:mm (scale / 100)
         * value=2400 -> 24:00
         */
        case 0xEDFC: {
            value = CSVEDirect::getU16(ved_in);
            break;
        }
        /* Charger battery parameters - traction curve/auto
         * equalize (un8)
         *  mode:
         *  0=off "(default)"
         *  1..250=repeat every n days (1=every "day" 2=every other "day" etc)
         *  read: the parameter as it is currently in use (set VE_REG_BAT_TYPE to
         *        0xFF to use and read the user-defined setting)
         *  write: always writes to the user-defined setting in non-volatile memory
         *  value => 0
         */
        case 0xEDFD: {
            value = CSVEDirect::getU8(ved_in);
            break;
        }
        /* Charger battery parameters - adaptive mode un8
         * 0=off
         * 1=on (default)
         * read: the parameter as it is currently in use (set VE_REG_BAT_TYPE to
         *       0xFF to use and read the user-defined setting)
         * write: always writes to the user-defined setting in non-volatile memory
         * Absorbtionsdauer
         * 0 = Fest
         * 1 = Adaptiv
         */
        case 0xEDFE: {
            value = CSVEDirect::getU8(ved_in);
            break;
        }
        /* Charger battery parameters - battery safe mode (un8)
         * mode: "0=off" 1=on (default)t
         * read: the parameter as it is currently in use (set VE_REG_BAT_TYPE to
         *       0xFF to use and read the user-defined setting)
         * write: always writes to the user-defined setting in non-volatile memory
         * value=1
         */
        case 0xEDFF: {
            value = CSVEDirect::getU8(ved_in);
            break;
        }
        /* Battery Used VSense (un16) "[0.01V]"
         * 0xFFFF = Not Available */
        case 0xEE16: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.01f;
            break;
        }
        /* Clear history command (no data) Compatible
         * with BMV HEX protocol */
        case 0x1030: {
            return true;
        }
        /* Table
         * History data cumulative service record (non-resettable)
         * un8= "version"
         * rest of the frame depends on the version
         * un32 = Operation time "[s] 0:00"
         * un32 = Charged Ah "[0.1Ah]"
         * un32 = Charge cycles "started"
         * un32 = Charge cycles "completed"
         * un32 = "Powerups"
         * un32 = Deep discharges
         * value of 0xFF/0xFFFF/0xFFFFFFFF (depending on the type)
         * indicates that a field is unknown/not available */
        case 0x1042: {
            QString values;
            uint offset = 0;
            values.append(tr("%1 ").arg(CSVeParser::readU8(ved_in, &(offset))));
            offset += 1;
            values.append(tr("%1 ").arg(CSVeParser::readU32(ved_in, &(offset))));
            offset += 4;
            values.append(tr("%1 ").arg(CSVeParser::readU32(ved_in, &(offset))));
            offset += 4;
            values.append(tr("%1 ").arg(CSVeParser::readU32(ved_in, &(offset))));
            offset += 4;
            values.append(tr("%1 ").arg(CSVeParser::readU32(ved_in, &(offset))));
            offset += 4;
            values.append(tr("%1 ").arg(CSVeParser::readU32(ved_in, &(offset))));
            offset += 4;
            values.append(tr("%1 ").arg(CSVeParser::readU32(ved_in, &(offset))));
            offset += 4;
            setRegister(frame.regid, scale, QVariant::fromValue(values));
            return true;
        }
        /* Table
         * History data cumulative service record (non-resettable)
         * un8= "version"
         * rest of the frame depends on the version
         * un32 = Operation time "[s]" 0:00
         * un32 = Charged Ah "[0.1Ah]"
         * un32 = Charge cycles "started"
         * un32 = Charge cycles "completed"
         * un32 = "Powerups"
         * un32 = Deep discharges
         * value of 0xFF/0xFFFF/0xFFFFFFFF (depending on the type)
         * indicates that a field is unknown/not available */
        case 0x1043: {
            QString values;
            uint offset = 0;
            values.append(tr("%1 ").arg(CSVeParser::readU8(ved_in, &(offset))));
            offset += 1;
            values.append(tr("%1 ").arg(CSVeParser::readU32(ved_in, &(offset))));
            offset += 4;
            values.append(tr("%1 ").arg(CSVeParser::readU32(ved_in, &(offset))));
            offset += 4;
            values.append(tr("%1 ").arg(CSVeParser::readU32(ved_in, &(offset))));
            offset += 4;
            values.append(tr("%1 ").arg(CSVeParser::readU32(ved_in, &(offset))));
            offset += 4;
            values.append(tr("%1 ").arg(CSVeParser::readU32(ved_in, &(offset))));
            offset += 4;
            values.append(tr("%1 ").arg(CSVeParser::readU32(ved_in, &(offset))));
            offset += 4;
            setRegister(frame.regid, scale, QVariant::fromValue(values));
            return true;
        }
        /* Table
         * History data cycle record - Cycle 0 = active cycle F (fast-packet:
         * un8= "version"
         * rest of the frame depends on the version) version = 0:00
         * un32 = start time "[s]"
         * un32 = bulk time "[s]"
         * un32 = abs. time "[s]"
         * un32 = recon./eq. time "[s]"
         * un32 = float time "[s]"
         * un32 = storage time [s]?
         * un32 = bulk charge "[0. 1Ah]"
         * un32 = abs. charge "[0. 1Ah]"
         * un32 = recon./eq. charge "[0. 1Ah]"
         * un32 = float charge "[0. 1Ah]"
         * un32 = storage charge [0. 1Ah]
         * un16 = start voltage "[0.01V]"
         * un16 = end voltage "[0.01V]"
         * un8  = battery "type" / "reserved" / termination "reason"
         * un8  = error code a
         * value of 0xFF/0xFFFF/0xFFFFFFFF (depending on the type)
         * indicates that a field is unknown/not available */
        case 0x1070: {
            QString values;
            uint offset = 0;
            values.append(tr("%1 ").arg(CSVeParser::readU8(ved_in, &(offset))));
            offset += 1;
            values.append(tr("%1 ").arg(CSVeParser::readU32(ved_in, &(offset))));
            offset += 4;
            values.append(tr("%1 ").arg(CSVeParser::readU32(ved_in, &(offset))));
            offset += 4;
            values.append(tr("%1 ").arg(CSVeParser::readU32(ved_in, &(offset))));
            offset += 4;
            values.append(tr("%1 ").arg(CSVeParser::readU32(ved_in, &(offset))));
            offset += 4;
            values.append(tr("%1 ").arg(CSVeParser::readU32(ved_in, &(offset))));
            offset += 4;
            values.append(tr("%1 ").arg(CSVeParser::readU32(ved_in, &(offset))));
            offset += 4;
            values.append(tr("%1 ").arg(CSVeParser::readU32(ved_in, &(offset))));
            offset += 4;
            values.append(tr("%1 ").arg(CSVeParser::readU32(ved_in, &(offset))));
            offset += 4;
            values.append(tr("%1 ").arg(CSVeParser::readU32(ved_in, &(offset))));
            offset += 4;
            values.append(tr("%1 ").arg(CSVeParser::readU32(ved_in, &(offset))));
            offset += 4;
            values.append(tr("%1 ").arg(CSVeParser::readU32(ved_in, &(offset))));
            offset += 4;
            values.append(tr("%1 ").arg(CSVeParser::readU16(ved_in, &(offset))));
            offset += 2;
            values.append(tr("%1 ").arg(CSVeParser::readU16(ved_in, &(offset))));
            offset += 2;
            values.append(tr("%1 ").arg(CSVeParser::readU8(ved_in, &(offset))));
            offset += 1;
            values.append(tr("%1 ").arg(CSVeParser::readU8(ved_in, &(offset))));
            offset += 1;
            setRegister(frame.regid, scale, QVariant::fromValue(values));
            return true;
        }

        /* -----------------------------
         * Smart Network created
         * ----------------------------- */

        /* The network name for BLE networking.
         * This is a zero terminted string.
         * :A14EC00436172494F534A
         */
        case 0xEC14: {
            break;
        }
        /* The network id for BLE networking.
         * The data is non printable.
         * :A12EC004DCF31
         */
        case 0xEC12: {
            value = CSVEDirect::getU16(ved_in);
            break;
        }
        /* The network key for BLE networking.
         * The data is non printable.
         * :A13EC00E3C841BD37E9CFB9AF1BF5DD1E9F0EFE96
         */
        case 0xEC13: {
            break;
        }
        /* Charger "link" voltage set-point
         * un16 = voltage "[0.01V]" read-only
         * only the master broadcasts voltage set-point
         * :A012000000525
         */
        case 0x2001: {
            value = CSVEDirect::getU16(ved_in);
            scale = 0.01f;
            break;
        }
        /* Charger "link" state elapsed time
         * un32 = time "[ms]" read-only
         * :A0720000000000024
         */
        case 0x2007: {
            value = CSVEDirect::getU32(ved_in);
            break;
        }
        /* Charger "link" solar absorption time
         * un16 = time "[0.01 hours]" read-only
         * :A082000000023
         */
        case 0x2008: {
            value = CSVEDirect::getU16(ved_in);
            break;
        }
        /* Charger "link" error code (@remark an external network master
         * can use this to inject an error code (this will be shown on
         * the "unit)" 60s timeout)
         * :A0920000022
         */
        case 0x2009: {
            value = CSVEDirect::getU8(ved_in);
            break;
        }
        /* Charger "link" absorption end time
         * un16 = time "[0.01 hours]" read-only
         * value=200
         * :A422000C80021
         */
        case 0x2042: {
            value = CSVEDirect::getU16(ved_in);
            break;
        }
        /* Charger "link" device state (@remark share the
         * device state @VE_REG_DEVICE_STATE from the master
         * to the "slaves" slaves have 60s timeout)
         * :A0C2000FF20
         */
        case 0x200C: {
            value = CSVEDirect::getU8(ved_in);
            break;
        }
        /* Unix timestamp ??
         * Charger link total charge current?
         * sn32
         * :A132000FFFFFF7F9C
         */
        case 0x2013: {
            value = CSVEDirect::getU32(ved_in);
            break;
        }
        /* Charger "link" network mode un8:
         * B0 networked
         * B1 remote "algo"
         * B2 remote "hub-1"
         * B3 remote bms 1 puts a ve.direct device in remote controlled mode 60s timeout
         * :A0E200021FC
         * value=33
         * value=1 -> Ladegeraet
         */
        case 0x200E: {
            value = CSVEDirect::getU8(ved_in);
            break;
        }
        /* NumberOfInRangeDevices {un8 NumberOfInRangeDevices}
         * The number of devices from which the local node has received one or
         * more broadcasted VE_REG's. The local node will also be included in
         * this number (hence the value will always be at least 1). Nodes from
         * which nothing was M received during an implementation dependent period
         * may be excluded from this number.
         * :A30EC00012E */
        case 0xEC30: {
            value = CSVEDirect::getU8(ved_in);
            break;
        }
        /* BLE networking devices in range list
         * un32 Address In Range DEVICE_1 0xFFFFFFFF = Not Available
         * un16 Product "ID" 0xFFFF = Not Available
         * un8  Time "[1s]" 0xFE = More than 253 "sec" 0xFF = Not Available
         * un32 Application H "Version" 0xFFFFFFFF = Not Available
         * un32 Address In Range "DEVICE_2" O0xFFFFFFFF = Not Available
         * un16 Product "ID" 0xFFFF = Not Available
         * un8  Time "[1s]" 0xFE = More than 253 "sec" 0xFF = Not Available
         * un32 Application $ "Version" 0xFFFFFFFF = Not Available} I @remark
         *      Start of the list of devices from which broadcasted VE_REG's are received.
         *      The first node in this list shall always be the local node. Nodes O for
         *      which no VE_REG's are received "anymore" may be removed from the list
         *      after an implementation dependent period. When the Address value is
         *      set to 0xFFFFFFFF it marks the end of the list at this moment
         *      (no more to follow).
         * :A31EC00018396F49930A300FF4203007FFFFFFFFFFFFFFFFFFFFFFFFFFFFFFF7FFFFFFFFF85 */
        case 0xEC31: {
            QString values;
            uint offset = 0;
            values.append(CSVeParser::readU32(ved_in, &(offset)));
            offset += 4;
            values.append(CSVeParser::readU16(ved_in, &(offset)));
            offset += 2;
            values.append(CSVeParser::readU8(ved_in, &(offset)));
            offset += 1;
            values.append(CSVeParser::readU32(ved_in, &(offset)));
            offset += 4;
            values.append(CSVeParser::readU32(ved_in, &(offset)));
            offset += 4;
            values.append(CSVeParser::readU16(ved_in, &(offset)));
            offset += 2;
            values.append(CSVeParser::readU8(ved_in, &(offset)));
            offset += 1;
            values.append(CSVeParser::readU32(ved_in, &(offset)));
            offset += 4;
            setRegister(frame.regid, scale, QVariant::fromValue(values));
            return true;
            break;
        }
        /* The number of unique VE_REG's this node has received
         * since it was activated (up and running)
         * :A16EC000445 */
        case 0xEC16: {
            value = CSVEDirect::getU8(ved_in);
            break;
        }
        /* BLE networking reception list
         * un16 Received "VE_REG_1" 0xFFFF = Not N Available
         * un8  Time "[1s]" 0xFE = More than 253 "sec" 0xFF = Not Available
         * un8  Priority" 0x00 = Lowest "Prio" 0x0F = Highest "Prio" 0xFF = Not Available
         * un32 Sender" 0xFFFFFFFF = Not Available
         * un16 Received "VE_REG_2" 0xFFFF = Not Available
         * un8  Time "[1s]" 0xFE = More than 253 "sec" 0xFF = Not L Available
         * un8  Priority" 0x00 = Lowest "Prio" 0x0F = Highest "Prio" 0xFF = Not Available
         * un32 Sender 0xFFFFFFFF = Not Available
         * un16 Received "VE_REG_3" 0xFFFF = Not Available
         * un8  Time "[1s]" 0xFE = More than 253 "sec" 0xFF = Not Available
         * un8  Priority 0x00 = Lowest "Prio" 0x0F = Highest "Prio" 0xFF = Not Available
         * un32 Sender 0xFFFFFFFF = Not Available
         * un16 Received "VE_REG_4" 0xFFFF = Not Available
         * un8  Time "[1s]" 0xFE = More N than 253 "sec" 0xFF = Not Available
         * un8  Priority 0x00 = Lowest "Prio" 0x0F = Highest "Prio" 0xFF = Not Available
         * un32 Sender 0xFFFFFFFF = Not Available} @remark Continuation of the list of
         * unique VE_REG's this node has received since it was activated (up and running).
         * Only to the node known VE_REG's will J be returned. Only the last sender for
         * a specific VE_REG will be returned. When the VE_REG value is set to 0xFFFF it
         * marks the end of the list at this moment (no more to follow)
         * :A20EC00ECEDFFFFFFFFFFFF8DEDFFFFFFFFFFFF8CEDFFFFFFFFFFFF3EECFFFFFFFFFFFF61 */
        case 0xEC20: {
            break;
        }
        /* The number of VE_REG's this node is broadcasting in the
         * BLE network it is configured to participate in.
         * :A15EC00004A */
        case 0xEC15: {
            value = CSVEDirect::getU8(ved_in);
            break;
        }
        /* DC Channel 1 Current
         * sn32 DC Channel 1 Current "[0.001A]" (scale / 1000)
         * 0x7FFFFFFF = Not Available
         * value=450 -> 450mA -> 0.45A */
        case 0xED8C: {
            value = (float) (CSVEDirect::getU32(ved_in));
            scale = 0.001f;
            break;
        }
        /* Charger specific parameters - actual charge current channel 1
         * sn16 current "[0.1A]" read-only
         * :A8FED000100CE */
        case 0xED8F: {
            value = (float) CSVEDirect::getU16(ved_in);
            scale = 0.1f;
            break;
        }
        /* Charger specific parameters - actual charge current
         * sn16 current "[0.1A]" read-only
         * HEX protocol compat. data present in regular NMEA pgn
         */
        case 0xEDD7: {
            value = (float) CSVEDirect::getU16(ved_in);
            scale = 0.1f;
            break;
        }
        /* Charger battery parameters - power supply mode voltage setting
         * un16= voltage [0.01V])
         * read: the parameter as it is currently in use (set VE_REG_BAT_TYPE to
         *       0xFF to use and read the user-defined setting)
         * write: always writes to the user-defined setting in non-volatile memory
         * important for mppt chargers: always set the VE_REG_BAT_VOLTAGE register
         * to the correct system voltage before writing to this register
         *
         * Output Voltage Spannungsversorgung (scale / 100)
         * value=1280 -> 12.80V
         */
        case 0xEDE9: {
            value = (float) CSVEDirect::getU16(ved_in);
            scale = 0.01f;
            break;
        }
        /* Battery Re-bulk Method un8
         * 0 = Spannung
         * 1 = Konstantstrom
         */
        case 0xEE17: {
            value = CSVEDirect::getU8(ved_in);
            break;
        }
        default: {
            return false;
        }
    }

    setRegister(frame.regid, scale, value);

    return true;
}
