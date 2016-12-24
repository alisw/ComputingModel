// Funding Agency
// Y. Schutz November 2016
#
#ifndef FUNDINGAGENCY_H
#define FUNDINGAGENCY_H

#include <QObject>
#include "resources.h"

class Tier;

class FundingAgency : public QObject
{
    Q_OBJECT
    Q_ENUMS (Status)
public:
    enum Status {kMS, kNMS};

    explicit FundingAgency(QObject *parent = 0);
    FundingAgency(QString name, qint32 status, qint32 mopay, QObject *parent = 0);
    FundingAgency(QString name, qint32 status, QObject *parent = 0);
    ~FundingAgency() {;}

    void       addFA(FundingAgency *fa);
    void       addTier(Tier *site);
    void       clear();
    void       clearUsed(const QString &month);
    double     contrib() const        { return mContrib; }
    double     contribT() const       { return mContribT; }
    double     getPledgedCPU() const  { return mPledgedResources.getCPU(); }
    double     getPledgedDisk() const { return mPledgedResources.getDisk(); }
    double     getPledgedTape() const { return mPledgedResources.getTape(); }
    double     getRequiredCPU() const  { return mRequiredResources.getCPU(); }
    double     getRequiredDisk() const { return mRequiredResources.getDisk(); }
    double     getRequiredTape() const { return mRequiredResources.getTape(); }
    Resources* getUsed(const QString &month) const;
    bool       hasT1() const;
    QString    name() const           { return objectName(); }
    qint32     payers() const         { return mMandOPayers; }
    Tier*      search(const QString &n, bool aliasing = false) const;
    void       setContrib(double val) { mContrib = val; }
    void       setContribT(double val);
    void       setRequired(double cpu, double disk, double tape);
    QString    status() const         { if (mStatus == kMS) return "MS"; else return "NMS"; }

    QString list() const;

private:
    QList<Tier*> tiers()     { return mTiers; }
    Resources    &pledged()  { return mPledgedResources; }
    Resources    &required() { return mRequiredResources; }


    double       mContrib;             // required contribution, fraction of total required in %
    double       mContribT;            // required contribution for tape, T1 only
    qint32       mMandOPayers;         // number of M&O-A payers
    Resources    mPledgedResources;    // required resources
    Resources    mRequiredResources;   // required resources
    QList<Tier*> mTiers;               // the list of sites for this FA
    qint32       mStatus;              // member state or non member state
};

#endif // FUNDINGAGENCY_H
