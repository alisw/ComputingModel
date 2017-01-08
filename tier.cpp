// computing center description
// Y. Schutz December 2016

#include <QDebug>

#include "naming.h"
#include "tier.h"

//===========================================================================
Tier::Tier(QObject *parent) : QObject(parent),
    mTierCategory(kUnknown), mWLCGAliasName(""), mWLCGName("")
{
    // default ctor
    mResources.clear();
    mUsed.clear();
}

//===========================================================================
Tier::Tier(QString name, Tier::TierCat cat, Resources &res, QObject *parent) : QObject(parent),
    mTierCategory(cat)
{
    // ctor with assignation

    mWLCGAliasName = "";
    mWLCGName = name;
    mResources.setObjectName(res.objectName());
    mResources.setCPU(res.getCPU());
    mResources.setDisk(res.getDisk());
    mResources.setTape(res.getTape());
}

//===========================================================================
void Tier::addCEs(const QList<QString> &list)
{
    // add CEs names to the ML CE List
    for (QString ce : list)
        mMLCENames.append(ce);
}

//===========================================================================
void Tier::addSEs(const QList<QString> &list)
{
    // add SEs names to the ML SE List
    for (QString se : list)
        mMLSENames.append(se);

}

//===========================================================================
void Tier::setUsedCPU(QString &month, double cpu)
{
    // set the used CPU resources during month
    Resources used;
    used.setCPU(getUsedCPU(month) + cpu);
    mUsed[month] = used;
}

//===========================================================================
void Tier::clearUsed(const QString &month)
{
    // clear resources for this month

    Resources res = mUsed[month];
    res.clear();
}

//===========================================================================
bool Tier::findCE(const QString &ce)
{
    // check if ce is in the CE list
    bool rv = false;
    if (mMLCENames.indexOf(ce) != -1)
        rv = true;
    return rv;
}

//===========================================================================
bool Tier::findSE(const QString &se)
{
    // check if se is in the SE list
    bool rv = false;
    if (mMLSENames.indexOf(se) != -1)
        rv = true;
    return rv;
}

//===========================================================================
QString Tier::list() const
{
    // list the tier information
    QString text = QString("\n           ▻ %1 is a Tier %2 and has:\n").arg(mWLCGName).arg(mTierCategory);
    text.append(mResources.list());

    text.append("ML CEs: ");
    for (QString ce : mMLCENames)
        text.append(QString("%1, ").arg(ce));
    text.remove(text.lastIndexOf(", "), 1);

    text.append("\nML SEs: ");
    for (QString se : mMLSENames)
        text.append(QString("%1, ").arg(se));
    text.remove(text.lastIndexOf(", "), 1);

    return text;
}
