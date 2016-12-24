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
    enum Elements {kFA, kSE, kCEML, kCEWLCG};
    static Naming *instance();

    const QList<QString> find(const QString &faName, Elements el);

private:
    explicit Naming(QObject *parent = 0);
    ~Naming();
    Naming(const Naming&);

    static Naming*              mInstance; // the unique instance of the object
    QMultiHash<QString,QString> mDict;     // key is the FA name  and values are the various naming
};

#endif // NAMING_H
