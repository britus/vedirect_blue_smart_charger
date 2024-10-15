#include <cschargerdatamodel.h>

#ifndef BIT
#define BIT(x) (1 << (x))
#endif

CSChargerDataModel::CSChargerDataModel(QObject* parent)
    : QAbstractTableModel(parent)
{
    m_rowData[0x0001] = {"Product Id.", QPair<float, QVariant>(1, tr(""))};
    m_rowData[0x0002] = {"Firmware Rel.", QPair<float, QVariant>(1, tr(""))};
    m_rowData[0x0003] = {"Serial Number", QPair<float, QVariant>(1, tr(""))};
    m_rowData[0x0004] = {"Tag", QPair<float, QVariant>(1, tr(""))};
    m_rowData[0x0104] = {"Group Id", QPair<float, QVariant>(1, 0)};
    m_rowData[0x010C] = {"Model", QPair<float, QVariant>(1, tr(""))};
    m_rowData[0x010E] = {"Identify Mode", QPair<float, QVariant>(1, 0)};
    m_rowData[0x0140] = {"Capabilities 1", QPair<float, QVariant>(1, 0)};
    m_rowData[0x0143] = {"Capabilities 4", QPair<float, QVariant>(1, 0)};
    m_rowData[0x0201] = {"Device State", QPair<float, QVariant>(1, 0)};
    m_rowData[0x0206] = {"Device Function", QPair<float, QVariant>(1, 0)};
    m_rowData[0x0207] = {"Device Off Reason", QPair<float, QVariant>(1, 0)};
    m_rowData[0x1042] = {"History cumulative 1", QPair<float, QVariant>(1, tr(""))};
    m_rowData[0x1043] = {"History cumulative 2", QPair<float, QVariant>(1, tr(""))};
    m_rowData[0x1070] = {"History Cycle Record", QPair<float, QVariant>(1, tr(""))};
    m_rowData[0x2001] = {"Charger Voltage Set-Point", QPair<float, QVariant>(1, 0)};
    m_rowData[0x2002] = {"VE_REG_LINK_VSENSE", QPair<float, QVariant>(1, 0)};
    m_rowData[0x2003] = {"VE_REG_LINK_TSENSE", QPair<float, QVariant>(1, 0)};
    m_rowData[0x2007] = {"Charger Elapsed Time", QPair<float, QVariant>(1, 0)};
    m_rowData[0x2008] = {"Charger Absorption Time", QPair<float, QVariant>(1, 0)};
    m_rowData[0x2009] = {"Charger Error Code", QPair<float, QVariant>(1, 0)};
    m_rowData[0x200A] = {"System Bat. Current (A)", QPair<float, QVariant>(1, 0)};
    m_rowData[0x200C] = {"Link Work State", QPair<float, QVariant>(1, 0)};
    m_rowData[0x200D] = {"Charger Network Info", QPair<float, QVariant>(1, 0)};
    m_rowData[0x200E] = {"Charger Network Mode", QPair<float, QVariant>(1, 0)};
    m_rowData[0x200F] = {"Network Status", QPair<float, QVariant>(1, 0)};
    m_rowData[0x2013] = {"System Timer", QPair<float, QVariant>(1, 0)};
    m_rowData[0x2042] = {"Charger Absorption End Time", QPair<float, QVariant>(1, 0)};
    m_rowData[0xE001] = {"Low Current Mode", QPair<float, QVariant>(1, 0)};
    m_rowData[0xE002] = {"Low Current Time Left", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEC15] = {"Number of REG's broadcasting", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEC41] = {"Date/time of last change", QPair<float, QVariant>(1, 0)};
    m_rowData[0xED2E] = {"Re-bulk Offset Voltage-Level", QPair<float, QVariant>(1, 0)};
    m_rowData[0xED8D] = {"Actual Charge Voltage C1 (V)", QPair<float, QVariant>(1, 0)};
    m_rowData[0xED8F] = {"Actual Charge Current C1 (A)", QPair<float, QVariant>(1, 0)};
    m_rowData[0xED8C] = {"DC Channel 1 Current", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDC8] = {"Minimum Current (A)", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDD5] = {"Actual Charge Voltage (V)", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDD7] = {"Actual Charge Current (A)", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDDE] = {"Number of Outputs", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDDB] = {"Dev. Temperature (°C)", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDE0] = {"Stop below temperature", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDE1] = {"Re-bulk Current Level (A)", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDE2] = {"Re-bulk Voltage Level (V)", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDE3] = {"Equalisation Time Duration (Sec)", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDE4] = {"Equalisation Cur. Percentage (%)", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDE5] = {"Equalisation Auto Stop", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDE6] = {"Low-temp Charge Current", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDE7] = {"Tail Current", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDE9] = {"Output Voltage (V)", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDF0] = {"Maximum Current (A)", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDF1] = {"Charging Preset", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDF2] = {"Temperature Compensation", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDF4] = {"Equalization Voltage (V)", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDF5] = {"Storage Voltage (V)", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDF6] = {"Float Voltage (V)", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDF7] = {"Absorption Voltage (V)", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDF8] = {"Repeated Abs. Interval", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDF9] = {"Repeated Abs. Time", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDFA] = {"Maximum Float Time", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDFB] = {"Maximum Abs. Time", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDFC] = {"Bulk Time Limit", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDFD] = {"Traction Curve/Auto Equ.", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDFE] = {"Adaptive Mode", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEDFF] = {"Battery Safe Mode", QPair<float, QVariant>(1, 0)};
    m_rowData[0xEE16] = {"Battery Used VSense (V)", QPair<float, QVariant>(1, 0)};
}

int CSChargerDataModel::columnCount(const QModelIndex&) const
{
    return 2;
}

int CSChargerDataModel::rowCount(const QModelIndex&) const
{
    return m_rowData.count();
}

quint16 CSChargerDataModel::toRegId(const QModelIndex& index) const
{
    QList<quint16> registers = m_rowData.keys();
    if (index.row() >= registers.count()) {
        return 0xffff;
    }
    return registers[index.row()];
}

QVariant CSChargerDataModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation != Qt::Orientation::Horizontal)
        return QVariant();
    if (role != Qt::DisplayRole)
        return QVariant();

    switch (section) {
        case 0:
            return tr("Register");
        case 1:
            return tr("Value");
    }

    return QVariant();
}

QVariant CSChargerDataModel::data(const QModelIndex& index, int role) const
{
    QList<quint16> registers = m_rowData.keys();
    if (index.row() >= registers.count()) {
        return QVariant();
    }

    const uint regid = registers[index.row()];
    const QString streg = tr("0x%1").arg(regid, 4, 16, QChar('0')).toUpper();
    const QPair<QString, QPair<float, QVariant>> row = m_rowData[regid];
    const QPair<float, QVariant> sv = row.second;
    switch (role) {
        case Qt::UserRole: {
            return QVariant::fromValue(sv);
            break;
        }
        case Qt::DisplayRole: {
            switch (index.column()) {
                case 0: {
                    if (row.first.isEmpty()) {
                        return QVariant::fromValue(streg);
                    }
                    return QVariant::fromValue(tr("%1: %2").arg(streg, row.first));
                }
                case 1: {
                    /* variant object */
                    if (sv.second.isNull() || !sv.second.isValid())
                        return QVariant();
                    float scale = sv.first;
                    QVariant value = sv.second;
                    QString infoStr = "";
                    switch (regid) {
                        case 0x0140: {
                            uint flags = static_cast<quint16>(value.toUInt());
                            for (quint8 i = 0; i < 31; i++) {
                                if (flags & BIT(i)) {
                                    infoStr += tr("B%1 ").arg(i);
                                }
                            }
                            return QVariant::fromValue(infoStr);
                        }
                        case 0x0143: {
                            uint flags = static_cast<quint16>(value.toUInt());
                            for (quint8 i = 0; i < 31; i++) {
                                if (flags & BIT(i)) {
                                    infoStr += tr("B%1 ").arg(i);
                                }
                            }
                            return QVariant::fromValue(infoStr);
                        }
                        case 0xEDF1: {
                            switch (static_cast<quint8>(value.toUInt())) {
                                case 0:
                                    return QVariant::fromValue(tr("Normal"));
                                case 1:
                                    return QVariant::fromValue(tr("Normal + Regenerate"));
                                case 2:
                                    return QVariant::fromValue(tr("High"));
                                case 3:
                                    return QVariant::fromValue(tr("High + Regenerate"));
                                case 4:
                                    return QVariant::fromValue(tr("Li-Ion"));
                            }
                            break;
                        }
                        case 0xEDFE: {
                            switch (static_cast<quint8>(value.toUInt())) {
                                case 0:
                                    return QVariant::fromValue(tr("Fixed"));
                                case 1:
                                    return QVariant::fromValue(tr("Adaptive"));
                            }
                            break;
                        }
                        case 0xEDFF: {
                            switch (static_cast<quint8>(value.toUInt())) {
                                case 0:
                                    return QVariant::fromValue(tr("Off"));
                                case 1:
                                    return QVariant::fromValue(tr("On"));
                            }
                            break;
                        }
                    }
                    if (value.type() == QVariant::Double) {
                        if (value.toUInt() == 0xFFFF || value.toUInt() == 0x7FFFFFFF) {
                            return QVariant::fromValue(tr("---"));
                        }
                        return QVariant::fromValue(QString::number(value.toDouble() * scale));
                    }
                    else if (value.type() == QVariant::String) {
                        return QVariant::fromValue(value.toString());
                    }
                    else if (value.type() == QVariant::ByteArray) {
                        return QVariant::fromValue(value.toString());
                    }
                    else {
                        int val = value.toInt();
                        QString str = tr("%1 (0x%2)").arg(val).arg(val, 4, 16, QChar('0'));
                        return QVariant::fromValue(str);
                    }
                }
            }
            break;
        }
    }
    return QVariant();
}

void CSChargerDataModel::beginUpdate()
{
    beginResetModel();
}

void CSChargerDataModel::endUpdate()
{
    endResetModel();
}

void CSChargerDataModel::updateRegister(quint16 regid, const QPair<float, QVariant>& p)
{
    m_rowData[regid].second = p;
}
