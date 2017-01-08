// Funding Agency
// Y. Schutz November 2016


#include <QDateTime>
#include <QDebug>

#include "fundingagency.h"
#include "tier.h"

//===========================================================================
FundingAgency::FundingAgency(QObject *parent) : QObject(parent),
    mContrib(0.0), mContribT(0.0), mMandOPayers(0), mStatus(0)
{
    // default ctor
    setObjectName("No Name!");
    mPledgedResources.clear();
    mRequiredResources.clear();
    mTiers.clear();
}

//===========================================================================
FundingAgency::FundingAgency(QString name, qint32 status, qint32 mopay, QObject *parent) : QObject(parent),
   mContrib(0.0), mMandOPayers(mopay), mStatus(status)
{
    // ctor with initialisation
    setObjectName(name);
    mPledgedResources.clear();
    mRequiredResources.clear();
    mTiers.clear();
}

//===========================================================================
FundingAgency::FundingAgency(QString name, qint32 status, QObject *parent) : QObject(parent),
    mContrib(0.0), mContribT(0.0), mMandOPayers(0), mStatus(status)
{
    //ctor with initialisation by name
    setObjectName(name);
    mPledgedResources.clear();
    mRequiredResources.clear();
    mTiers.clear();
}

//===========================================================================
void FundingAgency::addFA(FundingAgency *fa)
{
    // add a funding agency
    if (!fa)
        return;
    mContrib += fa->contrib();
    mContribT  += fa->contribT();
    mMandOPayers += fa->payers();
    mRequiredResources.setCPU(getRequiredCPU() + fa->getRequiredCPU());
    mRequiredResources.setDisk(getRequiredDisk() + fa->getRequiredDisk());
    mRequiredResources.setTape(getRequiredTape() + fa->getRequiredTape());
    for (Tier *t : fa->tiers())
        addTier(t);
    fa->setObjectName(fa->objectName().prepend("-"));
}

//===========================================================================
void FundingAgency::addTier(Tier *site)
{
    // add a site to this Funding Agency

    mPledgedResources.setCPU(mPledgedResources.getCPU() + site->getCPU());
    mPledgedResources.setDisk(mPledgedResources.getDisk() + site->getDisk());
    mPledgedResources.setTape(mPledgedResources.getTape() + site->getTape());
    mTiers.append(site);
}

//===========================================================================
void FundingAgency::addUsedCPU(const QString &month, double cpu)
{
    // add used cpu storage from this fa as reported by ML
   if (!mUsedResourcesML[month]) {
       Resources *res = new Resources(this);
       res->setObjectName("Used Resources from ML");
       mUsedResourcesML[month] =  res;
   }
   mUsedResourcesML[month]->setCPU(mUsedResourcesML[month]->getCPU() + cpu);

}

//===========================================================================
Resources::Resources_type FundingAgency::addUsedDiskTape(const QString &month, const QString &se, double storage)
{
    // add used disk or trape storage from this fa as reported by ML
    Resources::Resources_type rv;

    if (se.contains("TAPE") || se.contains("T0ALICE") || se.contains("CASTOR2")) {
        mUsedResourcesML[month]->setTape(mUsedResourcesML[month]->getTape() + storage);
        rv = Resources::kTAPE;
    }
    else {
        mUsedResourcesML[month]->setDisk(mUsedResourcesML[month]->getDisk() + storage);
        rv = Resources::kDISK;
    }
    return rv;
}

//===========================================================================
void FundingAgency::clear()
{
    // clears everything
    mContrib = 0.0;
    mMandOPayers = 0;
    mPledgedResources.clear();
    mRequiredResources.clear();
    foreach (Resources *res, mUsedResources)
        res->clear();
}

//===========================================================================
void FundingAgency::clearUsed(const QString &month)
{
   // clears used resources during month

    for (Tier *t : mTiers)
        t->clearUsed(month);
    Resources *res = mUsedResources[month];
    res->clear();
}

//===========================================================================
void FundingAgency::computeUsedCPU(const QString &month)
{
    // calculates total used resources from this FA
    if (!mUsedResources[month]) {
        double cpu  = 0.0;
        for (Tier *t : mTiers) {
            cpu  += t->usedCPU(month);
        }
        Resources *res = new Resources(this);
        res->setObjectName("Used Resources");
        res->setCPU(cpu);
        mUsedResources[month] = res;
    }
}

//===========================================================================
double FundingAgency::getUsedCPU(const QString &month) const
{
    // retrieves total used CPU resources from this FA from WLCG
    Resources *res = mUsedResources[month];
    if (!res)
        return 0.0;
    return res->getCPU();
}

//===========================================================================
double FundingAgency::getUsedCPUML(const QString &month) const
{
    // retrieves total used CPU resources from this FA from MonALISA
    Resources *res = mUsedResourcesML[month];
    if (!res)
        return 0.0;
    return res->getCPU();
}

//===========================================================================
double FundingAgency::getUsedDiskML(const QString &month) const
{
    // retrieves total used disk resources from this FA
    Resources *res = mUsedResourcesML[month];
    if (!res)
        return 0.0;
    return res->getDisk();
}

//===========================================================================
double FundingAgency::getUsedTapeML(const QString &month) const
{
    // retrieves total used tape resources from this FA
    Resources *res = mUsedResourcesML[month];
    if (!res)
        return 0.0;
    return res->getTape();
}

//===========================================================================
bool FundingAgency::hasT1() const
{
    // checks if this funding agency has a T1

    bool rv = false;
    for (Tier* t : mTiers) {
        if (t->category() == Tier::kT1)
            rv = true;
    }
    return rv;
}

//===========================================================================
Tier *FundingAgency::search(const QString &n, bool aliasing) const
{
    // search a Tier by name

    Tier *rv = Q_NULLPTR;

    for (Tier *t : mTiers) {
        if (n == t->getWLCGName() || n == t->getWLCGAlias()) {
            rv = t;
            break;
        }        
    }
    if (!rv && aliasing) {
        for (Tier *t : mTiers) {
            if (t->category() == Tier::kT2 && t->getWLCGAlias() == "") {
                t->setAlias(n);
                rv = t;
                break;
            }
        }
    }
    return rv;
}

//===========================================================================
bool FundingAgency::searchCE(const QString &ce) const
{
    // check if se belongs to this fa

    for (Tier *t : mTiers)
        if( t->findCE(ce) )
            return true;
    return false;
}

//===========================================================================
bool FundingAgency::searchSE(const QString &se) const
{
    // check if se belongs to this fa

    for (Tier *t : mTiers)
        if( t->findSE(se) )
            return true;
    return false;
}

//===========================================================================
void FundingAgency::setContribT(double val)
{
    // set the contribution to Tape

    if (hasT1())
        mContribT = val;
    else
        mContribT = 0.0;
}

//===========================================================================
void FundingAgency::setRequired(double cpu, double disk, double tape)
{
    // sets the requred resources
    mRequiredResources.setCPU(cpu);
    mRequiredResources.setDisk(disk);
    mRequiredResources.setTape(tape);
}

//===========================================================================
QString FundingAgency::list() const
{
    // list funding agency attributes
    QString rv = QString("**** %1: \n").arg(objectName());
    rv.append(QString("       -- M&O payers %1\n").arg(mMandOPayers, 20, 10, QChar('.')));
    rv.append(QString("       -- Contribution %1 \%\n").arg(mContrib, 20, 'f', 2, QChar('.')));

    if (mTiers.size() == 0)
        rv.append("No site");
    else {
        rv.append( QString("      -- Sites:\n"));
        for (Tier *t : mTiers)
            rv.append(t->list());
    }
    return rv;
}
