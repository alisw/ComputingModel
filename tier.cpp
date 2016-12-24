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
void Tier::addUsedResources(QString &month, Resources *res)
{
    // set the used resources during month
    Resources used;
    used.setCPU(getUsedCPU(month) + res->getCPU());
    used.setDisk(getUsedDisk(month) + res->getDisk());
    used.setTape(getUsedTape(month) + res->getTape());
    mUsed.insert(month, used);
}

//===========================================================================
void Tier::clearUsed(const QString &month)
{
    // clear resources for this month

    Resources res = mUsed[month];
    res.clear();
}

//===========================================================================
QString Tier::list() const
{
    // list the tier information
    QString text = QString("           ▻ %1 is a Tier %2 and has:\n").arg(mWLCGName).arg(mTierCategory);
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
