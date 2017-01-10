// Class defining the various namings for CE, SE in ML and WLCG and their FA affiliation
// Y. Schutz December 2016

#include<QDebug>
#include <QFile>

#include "naming.h"

Naming* Naming::mInstance = Q_NULLPTR;

//===========================================================================
Naming::Naming(QObject *parent) : QObject(parent)
{
    // ctor
    // read the namings from a csv file
    // FORMAT: FA; SE; CE in ML; CE in WLCG

    mDict.clear();
    QString fileName = QString(":/data/NamingDictionary.csv");
    QFile csvFile(fileName);
    if (!csvFile.open(QIODevice::ReadOnly)) {
        qWarning() << QString("File %1 cannot be opened").arg(fileName);
        exit(1);
    }

    // read the header
    QString line = csvFile.readLine();

    while (!csvFile.atEnd()) {
        line = csvFile.readLine();
        QStringList strlist = line.split(';');
        QString fa     = strlist[kFA];
        QString se     = strlist[kSE];
        QString ceML   = strlist[kCEML];
        QString ceWLCG = strlist[kCEWLCG];
        ceWLCG.remove("\r\n");

        QVector<QString> *vec = new QVector<QString>(4);
        vec->insert(kFA,     fa);
        vec->insert(kSE,     se);
        vec->insert(kCEML,   ceML);
        vec->insert(kCEWLCG, ceWLCG);

        mDict.append(vec);
    }
}

//===========================================================================
Naming::~Naming()
{
    // dtor, deletes all elements of the hash
    qDeleteAll(mDict.begin(), mDict.end());
    mDict.clear();
}

//===========================================================================
Naming::Naming(const Naming &)
{
    //cpy ctor
}

//===========================================================================
Naming* Naming::instance()
{
    if (!mInstance)
        mInstance = new Naming();
    return mInstance;
}

//===========================================================================
const QList<QString> Naming::find(const QString &faName, QString wlcg, Elements el)
{
  // retrieve the ML CE/SE element name for Funding Agency faName

    QString fa = faName;
    fa.remove("*");
    QList<QString> rv;
    rv.clear();

    for (QVector<QString> *vect : mDict) {
        if (vect->at(kFA) == fa && vect->at(kCEWLCG) == wlcg)
            if (!vect->at(el).isEmpty())
                rv.append(vect->at(el));
    }

    return rv;
}
