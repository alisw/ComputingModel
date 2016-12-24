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
    Q_ENUMS (TierCat)

public:

    enum TierCat {kT0, kT1, kT2, kUnknown};
    explicit Tier(QObject *parent = 0);
    Tier (QString name, TierCat cat, Resources &res, QObject *parent = 0);

    void    addCEs(const QList<QString> &list);
    void    addSEs(const QList<QString> &list);
    void    addUsedResources(QString &month, Resources *res);
    TierCat category() const { return mTierCategory; }
    void    clearUsed(const QString &month);
    double  getCPU() const   { return mResources.getCPU(); }
    double  getDisk() const  { return mResources.getDisk(); }
    double  getTape() const  { return mResources.getTape(); }
    double  getUsedCPU(const QString &month)  const  { return mUsed[month].getCPU(); }
    double  getUsedDisk(const QString &month) const  { return mUsed[month].getDisk(); }
    double  getUsedTape(const QString &month) const  { return mUsed[month].getTape(); }
    QString getWLCGAlias() const { return mWLCGAliasName; }
    QString getWLCGName() const  { return mWLCGName; }
    QString list() const;
    void    setAlias(QString alias) { mWLCGAliasName = alias; }
    double  usedCPU(const QString &m) const { return mUsed[m].getCPU(); }
    double  usedDisk(const QString &m) const { return mUsed[m].getDisk(); }
    double  usedTape(const QString &m) const { return mUsed[m].getTape(); }

private:
    QList<QString>             mMLCENames;    // The CE name in MonALIsa
    QList<QString>             mMLSENames;    // The SE name in MonALIsa
    Resources                  mResources;    // The resources in this site (CPU, disk, tape)
    TierCat                    mTierCategory; // The Tier category 0, 1, or 2
    QString                    mWLCGAliasName;// An alias name in WLCG
    QString                    mWLCGName;     // The name in WLCG
    QMap<QString, Resources>   mUsed;         // The resources in this site (CPU, disk, tape) per month
};

#endif // TIER_H
