#pragma once
#include <QAbstractTableModel>

class CSChargerDataModel: public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit CSChargerDataModel(QObject* parent = nullptr);

    // Basic functionality:
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    int rowCount(const QModelIndex& parent = QModelIndex()) const override;

    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;
    quint16 toRegId(const QModelIndex& index) const;

    void updateRegister(quint16 regid, const QPair<float, QVariant>& p);
    void beginUpdate();
    void endUpdate();

private:
    QMap<quint16, QPair<QString, QPair<float, QVariant>>> m_rowData;
};
