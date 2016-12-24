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
#include "tier.h"

ALICE ALICE::mInstance = ALICE();

//===========================================================================
ALICE &ALICE::instance()
{
    return mInstance;
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

//    // draw the table in a view
//    drawTable();

//    // save it to a csv file

//    saveCSV(year);


    // TEMPORARY DISABLE
}

//===========================================================================
void ALICE::drawTable()
{
    // draw the table

    // first add a few FAs clustering FAs per country

    mModel->removeColumns(0, mModel->columnCount());

    qint32 row = 0;
    qint32 norm = countMOPayers();
    qint32 normT = countMOPayersT();

    for (FundingAgency *fa : mFAs) {
        QList<QStandardItem*> oneRow;
        oneRow.insert(kStatC, new QStandardItem(fa->status()));
        oneRow.insert(kFAC, new QStandardItem(fa->name()));
        oneRow.insert(kMOC, new QStandardItem(QString("%1").arg(fa->payers())));
        if (fa->name() == "CERN") {
            norm -= fa->payers();  // CERN is not counted in the M&O payers
            oneRow.insert(kConC, new QStandardItem(QString("-")));
        } else {
            double frac = (double)fa->payers() * 100. / norm;
            fa->setContrib(frac);
            double fracT = (double)fa->payers() * 100. / normT;
            fa->setContribT(fracT);
            oneRow.insert(kConC, new QStandardItem(QString("%1").arg(frac, 4, 'f', 2)));
        }
        mModel->insertRow(row, oneRow);
        for (qint32 index = 0; index <= kConC; index++)
            mModel->item(row, index)->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        oneRow.clear();
        row++;
    }

    mModel->setHorizontalHeaderItem(kStatC, new QStandardItem(tr("Status")));
    mModel->setHorizontalHeaderItem(kFAC,   new QStandardItem(tr("Funding Agency")));
    mModel->setHorizontalHeaderItem(kMOC,   new QStandardItem(tr("M&O Payers")));
    mModel->setHorizontalHeaderItem(kConC,  new QStandardItem(tr("Contribution(%)")));

    mT0Required.setObjectName(QString("Required Resources at T0"));
    mT1Required.setObjectName(QString("Required Resources at T1"));
    mT2Required.setObjectName(QString("Required Resources at T2"));

    // calculate requirements, display pledges and calculate (-requirements + pledges) / requirements

    QList<QStandardItem*> lcpuRColumn;
    QList<QStandardItem*> ldiskRColumn;
    QList<QStandardItem*> ltapeRColumn;
    QList<QStandardItem*> lcpuPColumn;
    QList<QStandardItem*> ldiskPColumn;
    QList<QStandardItem*> ltapePColumn;
    QList<QStandardItem*> lcpuDColumn;
    QList<QStandardItem*> ldiskDColumn;
    QList<QStandardItem*> ltapeDColumn;

    for (FundingAgency *fa : mFAs) {
        double cpuR;
        double diskR;
        double tapeR;
        QString faName = fa->name();
        if (faName == "CERN") {
            cpuR  = mT0Required.getCPU();
            diskR = mT0Required.getDisk();
            tapeR = mT0Required.getTape();
        } else {
            double mult = fa->contrib() / 100.;
            double multT = fa->contribT() / 100.;
            if (fa->name().left(1) == "-") {
                mult  = 0.0;
                multT = 0.0;
            }
            cpuR  = (mT1Required.getCPU()  + mT2Required.getCPU())  * mult;
            diskR = (mT1Required.getDisk() + mT2Required.getDisk()) * mult;
            tapeR = (mT1Required.getTape() + mT2Required.getTape()) * multT;
        }
        // the requirements
        fa->setRequired(cpuR, diskR, tapeR);
        QStandardItem *cpuSIR = new QStandardItem(QString("%1").arg(cpuR, 5, 'f', 2));
        cpuSIR->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        lcpuRColumn.append(cpuSIR);
        QStandardItem *diskSIR = new QStandardItem(QString("%1").arg(diskR, 5, 'f', 2));
        diskSIR->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        ldiskRColumn.append(diskSIR);
        QStandardItem *tapeSIR = new QStandardItem(QString("%1").arg(tapeR, 5, 'f', 2));
        tapeSIR->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        ltapeRColumn.append(tapeSIR);

        // the pledges
        QStandardItem *cpuSIP = new QStandardItem(QString("%1").arg(fa->getPledgedCPU(), 5, 'f', 2));
        cpuSIP->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        lcpuPColumn.append(cpuSIP);
        QStandardItem *diskSIP = new QStandardItem(QString("%1").arg(fa->getPledgedDisk(), 5, 'f', 2));
        diskSIP->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        ldiskPColumn.append(diskSIP);
        QStandardItem *tapeSIP = new QStandardItem(QString("%1").arg(fa->getPledgedTape(), 5, 'f', 2));
        tapeSIP->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        ltapePColumn.append(tapeSIP);

        // the difference
        double diff =  100 * (fa->getPledgedCPU() - fa->getRequiredCPU()) / fa->getRequiredCPU();
        QStandardItem *cpuSID = new QStandardItem(QString("%1").arg(diff, 5, 'f', 0));
        cpuSID->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (diff < -20)
            cpuSID->setForeground(QBrush(Qt::red));
        else
            cpuSID->setForeground(QBrush(Qt::green));
        lcpuDColumn.append(cpuSID);
        diff =  100 * (fa->getPledgedDisk() - fa->getRequiredDisk()) / fa->getRequiredDisk();
        QStandardItem *diskSID = new QStandardItem(QString("%1").arg(diff, 5, 'f', 0));
        diskSID->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (diff < -20)
            diskSID->setForeground(QBrush(Qt::red));
        else
            diskSID->setForeground(QBrush(Qt::green));
        ldiskDColumn.append(diskSID);
        if (fa->getRequiredTape() != 0.0)
            diff =  100 * (fa->getPledgedTape() - fa->getRequiredTape()) / fa->getRequiredTape();
        else
            diff = 0.0;
        QStandardItem *tapeSID = new QStandardItem(QString("%1").arg(diff, 5, 'f', 0));
        tapeSID->setTextAlignment(Qt::AlignRight | Qt::AlignVCenter);
        if (diff < -20)
            tapeSID->setForeground(QBrush(Qt::red));
        else
            tapeSID->setForeground(QBrush(Qt::green));
        ltapeDColumn.append(tapeSID);
    }

    mModel->insertColumn(kConC + 1, lcpuRColumn);
    mModel->setHorizontalHeaderItem(kConC + 1, new QStandardItem(tr("Required CPU (kHEPSPEC06)")));
    mModel->insertColumn(kConC + 2, ldiskRColumn);
    mModel->setHorizontalHeaderItem(kConC + 2, new QStandardItem(tr("Required Disk (PB)")));
    mModel->insertColumn(kConC + 3, ltapeRColumn);
    mModel->setHorizontalHeaderItem(kConC + 3, new QStandardItem(tr("Required Tape (PB)")));

    // the pledges

    mModel->insertColumn(kConC + 4, lcpuPColumn);
    mModel->setHorizontalHeaderItem(kConC + 4, new QStandardItem(tr("Pledged CPU (kHEPSPEC06)")));
    mModel->insertColumn(kConC + 5, ldiskPColumn);
    mModel->setHorizontalHeaderItem(kConC + 5, new QStandardItem(tr("Pledged Disk (PB)")));
    mModel->insertColumn(kConC + 6, ltapePColumn);
    mModel->setHorizontalHeaderItem(kConC + 6, new QStandardItem(tr("Pledged Tape (PB)")));

    // difference required and pledges


    mModel->insertColumn(kConC + 7, lcpuDColumn);
    mModel->setHorizontalHeaderItem(kConC + 7, new QStandardItem(tr("Diff CPU (%)")));
    mModel->insertColumn(kConC + 8, ldiskDColumn);
    mModel->setHorizontalHeaderItem(kConC + 8, new QStandardItem(tr("Diff Disk (%)")));
    mModel->insertColumn(kConC + 9, ltapeDColumn);
    mModel->setHorizontalHeaderItem(kConC + 9, new QStandardItem(tr("Diff Tape (%)")));

    lcpuRColumn.clear();
    ldiskRColumn.clear();
    ltapeRColumn.clear();
    lcpuPColumn.clear();
    ldiskPColumn.clear();
    ltapePColumn.clear();
    lcpuDColumn.clear();
    ldiskDColumn.clear();
    ltapeDColumn.clear();

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
bool ALICE::readRebus(const QString &year)
{
//    Associate sites to Funding Agencies and collect the pledges

    mT0Pledged.clear();
    mT1Pledged.clear();
    mT2Pledged.clear();
    mT0Pledged.setObjectName(QString("Pledged Resources at T0 in %1").arg(year));
    mT1Pledged.setObjectName(QString("Pledged Resources at T1 in %1").arg(year));
    mT2Pledged.setObjectName(QString("Pledged Resources at T2 in %1").arg(year));

    QFile csvFile(QString(":/data/%1/pledges.csv").arg(year));
    if (!csvFile.open(QIODevice::ReadOnly))
        return false;
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
            FundingAgency *fa = searchFA(strList.at(1));
            QString sCPU = strList.at(aliceColumn + diff);
            if (strList.at(4 + diff) == "HEP-SPEC06")
                cunit = Resources::HEPSPEC06;
            else
                qFatal("revise the csv format");
            res.setCPU(sCPU.toDouble(), cunit);
            addCPU(res.getCPU());
            line = csvFile.readLine();
            strList = line.split(',');
            qint32 diff = strList.size() - nbColumn;
            QString sDisk = strList.at(aliceColumn + diff);
            if (strList.at(4 + diff) == "Tbytes")
                sunit = Resources::TB;
            else
                qFatal("revise the csv format");
            res.setDisk(sDisk.toDouble(), sunit);
            addDisk(res.getDisk());
            Tier::TierCat cat = Tier::kT2;
            if (strList.at(0) == "Tier 0" || strList.at(0) == "Tier 1") {
                if (strList.at(0) == "Tier 0")
                    cat = Tier::kT0;
                else
                    cat = Tier::kT1;
                line = csvFile.readLine();
                strList = line.split(',');
                qint32 diff = strList.size() - nbColumn;
                QString sTape = strList.at(aliceColumn + diff);
                if (strList.at(4 + diff) == "Tbytes")
                    sunit = Resources::TB;
                else
                    qFatal("revise the csv format");
                res.setTape(sTape.toDouble(), sunit);

                addTape(res.getTape());
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

    if (MainWindow::isDebug())
        for (FundingAgency *fa : mFAs)
            fa->list();
    if (MainWindow::isDebug()) {
        mT0Pledged.list();
        mT1Pledged.list();
        mT2Pledged.list();
    }
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

    const QString cpuName("CPU");
    const QString diskName("Disk");
    const QString tapeName("Tape");
    const QString t0Name("T0");
    const QString t1Name("T1");
    const QString t2Name("T2");

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
        }
        row++;
    }

    csvFile.close();



    if (MainWindow::isDebug()) {
        mT0Required.list();
        mT1Required.list();
        mT2Required.list();
    }

    return true;
}

//===========================================================================
bool ALICE::readMonthlyReport(const QDate &date)
{
    qint32 hours = date.daysInMonth() * 24;
    QString month = date.toString("MMMM");

    // First read the monthly report provided by EGI (http://accounting.egi.eu/egi.php)
    // one file for T0+T1s and one for T2s
    // select Norm. Sum CPU (HEPSPEC06-hours)
    // format T0+T1s
    // line 1-4: header to be skipped
    // line 5: header TIER1,"alice","atlas","cms","lhcb",Total
    // last line      Total, xxxxx (HEPSPEC06-hours)

    double totalCPU = 0.0;

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
            Resources *res = new Resources;
            double rcpu = cpu / hours / 1000.;
            totalCPU += rcpu;
            res->setCPU(rcpu, Resources::kHEPSPEC06);
            tier->addUsedResources(month, res);
        }
    }
    csvFile.close();

    // format T2s
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
    while(!csvFile.atEnd()) {
        line = csvFile.readLine();
        strList = line.split(',');
        QString country = strList.at(0);
        if (country == "")
            continue;
        QString federation = strList.at(1);
        QString site = strList.at(aliceColumn - 1);
        if (federation.contains("Total"))
            break;
        if (strList.at(aliceColumn) != "0") {
            QString scpu  = strList.at(aliceColumn);
            double cpu = scpu.toDouble();
            FundingAgency *fa = searchFA(country);
            qDebug() << Q_FUNC_INFO << country;
            Tier *tier = fa->search(federation, true);
            if (!tier) {
                qWarning() << Q_FUNC_INFO << federation << " not found!";
                exit(1);
            }
            Resources *res = new Resources;
            double rcpu = cpu / hours / 1000.;
            totalCPU += rcpu;
            res->setCPU(rcpu, Resources::kHEPSPEC06);
            tier->addUsedResources(month, res);
        }
    }
    csvFile.close();

    // complete the table
    QList<QStandardItem*> lcpuUColumn;
    QList<QStandardItem*> ldiskUColumn;
    QList<QStandardItem*> ltapeUColumn;
    for (FundingAgency *fa : mFAs) {
        double cpuU  = fa->getUsed(month)->getCPU();
        double diskU = fa->getUsed(month)->getDisk();
        double tapeU = fa->getUsed(month)->getTape();
        QStandardItem *cpuSIU  = new QStandardItem(QString("%1").arg(cpuU,  5, 'f', 2));
        QStandardItem *diskSIU = new QStandardItem(QString("%1").arg(diskU, 5, 'f', 2));
        QStandardItem *tapeSIU = new QStandardItem(QString("%1").arg(tapeU, 5, 'f', 2));
        lcpuUColumn.append(cpuSIU);
        ldiskUColumn.append(diskSIU);
        ltapeUColumn.append(tapeSIU);
    }
    QStandardItem *totalcpuSIU  = new QStandardItem(QString("%1").arg(totalCPU,  5, 'f', 2));
    lcpuUColumn.append(totalcpuSIU);
    mModel->appendColumn(lcpuUColumn);
    QString header = QString("%1 \n Used CPU (kHEPSPEC06)").arg(month);
    mModel->setHorizontalHeaderItem(mModel->columnCount() -1, new QStandardItem(header));
    mModel->appendColumn(ldiskUColumn);
    header = QString("%1 \n Used Disk (PB)").arg(month);
    mModel->setHorizontalHeaderItem(mModel->columnCount() - 1, new QStandardItem(header));
    mModel->appendColumn(ltapeUColumn);
    header = QString("%1 \n Used Tape (PB)").arg(month);
    mModel->setHorizontalHeaderItem(mModel->columnCount() - 1, new QStandardItem(header));

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

    QHash<QString, double> storages; // QString is the SE name taken from the header of the file

    fileName = QString(":/data/%1/%2/Disk_Tape_Usage.csv").arg(date.year()).arg(date.month());
    csvFile.setFileName(fileName);
    if(!csvFile.open(QIODevice::ReadOnly))
        return false;
    line = csvFile.readLine();
    QStringList listSE = line.split(',');
    for (QString se : listSE )
         storages[se] = 0.;
    qDebug() << Q_FUNC_INFO << mFAs;


    csvFile.close();
    // consider making a dictionary with all the names
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
    qWarning() << Q_FUNC_INFO << QString("FA %1 not found").arg(name);
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
void ALICE::setCEandSE(const QString &year)
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
ALICE::ALICE(QObject *parent) : QObject(parent)
{
    // ctor
    setObjectName("The ALICE Collaboration");
    mModel = new QStandardItemModel(this);
}

//===========================================================================
qint32 ALICE::countMOPayers() const
{
    // count all the M&O payers
    qint32 count = 0;
    for (FundingAgency *fa : mFAs)
        if (fa->objectName().left(1) != "*")
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
