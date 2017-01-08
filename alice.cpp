// Class describing the ALIE collaboration: Funding agencies and M&O-A payers
// singleton
// Y. Schutz November 2016

#include <QDir>
#include <QErrorMessage>
#include <QFile>
#include <QStandardItemModel>
#include <QStandardPaths>
#include <QTableView>

#include "alice.h"
#include "fundingagency.h"
#include "mainwindow.h"
#include "naming.h"

ALICE ALICE::mInstance = ALICE();

//===========================================================================
ALICE &ALICE::instance()
{
    return mInstance;
}

//===========================================================================
void ALICE::addCPU(Tier::TierCat cat, double cpu)
{
    // adds to the the total pledged cpu
    switch (cat) {
    case Tier::kT0:
        mT0Pledged.setCPU(mT0Pledged.getCPU() + cpu);
        break;
    case Tier::kT1:
        mT1Pledged.setCPU(mT1Pledged.getCPU() + cpu);
        break;
    case Tier::kT2:
        mT2Pledged.setCPU(mT2Pledged.getCPU() + cpu);
        break;
    default:
        break;
    }
}

//===========================================================================
void ALICE::addDisk(Tier::TierCat cat, double disk)
{
    // adds to the the total pledged disk
    switch (cat) {
    case Tier::kT0:
        mT0Pledged.setDisk(mT0Pledged.getDisk() + disk);
        break;
    case Tier::kT1:
        mT1Pledged.setDisk(mT1Pledged.getDisk() + disk);
        break;
    case Tier::kT2:
        mT2Pledged.setDisk(mT2Pledged.getDisk() + disk);
        break;
    default:
        break;
    }
}

//===========================================================================
void ALICE::addTape(Tier::TierCat cat, double tape)
{
    // adds to the the total pledged tape
    switch (cat) {
    case Tier::kT0:
        mT0Pledged.setTape(mT0Pledged.getTape() + tape);
        break;
    case Tier::kT1:
        mT1Pledged.setTape(mT1Pledged.getTape() + tape);
        break;
    case Tier::kT2:
        mT2Pledged.setTape(mT2Pledged.getTape() + tape);
        break;
    default:
        break;
    }

}

//===========================================================================
void ALICE::doReqAndPle(const QString &year)
{
    // build the table of requirements and pledges
    // collect the M&O information from Glance
    readGlanceData(year);

    organizeFA();

//    setCEandSE(year);


// TEMPORARY DISABLE

    // collect the information from Rebus, sites and pledges
    readRebus(year);

    // collect the requirements from an ad hoc table
    readRequirements(year);

    // draw the table in a view
    drawTable();

//    // save it to a csv file

    saveCSV(year);


    // TEMPORARY DISABLE
}

//===========================================================================
void ALICE::drawTable()
{
    // draw the table

    // first add a few FAs clustering FAs per country

    mModel->removeColumns(0, mModel->columnCount());

    qint32 row = 0;

    for (FundingAgency *fa : mFAs) {
        if (fa->name().left(1) == "-")
            continue;
        QList<QStandardItem*> oneRow;
        oneRow.insert(kStatC, new QStandardItem(fa->status()));
        oneRow.insert(kFAC, new QStandardItem(fa->name()));
        oneRow.insert(kMOC, new QStandardItem(QString("%1").arg(fa->payers())));
        if (fa->name() == "CERN") {
            oneRow.insert(kConC, new QStandardItem(QString("-")));
        } else {
            oneRow.insert(kConC, new QStandardItem(QString("%1").arg(fa->contrib(), 4, 'f', 2)));
        }

        qint32 col = kConC + 1;

        // required
        double cpu = fa->getRequiredCPU();
        QStandardItem *cpuSIR = new QStandardItem(QString("%1").arg(cpu, 5, 'f', 2));
        oneRow.insert(col++, cpuSIR);

        double disk = fa->getRequiredDisk();
        QStandardItem *diskSIR = new QStandardItem(QString("%1").arg(disk, 5, 'f', 2));
        oneRow.insert(col++, diskSIR);

        double tape = fa->getRequiredTape();
        QStandardItem *tapeSIR = new QStandardItem(QString("%1").arg(tape, 5, 'f', 2));
        oneRow.insert(col++, tapeSIR);

        // pledges
        cpu = fa->getPledgedCPU();
        QStandardItem *cpuSIP = new QStandardItem(QString("%1").arg(cpu, 5, 'f', 2));
        oneRow.insert(col++, cpuSIP);

        disk = fa->getPledgedDisk();
        QStandardItem *diskSIP = new QStandardItem(QString("%1").arg(disk, 5, 'f', 2));
        oneRow.insert(col++, diskSIP);

        tape = fa->getPledgedTape();
        QStandardItem *tapeSIP = new QStandardItem(QString("%1").arg(tape, 5, 'f', 2));
        oneRow.insert(col++, tapeSIP);

        // the difference
        double diff =  100 * (fa->getPledgedCPU() - fa->getRequiredCPU()) / fa->getRequiredCPU();
        QStandardItem *cpuSID = new QStandardItem(QString("%1").arg(diff, 5, 'f', 0));
        if (diff < -20)
            cpuSID->setForeground(QBrush(Qt::red));
        else
            cpuSID->setForeground(QBrush(Qt::green));
        oneRow.insert(col++, cpuSID);

        diff =  100 * (fa->getPledgedDisk() - fa->getRequiredDisk()) / fa->getRequiredDisk();
        QStandardItem *diskSID = new QStandardItem(QString("%1").arg(diff, 5, 'f', 0));
        if (diff < -20)
            diskSID->setForeground(QBrush(Qt::red));
        else
            diskSID->setForeground(QBrush(Qt::green));
        oneRow.insert(col++, diskSID);

        if (fa->getRequiredTape() != 0.0)
            diff =  100 * (fa->getPledgedTape() - fa->getRequiredTape()) / fa->getRequiredTape();
        else
            diff = 0.0;
        QStandardItem *tapeSID = new QStandardItem(QString("%1").arg(diff, 5, 'f', 0));
        if (diff < -20)
            tapeSID->setForeground(QBrush(Qt::red));
        else
            tapeSID->setForeground(QBrush(Qt::green));
        oneRow.insert(col++, tapeSID);

        mModel->insertRow(row, oneRow);
        for (qint32 index = 0; index < col; index++)
            mModel->item(row, index)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        oneRow.clear();
        row++;
    }

    // the horizontal headers

    mModel->setHorizontalHeaderItem(kStatC, new QStandardItem(tr("Status")));
    mModel->setHorizontalHeaderItem(kFAC,   new QStandardItem(tr("Funding Agency")));
    mModel->setHorizontalHeaderItem(kMOC,   new QStandardItem(tr("M&O Payers")));
    mModel->setHorizontalHeaderItem(kConC,  new QStandardItem(tr("Contribution(%)")));

    mT0Required.setObjectName(QString("Required Resources at T0"));
    mT1Required.setObjectName(QString("Required Resources at T1"));
    mT2Required.setObjectName(QString("Required Resources at T2"));

    mModel->setHorizontalHeaderItem(kConC + 1, new QStandardItem(tr("Required CPU (kHEPSPEC06)")));
    mModel->setHorizontalHeaderItem(kConC + 2, new QStandardItem(tr("Required Disk (PB)")));
    mModel->setHorizontalHeaderItem(kConC + 3, new QStandardItem(tr("Required Tape (PB)")));

    mModel->setHorizontalHeaderItem(kConC + 4, new QStandardItem(tr("Pledged CPU (kHEPSPEC06)")));
    mModel->setHorizontalHeaderItem(kConC + 5, new QStandardItem(tr("Pledged Disk (PB)")));
    mModel->setHorizontalHeaderItem(kConC + 6, new QStandardItem(tr("Pledged Tape (PB)")));

    mModel->setHorizontalHeaderItem(kConC + 7, new QStandardItem(tr("Diff CPU (%)")));
    mModel->setHorizontalHeaderItem(kConC + 8, new QStandardItem(tr("Diff Disk (%)")));
    mModel->setHorizontalHeaderItem(kConC + 9, new QStandardItem(tr("Diff Tape (%)")));

    // the last row

    mLastRow.clear();

    QStandardItem * blanck1 = new QStandardItem("");
    mLastRow.insert(0, blanck1);
    QStandardItem *total = new QStandardItem("Total");
    total->setTextAlignment((Qt::AlignHCenter | Qt::AlignVCenter));
    mLastRow.insert(1, total);
    QStandardItem *totMO = new QStandardItem(QString("%1").arg(countMOPayers()));
    totMO->setTextAlignment((Qt::AlignRight | Qt::AlignVCenter));
    mLastRow.insert(2, totMO);
    QStandardItem * blanck2 = new QStandardItem("");
    mLastRow.insert(3, blanck2);

    double sumRequiredCPU = mT0Required.getCPU()  + mT1Required.getCPU() + mT2Required.getCPU();
    QStandardItem *sumCPUR  = new QStandardItem(QString("%1").arg(sumRequiredCPU, 5, 'f', 2));
    double sumRequiredDisk = mT0Required.getDisk()  + mT1Required.getDisk() + mT2Required.getDisk();
    QStandardItem *sumDiskR = new QStandardItem(QString("%1").arg(sumRequiredDisk, 5, 'f', 2));
    double sumRequiredTape = mT0Required.getTape()  + mT1Required.getTape() + mT2Required.getTape();
    QStandardItem *sumTapeR = new QStandardItem(QString("%1").arg(sumRequiredTape, 5, 'f', 2));
    sumCPUR->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sumDiskR->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sumTapeR->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);

    mLastRow.insert(4, sumCPUR);
    mLastRow.insert(5, sumDiskR);
    mLastRow.insert(6, sumTapeR);

    double sumPledgedCPU = mT0Pledged.getCPU()  + mT1Pledged.getCPU()  + mT2Pledged.getCPU();
    QStandardItem *sumCPUP  = new QStandardItem(QString("%1").arg(sumPledgedCPU, 5, 'f', 2));
    double sumPledgedDisk = mT0Pledged.getDisk()  + mT1Pledged.getDisk()  + mT2Pledged.getDisk();
    QStandardItem *sumDiskP = new QStandardItem(QString("%1").arg(sumPledgedDisk, 5, 'f', 2));
    double sumPledgedTape = mT0Pledged.getTape()  + mT1Pledged.getTape()  + mT2Pledged.getTape();
    QStandardItem *sumTapeP = new QStandardItem(QString("%1").arg(sumPledgedTape, 5, 'f', 2));
    sumCPUP->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sumDiskP->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    sumTapeP->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);


    mLastRow.insert(7, sumCPUP);
    mLastRow.insert(8, sumDiskP);
    mLastRow.insert(9, sumTapeP);

    double diff = 100 * ( sumPledgedCPU - sumRequiredCPU ) / sumRequiredCPU;
    QStandardItem *sumCPUD  = new QStandardItem(QString("%1").arg(diff, 5, 'f', 2));
    if (diff < -20)
        sumCPUD->setForeground(QBrush(Qt::red));
    else
        sumCPUD->setForeground(QBrush(Qt::green));
    sumCPUD->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    diff = 100 * ( sumPledgedDisk - sumRequiredDisk ) / sumRequiredDisk;
    QStandardItem *sumDiskD = new QStandardItem(QString("%1").arg(diff, 5, 'f', 2));
    if (diff < -20)
        sumDiskD->setForeground(QBrush(Qt::red));
    else
        sumDiskD->setForeground(QBrush(Qt::green));
    diff = 100 * ( sumPledgedTape - sumRequiredTape ) / sumRequiredTape;
    sumDiskD->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
    QStandardItem *sumTapeD = new QStandardItem(QString("%1").arg(diff, 5, 'f', 2));
    if (diff < -20)
        sumTapeD->setForeground(QBrush(Qt::red));
    else
        sumTapeD->setForeground(QBrush(Qt::green));
    sumTapeD->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);


    mLastRow.insert(10, sumCPUD);
    mLastRow.insert(11, sumDiskD);
    mLastRow.insert(12, sumTapeD);


    mModel->appendRow(mLastRow);

}

//===========================================================================
double ALICE::getPledged(Tier::TierCat tier, Resources::Resources_type restype, const QString &year)
{
    // retrieve required resources

    if (year != mCurrentPledgedYear)
            readRebus(year);

    Resources res;
    switch (tier) {
    case Tier::kT0:
        res = mT0Pledged;
        break;
    case Tier::kT1:
        res = mT1Pledged;
        break;
    case Tier::kT2:
        res = mT2Pledged;
        break;
    case Tier::kTOTS:
        res = mToPledged;
        break;
    default:
        break;
    }
    double rv = 0.0;
    switch (restype) {
    case Resources::kCPU:
        rv = res.getCPU();
        break;
    case Resources::kDISK:
        rv = res.getDisk();
        break;
    case Resources::kTAPE:
        rv = res.getTape();
        break;
    default:
        break;
    }

    return rv;
}

//===========================================================================
double ALICE::getRequired(Tier::TierCat tier, Resources::Resources_type restype, const QString &year)
{
    // retrieve required resources

    if (year != mCurrentRequirementYear)
            readRequirements(year);

    Resources res;
    switch (tier) {
    case Tier::kT0:
        res = mT0Required;
        break;
    case Tier::kT1:
        res = mT1Required;
        break;
    case Tier::kT2:
        res = mT2Required;
        break;
    case Tier::kTOTS:
        res = mToRequired;
        break;
    default:
        break;
    }
    double rv = 0.0;
    switch (restype) {
    case Resources::kCPU:
        rv = res.getCPU();
        break;
    case Resources::kDISK:
        rv = res.getDisk();
        break;
    case Resources::kTAPE:
        rv = res.getTape();
        break;
    default:
        break;
    }

    return rv;
}

//===========================================================================
double ALICE::getUsed(Tier::TierCat tier, Resources::Resources_type restype, const QDate date)
{
    // retrieve required resources

    if (date != mCurrentUsedDate)
            readMonthlyReport(date);

    Resources res;
    switch (tier) {
    case Tier::kT0:
        res = mT0Used;
        break;
    case Tier::kT1:
        res = mT1Used;
        break;
    case Tier::kT2:
        res = mT2Used;
        break;
    case Tier::kTOTS:
        res = mToUsed;
        break;
    default:
        break;
    }
    double rv = 0.0;
    switch (restype) {
    case Resources::kCPU:
        rv = res.getCPU();
        break;
    case Resources::kDISK:
        rv = res.getDisk();
        break;
    case Resources::kTAPE:
        rv = res.getTape();
        break;
    default:
        break;
    }

    return rv;
}

//===========================================================================
bool ALICE::readRebus(const QString &year)
{
//    Associate sites to Funding Agencies and collect the pledges

    if (mFAs.isEmpty()) {
        readGlanceData(year);
        organizeFA();
    }
    mT0Pledged.clear();
    mT1Pledged.clear();
    mT2Pledged.clear();
    mToPledged.clear();
    mT0Pledged.setObjectName(QString("Pledged Resources at T0 in %1").arg(year));
    mT1Pledged.setObjectName(QString("Pledged Resources at T1 in %1").arg(year));
    mT2Pledged.setObjectName(QString("Pledged Resources at T2 in %1").arg(year));
    mToPledged.setObjectName(QString("Pledged Resources in total in %1").arg(year));

    QString filename = QString(":/data/%1/pledges.csv").arg(year);
    QFile csvFile(filename);
    if (!csvFile.open(QIODevice::ReadOnly)) {
        qWarning() << "file" << filename << "not found";
        return false;
    }
    // read the header row and find the column of ALICE V0
    qint32 aliceColumn = -1;
    QString line = csvFile.readLine();
    QStringList strList = line.split(',');
    qint32 nbColumn = strList.size();
    for (qint32 column = 0; column < strList.size(); column++) {
        QString tempo = strList.at(column);
        tempo.remove("\"");
        tempo.remove(" ");
        // find the ALICE column
        if (tempo == "ALICE") {
            aliceColumn = column;
            break;
        }
    }
    while (!csvFile.atEnd()) {
        QString line = csvFile.readLine();
        QStringList strList = line.split(',');
        qint32 diff = strList.size() - nbColumn;
        QString test = strList.at(aliceColumn + diff);
        if (test.toInt() != 0) {
            Resources::Cpu_Unit cunit; ;
            Resources::Storage_Unit sunit;
            Resources res("pledged");
            res.clear();
            Tier::TierCat cat;
            if (strList.at(0) == "Tier 0")
                cat = Tier::kT0;
            else if (strList.at(0) == "Tier 1")
                cat = Tier::kT1;
            else if (strList.at(0) == "Tier 2")
                cat = Tier::kT2;
            else
                qWarning() << "Tier category" << strList.at(0) << "not recognized";

            FundingAgency *fa = searchFA(strList.at(1));

            QString sCPU = strList.at(aliceColumn + diff);
            if (strList.at(4 + diff) == "HEP-SPEC06")
                cunit = Resources::HEPSPEC06;
            else
                qFatal("revise the csv format");
            res.setCPU(sCPU.toDouble(), cunit);
            addCPU(cat, res.getCPU());
            line = csvFile.readLine();
            strList = line.split(',');
            qint32 diff = strList.size() - nbColumn;
            QString sDisk = strList.at(aliceColumn + diff);
            if (strList.at(4 + diff) == "Tbytes")
                sunit = Resources::TB;
            else
                qFatal("revise the csv format");
            res.setDisk(sDisk.toDouble(), sunit);
            addDisk(cat, res.getDisk());
            if (cat == Tier::kT0 || cat == Tier::kT1) {
                line = csvFile.readLine();
                strList = line.split(',');
                qint32 diff = strList.size() - nbColumn;
                QString sTape = strList.at(aliceColumn + diff);
                if (strList.at(4 + diff) == "Tbytes")
                    sunit = Resources::TB;
                else
                    qFatal("revise the csv format");
                res.setTape(sTape.toDouble(), sunit);

                addTape(cat, res.getTape());
            }
            QString site = strList.at(2);
            site.remove("\"");
            Tier *t = new Tier(site, cat, res, fa);
            QList<QString> ceList = Naming::instance()->find(fa->name(), Naming::kCEML);
            QList<QString> seList = Naming::instance()->find(fa->name(), Naming::kSE);
            t->addCEs(ceList);
            t->addSEs(seList);
            fa->addTier(t);
        }
    }
    csvFile.close();

    // now register the sites (CE and SE) which are not member of WLCG
    Resources res;
    res.clear(); // no resources pledged
    for (FundingAgency *fa : mFAs) {
        if (!fa->hasTier() && fa->name().left(1) != "-") {
            QList<QString> ceList = Naming::instance()->find(fa->name(), Naming::kCEML);
            QList<QString> seList = Naming::instance()->find(fa->name(), Naming::kSE);
            for (QString site : ceList) {
                Tier *tier = new Tier(site, Tier::kT2, res, fa);
                tier->addCE(site);
                for (QString se : seList) {
                    if (se.contains(site))
                        tier->addSE(se);
                }
                fa->addTier(tier);
            }
        }
    }
    mToPledged.setCPU( mT0Pledged.getCPU()  + mT1Pledged.getCPU()  + mT2Pledged.getCPU());
    mToPledged.setDisk(mT0Pledged.getDisk() + mT1Pledged.getDisk() + mT2Pledged.getDisk());
    mToPledged.setTape(mT0Pledged.getTape() + mT1Pledged.getTape() + mT2Pledged.getTape());

    if (MainWindow::isDebug()) {
        for (FundingAgency *fa : mFAs)
            qInfo() << fa->list();
        qInfo() <<  mT0Pledged.list();
        qInfo() <<  mT1Pledged.list();
        qInfo() <<  mT2Pledged.list();
    }
    mCurrentPledgedYear = year;
    return true;
}

//===========================================================================
void ALICE::initTableViewModel()
{
    // initialize the model for viewing data in tableview format

//    mModel->setHorizontalHeaderItem(kStatC, new QStandardItem(tr("Status")));
//    mModel->setHorizontalHeaderItem(kFAC,   new QStandardItem(tr("Funding Agency")));
//    mModel->setHorizontalHeaderItem(kMOC,   new QStandardItem(tr("M&O Payers")));
//    mModel->setHorizontalHeaderItem(kConC,  new QStandardItem(tr("Contribution(%)")));
}

//===========================================================================
bool ALICE::readGlanceData(const QString &year)
{
    // read the csv file generated by glance:
    // 1. Membership -> Members
    // 2. M&O Budget = yes
    // 3. status = active
    // 4. start date <  1  September 20xx
    // 5. end date   > 31  Août      20xx
    // 6. export as csv

    // check the available data per year

    mFAs.clear();

    const QString fa("Funding Agency");

    QFile csvFile(QString(":/data/%1/MandO.csv").arg(year));
    if (!csvFile.open(QIODevice::ReadOnly))
        return false;
    // read the header row and find the column of funding agencies
    qint32 faColumn = 1;
    QString data = csvFile.readLine();
    data.remove(QRegExp("\r")); // remove all occurences of CR
    QStringList strList = data.split(',');
    qint32 nbColums = strList.size() + 1;  // thre is always a ',' in the name field

    for (QString str : strList) {
        if (str.count("\"")%2 == 0) {
            if (str.startsWith(QChar('\"')) && str.endsWith(QChar('\"'))) {
                str.remove(QRegExp("^\" "));
                str.remove(QRegExp("\"$"));
            }
        }
        if (str == fa) {
            break;
        }
        faColumn++;
    }

    // now fill the hash table with FA names and M&O payers
    qint32 line = 0;
    QMap<QString, int> collabo;
    while (!csvFile.atEnd()) {
        QString fa;
        QString data = csvFile.readLine();
        QStringList strList = data.split(',');
        qint32 delta = strList.size() - nbColums; // delta > 0: happens because there is one or more  ',' in the Institute field
        fa = strList.at(faColumn + delta );
        fa.remove(QRegExp("\""));

        if (collabo.contains(fa)) {
            collabo[fa] = collabo[fa] + 1;
        } else {
            collabo[fa] = 1;
        }

        line++;
    }

    csvFile.close();

    // fill the funding agencies list

    QMapIterator<QString, int> fag(collabo);
    while (fag.hasNext()) {
        fag.next();
        QString name(fag.key());
        qint32  payers = fag.value();
        QString status = name.left(name.indexOf('-'));
        qint32 istatus;
        if (status == "MS")
            istatus = 0;
        else
            istatus = 1;
        name.remove(0, name.indexOf('-') + 1); // remove MS- or NMS-
        mFAs.append(new FundingAgency(name, istatus, payers));
    }
    return true;
}

//===========================================================================
bool ALICE::readRequirements(const QString &year)
{
    // reads the requirements from a csv file
    // the csv file has to be produced by hand from an excell table:
    //      CPU (kHEPSPEC06)   Disk (PB)    Tape (PB)
    // T0           xxxx          xxxx         xxxx
    // T1           xxxx          xxxx         xxxx
    // T2           xxxx          xxxx         xxxx

    mT0Required.clear();
    mT1Required.clear();
    mT2Required.clear();
    mToRequired.clear();
    mT0Pledged.setObjectName(QString("Required Resources at T0 in %1").arg(year));
    mT1Pledged.setObjectName(QString("Required Resources at T1 in %1").arg(year));
    mT2Pledged.setObjectName(QString("Required Resources at T2 in %1").arg(year));
    mToPledged.setObjectName(QString("Required Resources in total in %1").arg(year));

    const QString cpuName("CPU");
    const QString diskName("Disk");
    const QString tapeName("Tape");
    const QString t0Name("T0");
    const QString t1Name("T1");
    const QString t2Name("T2");
    const QString toName("Total smooth");

    QFile csvFile(QString(":/data/%1/Requirements.csv").arg(year));
    if (!csvFile.open(QIODevice::ReadOnly))
        return false;
    // read the header row and find the column for CPU, Disk and Tape
    QString data = csvFile.readLine();
    QStringList strList = data.split(';');
    qint32 cpuColumn  = -1;
    qint32 diskColumn = -1;
    qint32 tapeColumn = -1;
    qint32 column = 0;

    for (QString str : strList) {
        str.remove(QRegExp("\r"));
        str.remove(QRegExp("\n"));
        if ( str == cpuName)
            cpuColumn = column;
        else if (str == diskName)
            diskColumn = column;
        else if (str == tapeName)
            tapeColumn = column;
        column++;
    }

    // now finds the row for T0, T1 and T2
    qint32 t0Row      = -1;
    qint32 t1Row      = -1;
    qint32 t2Row      = -1;
    qint32 toRow      = -1;
    qint32 row = 0;

    while (!csvFile.atEnd()) {
        QString data = csvFile.readLine();
        QStringList strList = data.split(';');
        if (strList.first() == t0Name)
            t0Row = row;
        else if (strList.first() == t1Name)
            t1Row = row;
        else if (strList.first() == t2Name)
            t2Row = row;
        else if (strList.first() == toName)
            toRow = row;
        row++;
    }
    csvFile.seek(0);
    csvFile.readLine();
    row = 0;
    while (!csvFile.atEnd()) {
        QString data = csvFile.readLine();
        QStringList strList = data.split(';');
        QString tempo = strList.at(cpuColumn);
        double cpu = tempo.toDouble();
        tempo = strList.at(diskColumn);
        double disk = tempo.toDouble();
        tempo = strList.at(tapeColumn);
        double tape = tempo.toDouble();
        if (row == t0Row) {
            mT0Required.setCPU(cpu);
            mT0Required.setDisk(disk);
            mT0Required.setTape(tape);
        }
        else if (row == t1Row) {
            mT1Required.setCPU(cpu);
            mT1Required.setDisk(disk);
            mT1Required.setTape(tape);
        } else if (row == t2Row) {
            mT2Required.setCPU(cpu);
            mT2Required.setDisk(disk);
            mT2Required.setTape(tape);
        }  else if (row == toRow) {
            mToRequired.setCPU(cpu);
            mToRequired.setDisk(disk);
            mToRequired.setTape(tape);
        }
        row++;
    }

    csvFile.close();

    if (MainWindow::isDebug()) {
        mT0Required.list();
        mT1Required.list();
        mT2Required.list();
    }

    // calculates the contribution of each FA

    qint32 norm = countMOPayers();
    if (norm == 0) // the following is not needed when plots are invoked
        return true;
    norm -= searchFA("CERN")->payers();  // CERN is not counted in the M&O payers
    qint32 normT = countMOPayersT();

    for (FundingAgency *fa : mFAs) {
        double cpuR;
        double diskR;
        double tapeR;
        if (fa->name() == "CERN") {
            cpuR  = mT0Required.getCPU();
            diskR = mT0Required.getDisk();
            tapeR = mT0Required.getTape();
        } else if (fa->name().left(1) == "-") {
            cpuR  = 0.0;
            diskR = 0.0;
            tapeR = 0.0;
        } else {
            double frac = (double)fa->payers() / norm;
            fa->setContrib(frac * 100);
            double fracT = (double)fa->payers() / normT;
            fa->setContribT(fracT * 100);
            cpuR  = (mT1Required.getCPU()  + mT2Required.getCPU())  * frac;
            diskR = (mT1Required.getDisk() + mT2Required.getDisk()) * frac;
            if (fa->hasT1())
                tapeR = (mT1Required.getTape() + mT2Required.getTape()) * fracT;
            else
                tapeR = 0.0;
        }
        fa->setRequired(cpuR, diskR, tapeR);
    }

    mCurrentRequirementYear = year;
    return true;
}

//===========================================================================
bool ALICE::readMonthlyReport(const QDate &date)
{
    // read the montly report

    if (mFAs.isEmpty()) {
        readGlanceData(QString("%1").arg(date.year()));
        organizeFA();
        readRebus(QString("%1").arg(date.year()));
    }

    mT0Used.clear();
    mT1Used.clear();
    mT2Used.clear();
    mToUsed.clear();
    qint32 hours = date.daysInMonth() * 24;
    QString month = date.toString("MMMM");
    mCurrentUsedDate = date;

    // First read the monthly report provided by EGI (http://accounting.egi.eu/egi.php)
    // one file for T0+T1s and one for T2s
    // select Norm. Sum CPU (HEPSPEC06-hours)
    // format T0+T1s
    // line 1-4: header to be skipped
    // line 5: header TIER1,"alice","atlas","cms","lhcb",Total
    // last line      Total, xxxxx (HEPSPEC06-hours)

    QString fileName = QString(":/data/%1/%2/TIER1_TIER1_sum_normcpu_TIER1_VO.csv").arg(date.year()).arg(date.month());
    QFile  csvFile(fileName);
    if(!csvFile.open(QIODevice::ReadOnly))
        return false;
    // read header 5 lines
    csvFile.readLine();
    csvFile.readLine();
    csvFile.readLine();
    csvFile.readLine();
    QString line = csvFile.readLine();
    QStringList strList = line.split(',');
    qint32 aliceColumn = -1;
    qint32 index = 0;
    for (QString str : strList) {
        if (str.contains("alice")) {
            aliceColumn = index;
            break;
        }
        index++;
    }

    double cpuUSumT0 = 0.0;
    double cpuUSumT1 = 0.0;

    while(!csvFile.atEnd()) {
        line = csvFile.readLine();
        strList = line.split(',');
        QString site = strList.at(0);
        if (site == "Total")
            break;
        if (strList.at(aliceColumn) != "") {
            QString scpu  = strList.at(aliceColumn);
            double cpu = scpu.toDouble();
            Tier* tier = searchTier(site);
            if (!tier) {
                qWarning() << Q_FUNC_INFO << site << " not found!";
                exit(1);
            }
            Resources res;
            double rcpu = cpu / hours / 1000.;
            res.setCPU(rcpu, Resources::kHEPSPEC06);
            tier->setUsedCPU(month, res.getCPU());
            if (site == "CH-CERN")
                cpuUSumT0 += rcpu;
            else
                cpuUSumT1 += rcpu;
        }
    }
    csvFile.close();
    mT0Used.setCPU(cpuUSumT0, Resources::kHEPSPEC06);
    mT1Used.setCPU(cpuUSumT1, Resources::kHEPSPEC06);

    // format T2s from http://accounting.egi.eu/reptier2.php
    // line 1-4: header to be skipped
    // line 5: COUNTRY,FEDERATION,2016 CPU Pledge (HEPSPEC06),pledge inc. efficiency (HEPSPEC06-Hrs),SITE,alice,atlas,cms,lhcb,Total,delivered as % of pledge
    fileName = QString(":/data/%1/%2/reptier2.csv").arg(date.year()).arg(date.month());
    csvFile.setFileName(fileName);
    if(!csvFile.open(QIODevice::ReadOnly))
        return false;
    // read header 5 lines
    csvFile.readLine();
    csvFile.readLine();
    csvFile.readLine();
    csvFile.readLine();
    line = csvFile.readLine();
    strList = line.split(',');
    aliceColumn = -1;
    index = 0;
    for (QString str : strList) {
        if (str.contains("alice")) {
            aliceColumn = index;
            break;
        }
        index++;
    }

    double cpuUSumT2 = 0.0;

    while(!csvFile.atEnd()) {
        line = csvFile.readLine();
        strList = line.split(',');
        QString country = strList.at(0);
        if (country == "")
            continue;
        QString federation = strList.at(1);
        if (federation.contains("Total"))
            break;
        if (strList.at(aliceColumn) != "0") {
            QString scpu  = strList.at(aliceColumn);
            double cpu = scpu.toDouble();
            FundingAgency *fa = searchFA(country);
            if (!fa) {
                qWarning() << country << "is not an ALICE member";
                continue;
            }
            Tier *tier = fa->search(federation, true);
            if (!tier) {
                qDebug() << date << line;
                qWarning() << federation << " not found!";
                continue;
            }
            Resources res;
            double rcpu = cpu / hours / 1000.;
            res.setCPU(rcpu, Resources::kHEPSPEC06);
            tier->setUsedCPU(month, res.getCPU());
            cpuUSumT2 += rcpu;
        }
    }
    csvFile.close();
    mT2Used.setCPU(cpuUSumT2, Resources::kHEPSPEC06);
    mToUsed.setCPU(cpuUSumT0 + cpuUSumT1 + cpuUSumT2, Resources::kHEPSPEC06);

    for (FundingAgency * fa : mFAs)
        fa->computeUsedCPU(month);

    // read the CPU usage delivered by MonALISA
    // get it from http://alimonitor.cern.ch/display?annotation.enabled=true&imgsize=1024x600&interval.max=0&interval.min=2628000000&page=jobResUsageSum_time_si2k&download_data_csv=true
    // where max and min are in milliseconds in the past, relative to current timestamp, so 0 = current time
    // see the speadsheet data/MillisecondsFromToday.numbers to compute min/max for each month
    // name of the csv file is CPU_Usage.csv
    // format:
    // header: Time xxxx
    // where xxxx is the name of the CE
    // TimeStamp, data (in GB)
    // take the average over time

    fileName = QString(":/data/%1/%2/CPU_Usage.csv").arg(date.year()).arg(date.month());
    csvFile.setFileName(fileName);
    if(!csvFile.open(QIODevice::ReadOnly))
        return false;
    line = csvFile.readLine();
    QStringList listCE = line.split(',');
    listCE.removeAt(0); // removes the Time column
    QHash<QString, double> cpuUsage;
    qint32 linecount = 0;
    while (!csvFile.atEnd()) {
        QString line = csvFile.readLine();
        QStringList valuesList = line.split(',');
        valuesList.removeAt(0);
        for (qint32 column = 0; column < valuesList.size(); column++) {
            QString key = listCE.at(column);
            key.remove("\n");
            QString value = valuesList.at(column);
            cpuUsage[key] += value.toDouble();
        }
        linecount++;
    }
    csvFile.close();
    QHashIterator<QString, double> cpuit(cpuUsage);
    while (cpuit.hasNext()) {
        cpuit.next();
        cpuUsage[cpuit.key()] = cpuit.value() * 4.2 / hours / linecount / 10000; //units = KHEPSpec06; 4.2 converts KSI2K into HEPSpec06
        FundingAgency *fa = searchCE(cpuit.key());
        if (fa)
            fa->addUsedCPU(month,cpuUsage[cpuit.key()]);
        else if (MainWindow::isDebug())
            qWarning() << "Ignore CE" << cpuit.key() << "providing" << cpuUsage[cpuit.key()] << "kHEPSpec06";
    }


    // read the disk usage delivered by MonALISA
    // get it from: http://alimonitor.cern.ch/display?imgsize=1024x600&interval.max=27630000000&interval.min=30301200000&job_stats.owner=brijesh&modules=SE%2Fhist_used&page=SE%2Fhist&download_data_csv=true
    // where max and min are in milliseconds in the past, relative to current timestamp, so 0 = current time
    // see the speadsheet data/MillisecondsFromToday.numbers to compute min/max for each month
    // name of the csv file is Disk_Tape_Usage.csv
    // format:
    // header: Time ALICE::xxxx::yy ....
    // where xxxx is the name of the SE and yy is either SE, TAPE, etc..
    // TimeStamp, data (in GB)
    // take the average over time


    fileName = QString(":/data/%1/%2/Disk_Tape_Usage.csv").arg(date.year()).arg(date.month());
    csvFile.setFileName(fileName);
    if(!csvFile.open(QIODevice::ReadOnly))
        return false;
    line = csvFile.readLine();
    QStringList listSE = line.split(',');
    listSE.removeAt(0); // removes the Time column
    QHash<QString, double> diskUsage;
    linecount = 0;
    while (!csvFile.atEnd()) {
        QString line = csvFile.readLine();
        QStringList valuesList = line.split(',');
        valuesList.removeAt(0);
        for (qint32 column = 0; column < valuesList.size(); column++) {
            QString key = listSE.at(column);
            key.remove("\n");
            QString value = valuesList.at(column);
            diskUsage[key] += value.toDouble();
        }
        linecount++;
    }
    csvFile.close();

    double tapeUSumT0 = 0.0;
    double tapeUSumT1 = 0.0;
    double diskUSumT0 = 0.0;
    double diskUSumT1 = 0.0;
    double diskUSumT2 = 0.0;

    QHashIterator<QString, double> stoit(diskUsage);
    while (stoit.hasNext()) {
        stoit.next();
        diskUsage[stoit.key()] = stoit.value() / linecount / 1000000; //units = PB
        FundingAgency *fa = searchSE(stoit.key());
        QString se = stoit.key();
        double storage = diskUsage[se];
        Resources::Resources_type diskOrTape = fa->addUsedDiskTape(month, se, storage);
        if (diskOrTape == Resources::kTAPE) {
            if (se.contains("CERN"))
                tapeUSumT0 += storage;
            else
                tapeUSumT1 += storage;
        } else if (diskOrTape == Resources::kDISK) {
            if (se.contains("CERN"))
                diskUSumT0 += storage;
            else if (se.contains("CCIN2P3") ||
                     se.contains("CNAF") ||
                     se.contains("FZK") ||
                     se.contains("KISTI") ||
                     se.contains("NDGF") ||
                     se.contains("RAL") ||
                     se.contains("RRC") ||
                     se.contains("SARA"))
                diskUSumT1 += storage;
            else
                diskUSumT2 += storage;
        }
    }

    mT0Used.setDisk(diskUSumT0, Resources::PB);
    mT1Used.setDisk(diskUSumT1, Resources::PB);
    mT2Used.setDisk(diskUSumT2, Resources::PB);
    mT0Used.setTape(tapeUSumT0, Resources::PB);
    mT1Used.setTape(tapeUSumT1, Resources::PB);
    mToUsed.setDisk(diskUSumT0 + diskUSumT1 + diskUSumT2, Resources::PB);
    mToUsed.setTape(tapeUSumT0 + tapeUSumT1, Resources::PB);


    if (!mDrawTable)
        return true;

    // complete the table
    double cpuUSum    = 0.0;
    double cpuUSumML  = 0.0;
    double diskUSumML = 0.0;
    double tapeUSumML = 0.0;

    QList<QStandardItem*> lcpuUColumn;
    QList<QStandardItem*> lcpuUColumnML;
    QList<QStandardItem*> ldiskUColumnML;
    QList<QStandardItem*> ltapeUColumnML;
    for (FundingAgency *fa : mFAs) {
        if (fa->name().left(1) == "-")
            continue;
        double cpuU  = fa->getUsedCPU(month);
        cpuUSum += cpuU;
        double cpuUML  = fa->getUsedCPUML(month);
        cpuUSumML += cpuUML;
        double diskUML = fa->getUsedDiskML(month);
        diskUSumML += diskUML;
        double tapeUML = fa->getUsedTapeML(month);
        tapeUSumML += tapeUML;
        QStandardItem *cpuSIU    = new QStandardItem(QString("%1").arg(cpuU,    5, 'f', 2));
        QStandardItem *cpuSIUML  = new QStandardItem(QString("%1").arg(cpuUML,  5, 'f', 2));
        QStandardItem *diskSIUML = new QStandardItem(QString("%1").arg(diskUML, 5, 'f', 2));
        QStandardItem *tapeSIUML = new QStandardItem(QString("%1").arg(tapeUML, 5, 'f', 2));
        lcpuUColumn.append(cpuSIU);
        lcpuUColumnML.append(cpuSIUML);
        ldiskUColumnML.append(diskSIUML);
        ltapeUColumnML.append(tapeSIUML);
    }

    mToUsed.setCPU(cpuUSum);
    mToUsed.setDisk(diskUSumML);
    mToUsed.setTape(tapeUSumML);

    QStandardItem *totalcpuSIU  = new QStandardItem(QString("%1").arg(cpuUSum,  5, 'f', 2));
    lcpuUColumn.append(totalcpuSIU);
    mModel->appendColumn(lcpuUColumn);
    QString header = QString("%1 \n Used CPU (kHEPSPEC06)").arg(month);
    mModel->setHorizontalHeaderItem(mModel->columnCount() -1, new QStandardItem(header));

    QStandardItem *totalcpuSIUML  = new QStandardItem(QString("%1").arg(cpuUSumML,  5, 'f', 2));
    lcpuUColumnML.append(totalcpuSIUML);
    mModel->appendColumn(lcpuUColumnML);
    header = QString("%1 \n ML Used CPU (kHEPSPEC06)").arg(month);
    mModel->setHorizontalHeaderItem(mModel->columnCount() -1, new QStandardItem(header));

    QStandardItem *totaldiskSIUML  = new QStandardItem(QString("%1").arg(diskUSumML,  5, 'f', 2));
    ldiskUColumnML.append(totaldiskSIUML);
    mModel->appendColumn(ldiskUColumnML);
    header = QString("%1 \n ML Used Disk (PB)").arg(month);
    mModel->setHorizontalHeaderItem(mModel->columnCount() - 1, new QStandardItem(header));

    QStandardItem *totaltapeSIUML  = new QStandardItem(QString("%1").arg(tapeUSumML,  5, 'f', 2));
    ltapeUColumnML.append(totaltapeSIUML);
    mModel->appendColumn(ltapeUColumnML);
    header = QString("%1 \n ML Used Tape (PB)").arg(month);
    mModel->setHorizontalHeaderItem(mModel->columnCount() - 1, new QStandardItem(header));


    return true;
}

//===========================================================================
void ALICE::saveCSV(const QString &year) const
{
    // save the table in a csv file
    QString textData;
    int rows = mModel->rowCount();
    int columns = mModel->columnCount();

    for (int j = 0; j < columns; j++) {
        textData += mModel->headerData(j, Qt::Horizontal, Qt::DisplayRole).toString();
        textData += ", " ;     // for .csv file format
    }
    textData += "\n";             // (optional: for new line segmentation)

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < columns; j++) {
                textData += mModel->data(mModel->index(i,j)).toString();
                textData += ", " ;     // for .csv file format
        }
        textData += "\n";             // (optional: for new line segmentation)
    }

    // [Save to file] (header file <QFile> needed)
    // .csv

    QString dir = QStandardPaths::displayName(QStandardPaths::DesktopLocation);
    dir.prepend(QDir::separator());
    dir.prepend(QDir::homePath());

    QFile csvFile(QString("%1/PleAndReq%2.csv").arg(dir, year));
    if(csvFile.open(QIODevice::WriteOnly | QIODevice::Truncate)) {

        QTextStream out(&csvFile);
        out << textData;

        csvFile.close();
    }
}

//===========================================================================
FundingAgency *ALICE::searchCE(const QString &ce) const
{
    // searches to which FA belongs the storage ce
    for (FundingAgency *fa : mFAs) {
        if (fa->name().left(1) == "-") // skip FAs included in a cluster
            continue;
        if (fa->searchCE(ce)) {
            return fa;
        }
    }
    return Q_NULLPTR;

}

//===========================================================================
FundingAgency *ALICE::searchFA(const QString &n) const
{
    // searches funding agency by name

    QString name(n);

    // treat special cases
    if (name == "Switzerland")
        name = "CERN";
    else if (name == "Russian Federation")
        name = "Russia";
    else if (name == "UK")
        name = "UnitedKingdom-STFC";
    else if (name == "Latin America")
        name = "Brazil";
    for (FundingAgency *fa : mFAs)
        if (fa->name().contains(name) && fa->name().left(1) != "-") {
           return fa;
           break;
        }
    if (MainWindow::isDebug())
         qWarning() << QString("FA %1 not found").arg(name);
    return Q_NULLPTR;
}

//===========================================================================
FundingAgency *ALICE::searchSE(const QString &se) const
{
    // searches to which FA belongs the storage se
    for (FundingAgency *fa : mFAs) {
        if (fa->name().left(1) == "-") // skip FAs included in a cluster
            continue;
        if (fa->searchSE(se)) {
            return fa;
        }
    }
    return Q_NULLPTR;
}

//===========================================================================
Tier *ALICE::searchTier(const QString &n) const
{
    // search a Tier by name in the list of FAs

    Tier * rv = Q_NULLPTR;
    for (FundingAgency *fa : mFAs) {
        if (fa->name().left(1) == "-") // skip FAs included in a cluster
            continue;
        rv = fa->search(n);
        if(rv) {
            break;
        }
    }
    return rv;
}

//===========================================================================
void ALICE::setCEandSE()
{
    // set the CE and SE with ML and WLCG naming to FAs

    for (FundingAgency *fa : mFAs) {
        QString name = fa->name();
        if (name.left(1) == "-")
            continue;
        QList<QString> tiers = Naming::instance()->find(fa->name(), Naming::kCEWLCG);
        for (QString t : tiers) {
            if (t.contains("T1"))
                qDebug() <<fa->name() <<  t;
        }
    }

}

//===========================================================================
void ALICE::listFA()
{
    // list all funding agencies with attributes

    for (FundingAgency *fa : mFAs)
        fa->list();
}

//===========================================================================
void ALICE::organizeFA()
{
    // organize FAs, clustering etc...

    // and for Brazil
    FundingAgency * brazil = new FundingAgency("*Brazil", FundingAgency::kNMS);
    brazil->addFA(searchFA("Brazil"));
    brazil->addFA(searchFA("Brazil UFRGS"));
    mFAs.append(brazil);

    // France
    FundingAgency * france = new FundingAgency("*France", FundingAgency::kMS);
    france->addFA(searchFA("France-CEA"));
    france->addFA(searchFA("France-IN2P3/CNRS"));
    mFAs.append(france);

    // and for Germany
    FundingAgency * germany = new FundingAgency("*Germany", FundingAgency::kMS);
    germany->addFA(searchFA("Germany-BMBF"));
    germany->addFA(searchFA("Germany-GSI"));
    mFAs.append(germany);

    // and for Italy
    FundingAgency * italy = new FundingAgency("*Italy", FundingAgency::kMS);
    italy->addFA(searchFA("Italy-Centro Fermi"));
    italy->addFA(searchFA("Italy-INFN"));
    mFAs.append(italy);

    // and for Japan
    FundingAgency * japan = new FundingAgency("*Japan", FundingAgency::kNMS);
    japan->addFA(searchFA("Japan Nagasaki"));
    japan->addFA(searchFA("Japan-MEXT"));
    japan->addFA(searchFA("Japan RIKEN"));
    mFAs.append(japan);

    //  Nordic countries
    FundingAgency * nordic = new FundingAgency("*Nordic", FundingAgency::kMS);
    nordic->addFA(searchFA("Denmark"));
    nordic->addFA(searchFA("Finland"));
    nordic->addFA(searchFA("Norway"));
//    nordic->addFA(searchFA("Sweden"));
    mFAs.append(nordic);

    // Rep of Korea
    FundingAgency * korea = new FundingAgency("*Republic of Korea", FundingAgency::kMS);
    korea->addFA(searchFA("Rep. Korea-KISTI"));
    korea->addFA(searchFA("Rep. Korea-NRF"));
    mFAs.append(korea);

    // and for Romania
    FundingAgency * romania = new FundingAgency("*Romania", FundingAgency::kMS);
    romania->addFA(searchFA("Romania-ISS"));
    romania->addFA(searchFA("Romania-NIPNE"));
    mFAs.append(romania);

    // and for Thailand
    FundingAgency * thailand = new FundingAgency("*Thailand", FundingAgency::kNMS);
    thailand->addFA(searchFA("Thailand-KMUTT"));
    thailand->addFA(searchFA("Thailand-SUT"));
    thailand->addFA(searchFA("Thailand-TMEC"));
    mFAs.append(thailand);

    // and for USA
    FundingAgency * usa = new FundingAgency("*USA", FundingAgency::kNMS);
    usa->addFA(searchFA("USA-DOENP"));
    usa->addFA(searchFA("USA-NSF"));
    mFAs.append(usa);

}

//===========================================================================
ALICE::ALICE(QObject *parent) : QObject(parent),
    mDrawTable(true),  mCurrentPledgedYear(""), mCurrentRequirementYear("")
{
    // ctor
    setObjectName("The ALICE Collaboration");
    mModel = new QStandardItemModel(this);
    qDeleteAll(mFAs.begin(), mFAs.end());
    mFAs.clear();
    qDeleteAll(mLastRow.begin(), mLastRow.end());
    mLastRow.clear();
    mT0Pledged.clear();
    mT1Pledged.clear();
    mT2Pledged.clear();
    mToPledged.clear();
    mT0Required.clear();
    mT1Required.clear();
    mT2Required.clear();
    mToRequired.clear();
    mT0Used.clear();
    mT1Used.clear();
    mT2Used.clear();
    mToUsed.clear();
}

//===========================================================================
qint32 ALICE::countMOPayers() const
{
    // count all the M&O payers
    qint32 count = 0;
    for (FundingAgency *fa : mFAs)
        if (fa->objectName().left(1) != "-")
            count += fa->payers();
    return count;
}

//===========================================================================
qint32 ALICE::countMOPayersT() const
{
    // count M&O payers in T1

    qint32 count = 0;
    for (FundingAgency *fa : mFAs)
        if (fa->objectName().left(1) != "-" && fa->hasT1())
            count += fa->payers();
    return count;

}
