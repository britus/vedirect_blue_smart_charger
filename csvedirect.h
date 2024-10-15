/*********************************************************************
 * Copyright (c) 2021 Strathbogie Brewing Company
 * Copyright EoF Software Labs. All Rights Reserved.
 * Copyright EoF Software Labs Authors.
 * Written by B. Eschrich (bjoern.eschrich@gmail.com)
 * SPDX-License-Identifier: MIT License
 **********************************************************************/
#pragma once
#include <QObject>

/* VE.Direct VE.HEX command code in request */
#define VED_CMD_PING           0x01
#define VED_CMD_GET_APPVER     0x03
#define VED_CMD_GET_PRODUCT_ID 0x04
#define VED_CMD_PING_RESPONSE  0x05
#define VED_CMD_RESTART        0x06
#define VED_CMD_GET            0x07
#define VED_CMD_SET            0x08
#define VED_CMD_ASYNC          0x0A

/* VE.Direct VE.HEX command code in response */
#define VED_RESP_DONE          0x01
#define VED_RESP_UNKNOWN       0x03
#define VED_RESP_ERROR         0x04
#define VED_RESP_PING          VED_CMD_PING_RESPONSE
#define VED_RESP_GET           VED_CMD_GET
#define VED_RESP_SET           VED_CMD_SET
#define VED_FLAG_UNK_ID        0x01 // Unknown Id
#define VED_FLAG_NOT_SUPPORTED 0x02 // Not supported
#define VED_FLAG_PARAM_ERROR   0x04 // Parameter Error

/**
 * @brief VE.HEX en-/decoder
 *
 * VE.Direct connection is based on a simple TTL UART
 * communication port. GND/Vdd (3.3V) is used to power up
 * isolated UART at Victron VE.Direct device (i.e. Cerbo GX).
 */
class CSVEDirect: public QObject
{
    Q_OBJECT

public:
    /* VE.HEX frame buffer size */
    static const quint16 FRAME_BUFF_SIZE = 512;

    typedef struct {
        quint16 size;
        quint8 data[FRAME_BUFF_SIZE];
    } ved_t;

    /**
     * @brief CSVEDirect
     * @param parent
     */
    explicit CSVEDirect(QObject* parent = nullptr);

    /* ------------------------------------------------------
     * VE.Direct VE.HEX Encoder/Decoder
     * ------------------------------------------------------ */

    /**
     * @brief enframe Encode to VE.HEX format
     * @param vedata
     * @return
     */
    static quint16 enframe(ved_t* vedata);
    /**
     * @brief deframe Decode VE.HEX format
     * @param vedata
     * @param inByte
     * @return
     */
    static quint16 deframe(ved_t* vedata, char inByte);
    /**
     * @brief getCommand
     * @param vedata
     * @return
     */
    static quint8 getCommand(ved_t* vedata);
    /**
     * @brief getId
     * @param vedata
     * @return
     */
    static quint16 getId(ved_t* vedata);
    /**
     * @brief getFlags
     * @param vedata
     * @return
     */
    static quint8 getFlags(ved_t* vedata);
    /**
     * @brief getU8
     * @param vedata
     * @return
     */
    static quint8 getU8(ved_t* vedata);
    static quint8 readU8(ved_t* vedata, uint* poffset);
    /**
     * @brief getU16
     * @param vedata
     * @return
     */
    static quint16 getU16(ved_t* vedata);
    static quint16 readU16(ved_t* vedata, uint* poffset);
    /**
     * @brief getU32
     * @param vedata
     * @return
     */
    static quint32 getU32(ved_t* vedata);
    static quint32 readU32(ved_t* vedata, uint* poffset);
    /**
     * @brief setCommand
     * @param vedata
     * @param command
     */
    static void setCommand(ved_t* vedata, quint8 command);
    /**
     * @brief setId
     * @param vedata
     * @param id
     */
    static void setId(ved_t* vedata, quint16 id);
    /**
     * @brief setFlags
     * @param vedata
     * @param flags
     */
    static void setFlags(ved_t* vedata, quint8 flags);
    /**
     * @brief setString
     * @param vedata
     * @param pc
     * @param size
     */
    static void setString(ved_t* vedata, const char* pc, size_t size);
    /**
     * @brief setU8
     * @param vedata
     * @param value
     */
    static void setU8(ved_t* vedata, quint8 value);
    /**
     * @brief setU16
     * @param vedata
     * @param value
     */
    static void setU16(ved_t* vedata, quint16 value);
    /**
     * @brief setU32
     * @param vedata
     * @param value
     */
    static void setU32(ved_t* vedata, quint32 value);
    /**
     * @brief addU8
     * @param vedata
     * @param value
     * @return
     */
    static bool addU8(ved_t* vedata, quint8 value);
    /**
     * @brief addU16
     * @param vedata
     * @param value
     * @return
     */
    static bool addU16(ved_t* vedata, quint16 value);
    /**
     * @brief addU32
     * @param vedata
     * @param value
     * @return
     */
    static bool addU32(ved_t* vedata, quint32 value);
    /**
     * @brief addString
     * @param vedata
     * @param pc
     * @param size
     * @return
     */
    static bool addString(ved_t* vedata, const char* pc, size_t size);

private:
    static inline quint16 frameSize(int reserve)
    {
        return (
           FRAME_BUFF_SIZE - //
           (reserve < FRAME_BUFF_SIZE ? reserve : 0));
    }
};

/**
 * @brief Event driven VE.TEXT / VE.HEX parser.
 */
class CSVeParser: public CSVEDirect
{
    Q_OBJECT

public:
    explicit CSVeParser(QObject* parent = nullptr);

    typedef struct {
        quint8 command;
        quint16 regid;
        quint8 flags;
        CSVEDirect::ved_t ve_in;
        CSVEDirect::ved_t ve_out;
        QByteArray source;
    } TVeHexFrame;

    void handle(int c);
    void setUnknownCmd(ved_t* ved, quint8 command);
    void setUnknownId(ved_t* ved, quint8 command, quint8 id);
    QByteArray toCmdStr(quint8 cmd) const;

signals:
    void vedHexFrame(const CSVeParser::TVeHexFrame& frame);
    void vedTextField(const QString& field, const QByteArray& value);
    void echoInbound(const char c);
    void errorOccured(const QByteArray& messge);

private:
    static const int VE_MAX_LABEL_LENGTH = 9;
    static const int VE_MAX_VALUE_LENGTH = 33;

    void (CSVeParser::*m_stateFunc)(char);

    QByteArray m_valueBuffer;
    QByteArray m_labelBuffer;

private:
    /* state callback functions */
    void vedRecordBegin(char c);
    void vedRecordName(char c);
    void vedRecordTabChar(char c);
    void vedRecordValue(char c);
    void vedRecordHex(char c);
    void vedRecordComplete(char c);

private:
    inline void vedErrorOccured(const QString& reason);
    inline void parseHexFrame(const QByteArray& hex);
};
Q_DECLARE_METATYPE(CSVeParser::TVeHexFrame)
