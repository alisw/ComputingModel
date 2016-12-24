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

        if (se != "") {
            se.prepend("xSEx");
            mDict.insert(fa, se);
        }
        if (ceML != "") {
            ceML.prepend("xCEMLx");
            mDict.insert(fa, ceML);
        }
        if (ceWLCG != "") {
            ceWLCG.prepend("xCEWLCGx");
            mDict.insert(fa, ceWLCG);
        }
    }
}

//===========================================================================
Naming::~Naming()
{
    // dtor, deletes all elements of the hash
//    qDeleteAll(dict.begin(), dict.end());

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
const QList<QString> Naming::find(const QString &faName, Elements el)
{
  // retrieve the ML CE element name for Funding Agency faName

    QString fa = faName;
    fa.remove("*");
    QList<QString> rv;
    rv.clear();
    QString search;
    switch (el) {
    case kSE:
        search = "xSEx";
        break;
    case kCEML:
        search = "xCEMLx";
        break;
    case kCEWLCG:
        search = "xCEWLCGx";
        break;
    default:
        break;
    }

    QList<QString> values = mDict.values(fa);
    for (int i = 0; i < values.size(); i++) {
        QString test = values.at(i);
        if (test.contains(search)) {
            test.remove(search);
            rv.append(test);
        }
    }

    return rv;
}
