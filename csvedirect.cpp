/*********************************************************************
 * Copyright (c) 2021 Strathbogie Brewing Company
 * Copyright EoF Software Labs. All Rights Reserved.
 * Copyright EoF Software Labs Authors.
 * Written by B. Eschrich (bjoern.eschrich@gmail.com)
 * SPDX-License-Identifier: MIT License
 **********************************************************************/
#include <QDebug>
#include <csvedirect.h>

CSVEDirect::CSVEDirect(QObject* parent)
    : QObject(parent)
{
}

/* ---------------------------------------------------------------
 * VE.Direct VE.HEX
 * --------------------------------------------------------------- */

static quint8 bin2hex(quint8 bin)
{
    bin &= 0x0F;
    if (bin < 10)
        return bin + '0';
    return bin + 'A' - 10;
}

static quint8 hex2bin(quint8 hex)
{
    if (hex < '0')
        return 0;
    quint8 val = hex - '0';
    if (val > 9) {
        val -= 7;
        if ((val > 15) || (val < 10))
            return 0;
    }
    return val;
}

quint16 CSVEDirect::enframe(ved_t* vedata)
{
    quint8* input = vedata->data;
    quint8 buffer[frameSize(0)];
    quint8* output = buffer;
    quint8 csum = 0x00;
    quint8 src = *input++;
    *output++ = ':';
    *output++ = bin2hex(src);
    csum -= src;
    while (input < vedata->data + vedata->size) {
        src = *input++;
        *output++ = bin2hex(src >> 4);
        *output++ = bin2hex(src);
        csum -= src;
    }
    csum += 0x55;
    *output++ = bin2hex(csum >> 4);
    *output++ = bin2hex(csum);
    *output++ = '\n';
    *output = '\0';

    vedata->size = output - buffer;
    memcpy(vedata->data, buffer, vedata->size);
    return vedata->size;
}

quint16 CSVEDirect::deframe(ved_t* vedata, char inByte)
{
    if (inByte == ':') {
        vedata->size = 0; // reset data buffer
        memset(vedata->data, 0, FRAME_BUFF_SIZE);
    }
    if (vedata->size < (quint16) frameSize(2)) {
        vedata->data[vedata->size++] = inByte;
        if (inByte == '\n') {
            vedata->data[vedata->size] = '\0';
            quint8* input = vedata->data;
            quint8* output = vedata->data;
            quint8 csum = 0x00;
            while ((*input != '\n') && (*input != '\0')) {
                quint8 byte = hex2bin(*input++) << 4;
                byte += hex2bin(*input++);
                csum += byte;
                *output++ = byte;
            }
            if (csum == 0x55) {
                vedata->size = output - vedata->data - 1;
                return vedata->size; // size not including terminating null
            }
        }
    }
    return 0;
}

quint8 CSVEDirect::getCommand(ved_t* vedata)
{
    return vedata->data[0];
}

quint16 CSVEDirect::getId(ved_t* vedata)
{
    return (((quint16) vedata->data[2]) << 8) + //
           (quint16) vedata->data[1];
}

quint8 CSVEDirect::getFlags(ved_t* vedata)
{
    return vedata->data[3];
}

quint8 CSVEDirect::getU8(ved_t* vedata)
{
    return (vedata->data[4] & 0xff);
}

quint16 CSVEDirect::getU16(ved_t* vedata)
{
    return static_cast<quint16>(
       ((vedata->data[5] << 8) | //
        vedata->data[4])
       & 0xffff);
}

quint32 CSVEDirect::getU32(ved_t* vedata)
{
    return static_cast<quint32>(
       ((vedata->data[7] << 24) | //
        (vedata->data[6] << 16) | //
        (vedata->data[5] << 8) |  //
        vedata->data[4])
       & 0xffffffff);
}

quint8 CSVEDirect::readU8(ved_t* vedata, uint* poffset)
{
    if (!poffset || !(*poffset)) {
        return getU8(vedata);
    }
    if ((*poffset) > vedata->size) {
        return 0;
    }
    return static_cast<quint8>(vedata->data[(*poffset)]);
}

quint16 CSVEDirect::readU16(ved_t* vedata, uint* poffset)
{
    if (!poffset || !(*poffset)) {
        return getU16(vedata);
    }
    if ((*poffset) + 1 > vedata->size) {
        return 0;
    }
    return static_cast<quint16>(
       ((vedata->data[(*poffset) + 1] << 8) | //
        vedata->data[(*poffset)])
       & 0xffff);
}

quint32 CSVEDirect::readU32(ved_t* vedata, uint* poffset)
{
    if (!poffset || !(*poffset)) {
        return getU32(vedata);
    }
    if ((*poffset) + 3 > vedata->size) {
        return 0;
    }
    return static_cast<quint32>(
       ((vedata->data[(*poffset) + 3] << 24) | //
        (vedata->data[(*poffset) + 2] << 16) | //
        (vedata->data[(*poffset) + 1] << 8) |  //
        vedata->data[(*poffset)])
       & 0xffffffff);
}

void CSVEDirect::setCommand(ved_t* vedata, quint8 command)
{
    vedata->data[0] = command;
    vedata->size = 1;
}

void CSVEDirect::setId(ved_t* vedata, quint16 id)
{
    vedata->data[1] = (id & 0x00ff);
    vedata->data[2] = ((id >> 8) & 0x00ff);
    vedata->size = 3;
}

void CSVEDirect::setFlags(ved_t* vedata, quint8 flags)
{
    vedata->data[3] = flags;
    vedata->size = 4;
}

void CSVEDirect::setString(ved_t* vedata, const char* pc, size_t size)
{
    vedata->size = 4;
    for (size_t i = 0; i < size && i < frameSize(2) && pc[i] != 0; i++) {
        vedata->data[i + 4] = pc[i];
        vedata->size++;
    }
}

void CSVEDirect::setU8(ved_t* vedata, quint8 value)
{
    vedata->data[4] = value;
    vedata->size = 5;
}

void CSVEDirect::setU16(ved_t* vedata, quint16 value)
{
    vedata->data[4] = (value & 0xff);
    vedata->data[5] = ((value >> 8) & 0xff);
    vedata->size = 6;
}

void CSVEDirect::setU32(ved_t* vedata, quint32 value)
{
    vedata->data[4] = (value & 0xff);
    vedata->data[5] = ((value >> 8) & 0xff);
    vedata->data[6] = ((value >> 16) & 0xff);
    vedata->data[7] = ((value >> 24) & 0xff);
    vedata->size = 8;
}

bool CSVEDirect::addU8(ved_t* vedata, quint8 value)
{
    /* header bytes must be set before */
    if (vedata->size > 3 && (vedata->size + 1) < frameSize(2)) {
        quint16 offset = vedata->size;
        vedata->data[offset++] = value;
        vedata->size = offset;
        return true;
    }
    return false;
}

bool CSVEDirect::addU16(ved_t* vedata, quint16 value)
{
    /* header bytes must be set before */
    if (vedata->size > 3 && (vedata->size + 2) < frameSize(2)) {
        quint16 offset = vedata->size;
        vedata->data[offset++] = (value & 0xff);
        vedata->data[offset++] = ((value >> 8) & 0xff);
        vedata->size = offset;
        return true;
    }
    return false;
}

bool CSVEDirect::addU32(ved_t* vedata, quint32 value)
{
    /* header bytes must be set before */
    if (vedata->size > 3 && (vedata->size + 4) < frameSize(2)) {
        quint16 offset = vedata->size;
        vedata->data[offset++] = (value & 0xff);
        vedata->data[offset++] = ((value >> 8) & 0xff);
        vedata->data[offset++] = ((value >> 16) & 0xff);
        vedata->data[offset++] = ((value >> 24) & 0xff);
        vedata->size = offset;
        return true;
    }
    return false;
}

bool CSVEDirect::addString(ved_t* vedata, const char* pc, size_t size)
{
    if (vedata->size > 3 && (vedata->size + size) < frameSize(2)) {
        quint16 offset = vedata->size;
        for (size_t i = 0; i < size && offset < frameSize(2) && pc[i] != 0; i++, offset++) {
            vedata->data[i + offset] = pc[i];
        }
        vedata->size = offset;
        return true;
    }
    return false;
}

/* ---------------------------------------------------------------
 * VE.Direct Parser
 * --------------------------------------------------------------- */

CSVeParser::CSVeParser(QObject* parent)
    : CSVEDirect(parent)
    , m_stateFunc(nullptr)
    , m_valueBuffer()
    , m_labelBuffer()
{
}

void CSVeParser::setUnknownCmd(ved_t* ved, quint8 command)
{
    setCommand(ved, VED_RESP_UNKNOWN);
    setId(ved, command);
}

void CSVeParser::setUnknownId(ved_t* ved, quint8 command, quint8 id)
{
    setCommand(ved, command);
    setId(ved, id);
    setFlags(ved, VED_FLAG_UNK_ID);
}

QByteArray CSVeParser::toCmdStr(quint8 cmd) const
{
    switch (cmd) {
        case VED_CMD_PING:
            return "CMD_PING";
        case VED_CMD_GET_APPVER:
            return "CMD_APPVER";
        case VED_CMD_GET_PRODUCT_ID:
            return "CMD_PRODUCT_ID";
        case VED_CMD_PING_RESPONSE:
            return "CMD_PING_RESP";
        case VED_CMD_GET:
            return "CMD_GET";
        case VED_CMD_SET:
            return "CMD_SET";
        case VED_CMD_ASYNC:
            return "CMD_ASYNC";
        case VED_CMD_RESTART:
            return "CMD_RESTART";
    }
    return "CMD_UNKNOWN";
}

void CSVeParser::handle(int c)
{
    switch (c) {
        case ':': {
            m_stateFunc = &CSVeParser::vedRecordHex;
            m_valueBuffer = QByteArray();
            m_labelBuffer = "VE.HEX";
            break;
        }
        case '\t': {
            /* done reading the label, switch reading value */
            if (m_stateFunc == &CSVeParser::vedRecordName) {
                m_stateFunc = &CSVeParser::vedRecordTabChar;
            }
            break;
        }
        case 0x0a:
        case 0x0d: {
            /* done reading the VE.HEX record, completion */
            if (m_stateFunc == &CSVeParser::vedRecordHex) {
                if (!m_valueBuffer.isEmpty()) {
                    parseHexFrame(m_valueBuffer);
                    vedRecordBegin(c);
                    return;
                }
            }
            /* done reading the VE.TEXT record, completion */
            else if (m_stateFunc == &CSVeParser::vedRecordValue) {
                if (!m_labelBuffer.isEmpty() && !m_valueBuffer.isEmpty()) {
                    m_stateFunc = &CSVeParser::vedRecordComplete;
                }
            }
            /* start */
            else {
                m_stateFunc = &CSVeParser::vedRecordBegin;
            }
            break;
        }
    }

    if (m_stateFunc) {
        (this->*(m_stateFunc))(c);
    }

    /* DON'T echo received character if VE.HEX record */
    if (m_stateFunc != &CSVeParser::vedRecordHex) {
        emit echoInbound(c);
    }
}

void CSVeParser::vedRecordBegin(char)
{
    m_stateFunc = &CSVeParser::vedRecordName;
    m_labelBuffer = QByteArray();
    m_valueBuffer = QByteArray();
}

void CSVeParser::vedRecordName(char c)
{
    static const QString CHARS = //
       "abcdefghijklmnopqrstuvwxyz"
       "ABCDEFGHIJKLMNOPQRSTUVWXYZ"
       "1234567890#:";

    // read too much already
    if (m_labelBuffer.count() > VE_MAX_LABEL_LENGTH) {
        vedErrorOccured(tr("Label exceeds maximum of %1 bytes.").arg(VE_MAX_LABEL_LENGTH));
        return;
    }

    // process the next character of the label
    if (!CHARS.contains(c)) {
        vedErrorOccured(tr("Invalid character: '%1'").arg(c));
        return;
    }

    m_labelBuffer.append(c);
}

void CSVeParser::vedRecordTabChar(char c)
{
    if (c == '\t') {
        m_stateFunc = &CSVeParser::vedRecordValue;
        m_valueBuffer = QByteArray();
    }
}

void CSVeParser::vedRecordValue(char c)
{
    m_valueBuffer.append(c);
}

void CSVeParser::vedRecordHex(char c)
{
    static const QString CHARS = //
       "1234567890ABCDEFabcdef:";

    /* add only HEX chars */
    if (CHARS.contains(c)) {
        m_valueBuffer.append(c);
    }
}

void CSVeParser::vedRecordComplete(char c)
{
    QString field = tr(m_labelBuffer).toUpper().trimmed();

    if (field.isEmpty()) {
        qWarning() << "[VE.Direct] Field name is emptry. Skipped!";
        vedRecordBegin(c);
        return;
    }

    /* set data field and parse */
    emit vedTextField(field, m_valueBuffer);

    /* reset to start */
    vedRecordBegin(c);
}

inline void CSVeParser::vedErrorOccured(const QString& reason)
{
    emit errorOccured("[VE.Direct] Error: " + reason.toUtf8());
    m_stateFunc = &CSVeParser::vedRecordBegin; /* restart */
}

/*Error responses:
 *  :4AAAA FD -> Invalid frame (checksum wrong)
 *  :30200 50 -> Unsupported command
 */
inline void CSVeParser::parseHexFrame(const QByteArray& hex)
{
    if (hex.isEmpty()) {
        return;
    }

    QByteArray data(hex + "\n");
    bool checkok = false;
    ved_t ve_recv = {};

    /* decode VE.Hex frame */
    for (int i = 0; i < data.count(); i++) {
        /* returns size if crc ok */
        if ((checkok = (deframe(&ve_recv, data[i]) > 0))) {
            break; /* chksum == ok done */
        }
    }
    if (!checkok) {
        vedErrorOccured("VE.HEX: Invalid hex frame.");
        return;
    }

    /* get command code */
    quint8 command = getCommand(&ve_recv);
    quint8 flags = 0;
    quint16 regid = 0;

    /* prepare VE.Direct output */
    ved_t ve_out = {};
    setCommand(&ve_out, command);

    /* determine id (register) and command flags */
    if (
       command == VED_CMD_GET ||         //
       command == VED_CMD_SET ||         //
       command == VED_CMD_ASYNC ||       //
       command == VED_CMD_PING_RESPONSE) //
    {
        flags = getFlags(&ve_recv);
        regid = getId(&ve_recv);

        /* override:
         * response command code on ASYNC is DONE */
        if (command == VED_CMD_ASYNC) {
            setCommand(&ve_out, VED_RESP_DONE);
        }

        /* response received id */
        setId(&ve_out, regid);

        /* response flags only available on GET/SET/ASYNC */
        if (
           command == VED_CMD_GET || //
           command == VED_CMD_SET || //
           command == VED_CMD_ASYNC) {
            setFlags(&ve_out, 0);
        }
    }

    emit vedHexFrame({
       .command = command, //
       .regid = regid,
       .flags = flags,
       .ve_in = ve_recv,
       .ve_out = ve_out,
       .source = hex,
    });
}
