// Class describing the ALICE collaboration: Funding agencies and M&O-A payers
// singleton
// Y. Schutz November 2016

#ifndef ALICE_H
#define ALICE_H

#include <QDebug>
#include <QMap>
#include <QObject>
#include <QStandardItemModel>

#include "fundingagency.h"
#include "resources.h"

class ALICE : public QObject
{
    Q_OBJECT
    Q_ENUMS (ListOptions)
    Q_ENUMS (Column)


public:
    enum ListOptions {kFA //to Funding Agencies and M&O payers
                     };
    enum Column {kStatC, kFAC, kMOC, kConC};
    static ALICE& instance();

    void                 addCPU(double cpu)   { mT0Pledged.setCPU(mT0Pledged.getCPU() + cpu); }
    void                 addDisk(double disk) { mT0Pledged.setDisk(mT0Pledged.getDisk() + disk); }
    void                 addTape(double tape) { mT0Pledged.setTape(mT0Pledged.getTape() + tape); }
    void                 doReqAndPle(const QString &year);
    void                 drawTable();
    void                 loadGlanceData(const QString & /*year*/) { qWarning() << Q_FUNC_INFO << "not implemented"; }
    void                 loadRequirements(const QString &/*year*/) { qWarning() << Q_FUNC_INFO << "not implemented"; }
    void                 loadPledges(const QString & /*year*/) { qWarning() << Q_FUNC_INFO << "not implemented"; }
    QStandardItemModel * getModel() { return mModel; }
    void                 initTableViewModel();
    void                 listFA();
    void                 organizeFA();
    bool                 readRequirements(const QString &year);
    bool                 readMonthlyReport(const QDate &date);
    void                 saveCSV(const QString &year) const;
    FundingAgency*       searchFA(const QString &n) const;
    Tier*                searchTier(const QString &n) const;
    void                 setCEandSE(const QString &year);

private:
    ALICE(QObject *parent = 0);
    ~ALICE() {;}// mLastRow.clear(); }
    ALICE(const ALICE&) {}
    qint32 countMOPayers() const;
    qint32 countMOPayersT() const;
    bool   readGlanceData(const QString &year);
    bool   readRebus(const QString &year);

    static ALICE          mInstance;          // The unique instance of this object
    QList<FundingAgency*> mFAs;               // list of funding agencies;
    QList<QStandardItem*> mLastRow;           // the last row of the table for SUM
    QStandardItemModel*   mModel;             // The model for the table view
    Resources             mT0Pledged;         // The resources pledged at T0 in a given year
    Resources             mT1Pledged;         // The resources pledged at T1 in a given year
    Resources             mT2Pledged;         // The resources pledged at T2 in a given year
    Resources             mT0Required;        // The resources required at T0 in a given year
    Resources             mT1Required;        // The resources required at T1 in a given year
    Resources             mT2Required;        // The resources required at T2 in a given year
};

#endif // ALICE_H
