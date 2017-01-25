// computing center description
// Y. Schutz December 2016

#ifndef TIER_H
#define TIER_H

#include <QMap>
#include <QObject>

#include "resources.h"

class Tier : public QObject
{
    Q_OBJECT

public:

    enum TierCat {kT0, kT1, kT2, kTOTS, kUnknown};
    Q_ENUM (TierCat)

    explicit Tier(QObject *parent = 0);
    Tier (QString name, TierCat cat, Resources &res, QObject *parent = 0);

    void    addCE(const QString &ce) { mMLCENames.append(ce); }
    void    addSE(const QString &se) { mMLSENames.append(se); }
    void    addCEs(const QList<QString> &list);
    void    addSEs(const QList<QString> &list);
    TierCat category() const { return mTierCategory; }
    void    clearUsed(const QString &month);
    qint32  countWLCGAlias() const { return mWLCGAliasNames.size(); }
    bool    findCE(const QString &ce);
    bool    findSE(const QString &se);
    double  getCPU() const   { return mResources.getCPU(); }
    double  getDisk() const  { return mResources.getDisk(); }
    double  getTape() const  { return mResources.getTape(); }
    double  getUsedCPU(const QString &month)  const  { return mUsed[month].getCPU(); }
    double  getUsedDisk(const QString &month) const  { return mUsed[month].getDisk(); }
    double  getUsedTape(const QString &month) const  { return mUsed[month].getTape(); }
    QString getWLCGAlias(qint32 index) const { return mWLCGAliasNames.at(index); }
    QString getWLCGName() const  { return mWLCGName; }
    QString list() const;
    void    addAlias(QString alias) { mWLCGAliasNames.append(alias); }
    void    setUsedCPU(QString &month, double cpu);
    double  usedCPU(const QString &m) const { return mUsed[m].getCPU(); }
    double  usedDisk(const QString &m) const { return mUsed[m].getDisk(); }
    double  usedTape(const QString &m) const { return mUsed[m].getTape(); }

private:
    QList<QString>             mMLCENames;     // The CE name in MonALIsa
    QList<QString>             mMLSENames;     // The SE name in MonALIsa
    Resources                  mResources;     // The resources in this site (CPU, disk, tape)
    TierCat                    mTierCategory;  // The Tier category 0, 1, or 2
    QList<QString>             mWLCGAliasNames;// Aliases name in WLCG
    QString                    mWLCGName;      // The name in WLCG
    QMap<QString, Resources>   mUsed;          // The resources in this site (CPU, disk, tape) per month
};

#endif // TIER_H
