// Author: Yves Schutz décembre 2016
//
// A table plot to display a 2D plot with xy tables

#include <QColor>
#include <QDebug>
#include <QFont>
#include <QTime>
#include "pltablemodel.h"

//===========================================================================
PlTableModel::PlTableModel(QObject *parent) : QAbstractTableModel(parent)
{
    Q_UNUSED(parent)
}

//===========================================================================
void PlTableModel::addData(const QString &year, QVector<double> *vec)
{
    // add one row of data: year, double,....

    QDateTime date;
    date.setDate(QDate(year.toInt(), 4, 1)); // 1 April year
    qint32 size = vec->size() + 2;
    QVector<double> *dataVec = new QVector<double>(size);
    dataVec->replace(0, (double) date.toMSecsSinceEpoch());
    dataVec->replace(1, year.toDouble());
    for (qint32 index = 0; index < vec->size(); index++)
        dataVec->replace(index + 2, vec->at(index));
    mData.append(dataVec);
}

//===========================================================================
void PlTableModel::addData(const QDateTime &date, QVector<double> *vec)
{
    // add one row of data; date, double,....

    qint32 size = vec->size() + 2;
    QVector<double> *dataVec = new QVector<double>(size);
    dataVec->replace(0, (double) date.toMSecsSinceEpoch());
    dataVec->replace(1, date.date().month()+ date.date().year()/10000.);
    for (qint32 index = 0; index < vec->size(); index++)
        dataVec->replace(index + 2, vec->at(index));
    mData.append(dataVec);

}

//===========================================================================
void PlTableModel::addData(QVector<double> *vec)
{
    // add one row of data: double, ....
    mData.append(vec);
    qDebug() << Q_FUNC_INFO << mData.at(0)->at(0);
}

//===========================================================================
void PlTableModel::addMapping(QString color, QRect area)
{
    mMapping.insertMulti(color, area);
}

//===========================================================================
int PlTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return mColums;
}

//===========================================================================
QVariant PlTableModel::data(const QModelIndex &index, int role) const
{
    switch (role) {
    case Qt::DisplayRole:
    {
        QVariant rv;
        if (index.column() > 1) {
            double value = mData[index.row()]->at(index.column());
            if (value == 0 && index.row() == rowCount() - 1) { // just in case the last year data are not available
                value = mData[index.row()-1]->at(index.column());
            }
            rv = QString::number(value, 'f', 1);
        } else
            rv = mData[index.row()]->at(index.column());
        return rv;
        break;
    }
    case Qt::EditRole:
        return mData[index.row()]->at(index.column());
        break;
    case Qt::BackgroundRole:
//        foreach (QRect rect, mMapping) {
//            if (rect.contains(index.column(), index.row()))
//                return QColor(mMapping.key(rect));
//            else
//                return QColor(Qt::white);
//        }
        break;
    case Qt::TextColorRole:
        if (index.column() == 1)
            return QColor(QColor(Qt::black));
        else
            return QColor(Qt::darkBlue);
        break;
    case Qt::ToolTipRole:
        return "to be implemented";
        break;
    case Qt::TextAlignmentRole:
        if (index.column() == 1)
            return Qt::AlignCenter;
        else
            return Qt::AlignRight + Qt::AlignVCenter;
        break;
    default:
        return QVariant();
        break;
    }
    return QVariant();
}

//===========================================================================
double PlTableModel::findMax() const
{
    // find the largest y value

    double max = -1;
    for (QVector<double> *vec : mData) {
        for (qint32 index = 2; index < vec->size(); index++) // skip the two first columns corresponding to x axis
            if (vec->at(index) > max)
                max = vec->at(index);
    }
    return max;
}

//===========================================================================
QVariant PlTableModel::headerData(int section, Qt::Orientation orientation, int role) const
{

    switch (role) {
    case Qt::DisplayRole:
        if (orientation == Qt::Horizontal)
            return mHeader[section];
       break;
    case Qt::TextColorRole:
        return QColor(Qt::black);
        break;
    case Qt::BackgroundColorRole:
        return QColor(Qt::lightGray);
        break;
    case Qt::FontRole: {
        QFont boldFont;
        boldFont.setBold(true);
        return boldFont;
    }
        break;
    default:
        return QVariant();
        break;
    }

    return QVariant();
}

//===========================================================================
bool PlTableModel::isEmpty() const
{
    // check if there are data
    if (mData.size() > 0)
        return false;
    else
        return true;
}

//===========================================================================
int PlTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent)
    return mData.count();
}

//===========================================================================
bool PlTableModel::setData(const QModelIndex &index, const QVariant &value, int role)
{
    if (index.isValid() && role == Qt::EditRole) {
        mData[index.row()]->replace(index.column(), value.toDouble());
        emit dataChanged(index, index);
        return true;
    }
    return false;
}

//===========================================================================
void PlTableModel::setHeader(QVector<QString> headers)
{
    // the the value of the headers
    for (qint32 index = 0; index < mColums; index++)
        mHeader.append(headers[index]);
}

//===========================================================================
Qt::ItemFlags PlTableModel::flags(const QModelIndex &index) const
{
    return QAbstractItemModel::flags(index) | Qt::ItemIsEditable;

}
