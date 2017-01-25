// Class defining the various namings for CE, SE in ML and WLCG and their FA affiliation
// Y. Schutz December 2016
#ifndef NAMING_H
#define NAMING_H

#include <QMultiHash>
#include <QObject>

class Naming : public QObject
{
    Q_OBJECT
    Q_ENUMS (Elements)

public:    
    enum Elements {kFASHORT, kFA, kSE, kCEML, kCEWLCG};
    static Naming *instance();

    const QList<QString> find(const QString &faName, QString wlcg, Elements el);
    const QString        find(const QString &faShort);

private:
    explicit Naming(QObject *parent = 0);
    ~Naming();
    Naming(const Naming&);

    static Naming               *mInstance; // the unique instance of the object
    QList<QVector<QString>*>     mDict;     // list of (FA, SEML, CEML, SiteWLVG)
};

#endif // NAMING_H
