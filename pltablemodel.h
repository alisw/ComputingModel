// Author: Yves Schutz décembre 2016
//
// A table plot to display a 2D plot with xy tables
#ifndef PLTABLEMODEL_H
#define PLTABLEMODEL_H

#include <QAbstractTableModel>
#include <QList>
#include <QRect>

class PlTableModel : public QAbstractTableModel
{
public:
    PlTableModel(QObject *parent = 0);

    void          addData(const QString &year, QVector<double> *vec);
    void          addData(const QDateTime &date, QVector<double> *vec);
    void          addData(QVector<double> *vec);
    void          addMapping(QString color, QRect area);
    void          clearMapping() { mMapping.clear(); }
    int           columnCount(const QModelIndex &parent = QModelIndex()) const;
    QVariant      data(const QModelIndex &index, int role) const;
    double        findMax() const ;
    Qt::ItemFlags flags(const QModelIndex &index) const;
    QVariant      headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    bool          isEmpty() const;
    int           rowCount(const QModelIndex &parent = QModelIndex()) const;
    bool          setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    void          setColRow(int col, int row) { mColums = col; mRows = row; }
    void          setHeader(QVector<QString> headers);

private:
    qint32                  mColums;  // number of colums
    QList<QVector<double>*> mData;    // the data to be plotted
    QVector<QString>        mHeader;  // the horizontal headers of the table
    QHash<QString, QRect>   mMapping; // not used
    qint32                  mRows;    // number of rows
};

#endif // PLTABLEMODEL_H
