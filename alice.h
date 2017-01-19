// Class describing the ALICE collaboration: Funding agencies and M&O-A payers
// singleton
// Y. Schutz November 2016

#ifndef ALICE_H
#define ALICE_H


#include <QDate>
#include <QDebug>
#include <QMap>
#include <QObject>
#include <QStandardItemModel>

#include "fundingagency.h"
#include "resources.h"
#include "tier.h"


class ALICE : public QObject
{
    Q_OBJECT
    Q_ENUMS (ListOptions)
    Q_ENUMS (Column)
    Q_ENUMS (UserCat)


public:
    enum ListOptions {kFA //to Funding Agencies and M&O payers
                     };
    enum Column {kStatC, kFAC, kMOC, kConC};

    enum UserCat {kAliDaq, kAliProd, kAliTrain, kAliUsers};

    static ALICE &instance();

    void                 addCPU(Tier::TierCat cat, double cpu);
    void                 addDisk(Tier::TierCat cat, double disk);
    void                 addTape(Tier::TierCat cat, double tape);
    qint32               countMOPayers() const;
    void                 doReqAndPle(const QString &year);
    void                 drawTable();
    void                 loadGlanceData(const QString & /*year*/) { qWarning() << Q_FUNC_INFO << "not implemented"; }
    void                 loadRequirements(const QString &/*year*/) { qWarning() << Q_FUNC_INFO << "not implemented"; }
    void                 loadPledges(const QString & /*year*/) { qWarning() << Q_FUNC_INFO << "not implemented"; }
    QStandardItemModel   *getModel() { return mModel; }
    double               getPledged(Tier::TierCat tier, Resources::Resources_type restype, const QString &year);
    double               getRequired(Tier::TierCat tier, Resources::Resources_type restype, const QString &year);
    double               getUsed(Tier::TierCat tier, Resources::Resources_type restype, const QDate date);
    void                 initTableViewModel();
    void                 listFA();
    void                 organizeFA();
    bool                 readRequirements(const QString &year);
    bool                 readMonthlyReport(const QDate &date);
    Tier                 *search(const QString &name);
    FundingAgency        *searchCE(const QString &ce) const;
    void                 saveCSV(const QString &year) const;
    FundingAgency        *searchFA(const QString &n) const;
    FundingAgency        *searchSE(const QString &se) const;
    Tier*                searchTier(const QString &n);
    void                 setCEandSE();
    void                 setDrawTable(bool val) { mDrawTable = val; }

private:
    ALICE(QObject *parent = 0);
    ~ALICE() {;}// mLastRow.clear(); }
    ALICE(const ALICE&) {}
    qint32 countMOPayersT() const;
    bool   readGlanceData(const QString &year);
    bool   readRebus(const QString &year);

    bool                  mDrawTable;              // Controls if table should be drawn of not
    static ALICE          mInstance;               // The unique instance of this object
    QList<FundingAgency*> mFAs;                    // List of funding agencies;
    QList<QStandardItem*> mLastRow;                // The last row of the table for SUM
    QStandardItemModel*   mModel;                  // The model for the table view
    Resources             mT0Pledged;              // The resources pledged at T0 in a given year
    Resources             mT1Pledged;              // The resources pledged at T1 in a given year
    Resources             mT2Pledged;              // The resources pledged at T2 in a given year
    Resources             mToPledged;              // The smoothed resources required in total in a given year
    QString               mCurrentPledgedYear;     // the current year for the pledges
    QString               mCurrentRequirementYear; // the current year for the requirements
    QDate                 mCurrentUsedDate;        // the current date for the requirements
    Resources             mT0Required;             // The resources required at T0 in a given year
    Resources             mT1Required;             // The resources required at T1 in a given year
    Resources             mT2Required;             // The resources required at T2 in a given year
    Resources             mToRequired;             // The smoothed resources required in total in a given year
    Resources             mT0Used;                 // The resources required at T0 in a given year
    Resources             mT1Used;                 // The resources required at T1 in a given year
    Resources             mT2Used;                 // The resources required at T2 in a given year
    Resources             mToUsed;                 // The smoothed resources required in total in a given year
};

#endif // ALICE_H
