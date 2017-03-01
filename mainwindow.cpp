// Y. Schutz November 2016

#include <QAction>
#include <QAreaSeries>
#include <QBarCategoryAxis>
#include <QBarSeries>
#include <QBarSet>
#include <QCalendarWidget>
#include <QCategoryAxis>
#include <QChart>
#include <QChartView>
#include <QtCore>
#include <QDateEdit>
#include <QDateTime>
#include <QDateTimeAxis>
#include <QDebug>
#include <QDir>
#include <QHeaderView>
#include <QLabel>
#include <QLegendMarker>
#include <QLineSeries>
#include <QMenuBar>
#include <QMessageBox>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QPdfWriter>
#include <QPicture>
#include <QPrinter>
#include <QProgressBar>
#include <QSplitter>
#include <QScatterSeries>
#include <QSslConfiguration>
#include <QStatusBar>
#include <QToolBar>
#include <QValueAxis>
#include <QVBarModelMapper>
#include <QVBoxLayout>
#include <QVXYModelMapper>

#include "consolewidget.h"
#include "logger.h"
#include "mainwindow.h"
#include "mymdiarea.h"
#include "pltablemodel.h"
#include "qfonticon.h"

QT_CHARTS_USE_NAMESPACE

bool MainWindow::mDebug = false;

//===========================================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // ctor

    mDebug = false;
    mDownLoadText      = Q_NULLPTR;
    mNetworkManager    = Q_NULLPTR;
    mProgressBar       = Q_NULLPTR;
    mProgressBarWidget = Q_NULLPTR;
    mTableConsol       = Q_NULLPTR;
    mURL            = "";
    setGeometry(0,0, 50, 25);

    setWindowTitle("ALICE Computing Resources");

    QWidget *topFiller = new QWidget(this);
    topFiller->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QWidget *intermediateFiller = new QWidget(this);
    intermediateFiller->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QWidget *bottomFiller = new QWidget;
    bottomFiller->setSizePolicy(QSizePolicy::Expanding, QSizePolicy::Expanding);

    QVBoxLayout *layout = new QVBoxLayout;
    layout->setMargin(5);
    layout->addWidget(topFiller);
    layout->addWidget(intermediateFiller);
    layout->addWidget(bottomFiller);

    mMdiArea = new MyMdiArea(this);
    mMdiArea->setViewMode(QMdiArea::TabbedView);
    setCentralWidget(mMdiArea);

    createConsol();

    createActions();
    createMenu();

    QString message = tr("Welcome to ALICE Computing Resources tool");
    statusBar()->showMessage(message);

    // the toolbars
    QToolBar *tb = new QToolBar(this);

    // icons if at http://fontawesome.io/icons/
    // the print icon (prints the current widow)

    QAction *pract = new QAction(QFontIcon::icon(0xf02f), "print");
    pract->setWhatsThis("print current window");
    connect(pract, SIGNAL(triggered(bool)), this, SLOT(printCurrentWindow()));
    tb->addAction(pract);

    addToolBar(tb);


    qint32 w = 1800;
    qint32 h = 1000;
    resize(w, h);

}

//===========================================================================
MainWindow::~MainWindow()
{
    // dtor
    delete mMdiArea;
}

//===========================================================================
void MainWindow::list(ALICE::ListOptions val)
{
    // list various stuff depending on val

    switch (val) {
    case ALICE::kFA:
        ALICE::instance().listFA();
        break;
    default:
        break;
    }

}

//===========================================================================
void MainWindow::paintEvent(QPaintEvent *event)
{
    QMainWindow::paintEvent(event);
}

//===========================================================================
void MainWindow::doit(qint32 opt)
{
    // does various things upon request
    switch (opt) {
    case kTheBigTable:
        selectDate(kTheBigTable);
        break;
    case kGetReport:
        selectDate(kGetReport);
        break;
    default:
        break;
    }
}

//===========================================================================
void MainWindow::findAName()
{   //FIXME: Find a suitable name for this method
    // display data retrieved from mURL

    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    QByteArray data = reply->readAll();
    if (data.isEmpty()) {
        qCritical() << Q_FUNC_INFO << "no data found";
        return;
    }

    QTextStream instream(&data);
    QString line;
    while (instream.readLineInto(&line)) {
        qDebug() << Q_FUNC_INFO << line;
    }
}

//===========================================================================
void MainWindow::onTableClicked(const QModelIndex & index)
{
    // do something when cell is clicked

    if (index.isValid()) {
         QString cellText = index.data().toString();
         if (index.column() == 1) {
             FundingAgency * fa =  ALICE::instance().searchFA(cellText);
             QMessageBox::about(this, cellText, fa->list());
         }
    }
}

//===========================================================================
void MainWindow::parsePlotUrlFile(PlotOptions opt)
{
    // read the csv file collected from MonALISA

    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    QByteArray data = reply->readAll();
    if (data.isEmpty())
        return;

    mDownLoadText->setText("DONE");

    setProgressBar(false);

    QTextStream stream(&data);
    QString line;

    qDeleteAll(mPlData.begin(), mPlData.end());
    mPlData.clear();
    mPlDataName.clear();

    QVector<qint64> dates;
    stream.readLineInto(&line); //the header line
    QStringList strList = line.split(",");
    for (QString str : strList)
        mPlDataName.append(str);
    while (stream.readLineInto(&line)) {
        QVector<double> *dataVec = new QVector<double>(strList.size());
        QStringList strlist = line.split(",");
        QString date = strlist.at(0);
        dates.append(date.toLongLong());
        for (qint32 index = 1; index < strList.size(); index++) {
            QString sdata = strlist.at(index);
            double data   = sdata.toDouble();
            dataVec->replace(index, data);
        }
        mPlData.append(dataVec);
    }
    stream.reset();

    QDateTime today(QDateTime::currentDateTime());
    qint32 index = 0;
    for (qint64 date : dates) {
        qint64 days = (dates.last() - date) / 3600. / 24.;
        QDateTime ddate(today.addDays(-days));
        QVector<double> *dataVec = mPlData.at(index++);
        dataVec->replace(0, (double)ddate.toMSecsSinceEpoch());
    }

    switch (opt) {
    case kRegisteredDataProfile:
        plRegisteredData(opt);
        break;
    case kTierEfficiencyProfile:
        plTierEfficiency(kTierEfficiencyProfile);
        break;
    case kUserEfficiencyProfile:
        plUserEfficiency(kUserEfficiencyProfile);
        break;
    case kPledgesProfile:
        saveData(opt);
        break;
    case kEventSizeProfile:
        plProfileEventSize();
        break;
    default:
        break;
    }
}

//===========================================================================
void MainWindow::showNetworkError(QNetworkReply::NetworkError er)
{
    // in ase of network error, popup a message
    Q_UNUSED(er);

    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    mDownLoadText->setText(QString("%1").arg(reply->errorString()));
}

//===========================================================================
void MainWindow::transferProgress(qint64 readBytes, qint64 totalBytes)
{
    // update the progress bar while transferring data
      mProgressBar->setMaximum(totalBytes);
      mProgressBar->setValue(readBytes);
}

//===========================================================================
void MainWindow::validateDate(ActionOptions opt)
{
    //sets date and launches data collection from url
    qobject_cast<QWidget*>(sender()->parent())->close();

    QDate date = mDEStart->date();

    switch (opt) {
    case kTheBigTable:
        doeReqAndPle(QString::number(date.year()));
        break;
    case kGetReport:
        readMonthlyReport(date);
        break;
    default:
        break;
    }
}

//===========================================================================
void MainWindow::validateDates(PlotOptions opt)
{
    //sets start and end date and launches data collection from url
    qobject_cast<QWidget*>(sender()->parent())->close();

    QDate dStart = mDEStart->date();
    QDate dEnd   = mDEEnd->date();
    qint64 diffS = dStart.daysTo(QDate::currentDate()) * 24 * 3600 * 1000;
    qint64 diffE = dEnd.daysTo(QDate::currentDate()) * 24 *3600 * 1000;

    switch (opt) {
    case kMandOProfile:
    {
        plProfileMandO();

        break;
    }
    case kRequirementsProfile:
    {
        mURL = ALICE::instance().dataURL();
        plProfile(kRequirementsProfile, Resources::kCPU);
        plProfile(kRequirementsProfile, Resources::kDISK);
        plProfile(kRequirementsProfile, Resources::kTAPE);

        break;
    }

    case kPledgesProfile:
    {
        mURL = ALICE::instance().dataURL();
        plProfile(kPledgesProfile, Resources::kCPU);
        plProfile(kPledgesProfile, Resources::kDISK);
        plProfile(kPledgesProfile, Resources::kTAPE);

        break;
    }

    case kRegisteredDataProfile:
    {
        mURL = QString("http://alimonitor.cern.ch/display");
        mURL += "?imgsize=1024x600";
        mURL += QString("&interval.max=%1&interval.min=%2").arg(diffS).arg(diffE);
        mURL += "&page=DAQ2%2Fdaq_size&plot_series=aldaqgw01-daq03.cern.ch&plot_series=aldaqgw02-daq03.cern.ch&download_data_csv=true";
        getDataFromWeb(kRegisteredDataProfile);
        break;
    }
    case kUsageProfile:
    {
        mURL = ALICE::instance().dataURL();
        plProfile(kUsageProfile, Resources::kCPU);
        plProfile(kUsageProfile, Resources::kDISK);
        plProfile(kUsageProfile, Resources::kTAPE);
        break;
    }
    case kUsage_PledgesProfile:
    {
        mURL = ALICE::instance().dataURL();
        plProfile(kUsage_PledgesProfile, Resources::kCPU);
        plProfile(kUsage_PledgesProfile, Resources::kDISK);
        plProfile(kUsage_PledgesProfile, Resources::kTAPE);
        break;
    }
    case kUsage_RequiredProfile:
    {
        mURL = ALICE::instance().dataURL();
        plProfile(kUsage_RequiredProfile, Resources::kCPU);
        plProfile(kUsage_RequiredProfile, Resources::kDISK);
        plProfile(kUsage_RequiredProfile, Resources::kTAPE);
        break;
    }
    case kTierEfficiencyProfile:
    {
        mURL = QString("http://alimonitor.cern.ch/display");
        mURL += "?annotation.enabled=true&imgsize=1280x700";
        mURL += QString("&interval.max=%1&interval.min=%2").arg(diffS).arg(diffE);
        mURL += "&page=jobResUsageSum_eff_time&download_data_csv=true";
        getDataFromWeb(kTierEfficiencyProfile);
        break;
    }
    case kUserEfficiencyProfile:
    {
        mURL = QString("http://alimonitor.cern.ch/display");
        mURL += "?annotation.enabled=true&imgsize=1280x700";
        mURL += QString("&interval.max=%1&interval.min=%2").arg(diffS).arg(diffE);
        mURL += "&page=jpu%2Fefficiency&download_data_csv=true";
        getDataFromWeb(kUserEfficiencyProfile);
        break;
    } 
    case kPledgedRequiredUsed:
    {
        mURL = ALICE::instance().dataURL();
        plBarchart(Resources::kCPU);
        plBarchart(Resources::kDISK);
        plBarchart(Resources::kTAPE);
      break;
    }
    default:
        break;

    }
}

//===========================================================================
void MainWindow::validateDates(LoadOptions opt)
{
    //sets start and end date and launches data collection from url

    qobject_cast<QWidget*>(sender()->parent())->close();

    switch (opt) {
    case kEGICPUReportT1:
    {
        QDate dStart = mDEStart->date();
        QDate dEnd   = mDEEnd->date();
        loadUsageWLCG(dStart, dEnd, Tier::kT1);
        break;
    }
    case kEGICPUReportT2:
    {
        QDate dStart = mDEStart->date();
        QDate dEnd   = mDEEnd->date();
        loadUsageWLCG(dStart, dEnd, Tier::kT2);
        break;
    }
    case kMLCPUReport:
    {
        QDateTime dStart(mDEStart->date());
        QDateTime dEnd(mDEEnd->date());
        loadUsageML(kMLCPUReport, dStart, dEnd);
        break;
    }
    case kMLStorageReport:
    {
        QDateTime dStart(mDEStart->date());
        QDateTime dEnd(mDEEnd->date());
        loadUsageML(kMLStorageReport, dStart, dEnd);
        break;
    }
    case kMLRAWProd:
    {
        QDateTime dStart(mDEStart->date());
        QDateTime dEnd(mDEEnd->date());
        loadUsageML(kMLRAWProd, dStart, dEnd);
        break;
    }
    default:
        break;
    }
}

//===========================================================================
void MainWindow::createActions()
{
    // created the actions associated to the menu items

    // Debug mode on/off
    mDebugOnAction = new QAction(this);
    mDebugOffAction = new QAction(this);

    if (mDebug) {
        mDebugOnAction->setText("✓ On");
        mDebugOffAction->setText("Off");
    }
    else {
        mDebugOnAction->setText("On");
        mDebugOffAction->setText("✓ Off");
    }
    mDebugOnAction->setStatusTip(tr("Set debug mode on"));
    mDebugOffAction->setStatusTip(tr("Set debug mode off"));
    connect(mDebugOnAction, &QAction::triggered, this, [this]{ setDebugMode(true); });
    connect(mDebugOffAction, &QAction::triggered, this, [this]{ setDebugMode(false); });

    // actions
    QMetaEnum me = QMetaEnum::fromType<ActionOptions>();
    for (qint32 index = 0; index < me.keyCount(); index++) {
        QString     swhat  = me.key(index);
        swhat.remove(0, 1); // removes the "k"
        qint32      what   = me.value(index);
        QAction *act = new QAction(this);
        act->setText(QString("do %1").arg(swhat));
        connect(act, &QAction::triggered, this, [what, this]{ doit(what); });
        mLiAct.append(act);
    }

    // plots
    me = QMetaEnum::fromType<PlotOptions>();
    for (qint32 index = 0; index < me.keyCount(); index++) {
        QString     swhat  = me.key(index);
        swhat.remove(0, 1); // removes the "k"
        qint32      what   = me.value(index);
        QAction *act = new QAction(this);
        act->setText(QString("plot %1").arg(swhat));
        connect(act, &QAction::triggered, this, [what, this]{ plot(what); });
        mPlAct.append(act);
    }

    // Loads
    me = QMetaEnum::fromType<LoadOptions>();
    for (qint32 index = 0; index < me.keyCount(); index++) {
        QString     swhat  = me.key(index);
        swhat.remove(0, 1); // removes the "k"
        qint32      what   = me.value(index);
        QAction *act = new QAction(this);
        act->setText(QString("load %1").arg(swhat));
        connect(act, &QAction::triggered, this, [what, this]{ load(what); });
        mLoAct.append(act);
    }
}

//===========================================================================
void MainWindow::createConsol()
{
    // creates an area where to display subwindows for log infos, plots, etcc


    // the console for log output from qDebug, qInfo, qWarning
    mLogConsol = new ConsoleWidget(mMdiArea);
    mLogConsolView = mMdiArea->addSubWindow(mLogConsol);
    mLogConsolView->setWindowFlags(Qt::CustomizeWindowHint | Qt::WindowMinMaxButtonsHint);
    mLogConsolView->hide();
    mLogConsolView->setWindowTitle("Debugger Consol");
    connect(Logger::instance(), SIGNAL(messageReceived(QString)), mLogConsol,SLOT(setMessage(QString)));

    // direct the log info to the console
//     qInstallMessageHandler(customMessageHandler);

    // wrap up
    setCentralWidget(mMdiArea);

    Logger::write("Debugger Console");
}

//===========================================================================
void MainWindow::createMenu()
{
    // creates the menus

    // Create file Menu
       QMenu * fileMenu = menuBar()->addMenu(tr("&File"));

       fileMenu->addAction(tr("&Quit"), qApp, SLOT(closeAllWindows()), QKeySequence::Quit);

    // Debug
    mDebugMenu = menuBar()->addMenu(tr("&Debug"));
    mDebugMenu->addAction(mDebugOnAction);
    mDebugMenu->addAction(mDebugOffAction);

    // Load data
    QMenu *loMenu = menuBar()->addMenu(tr("Do various loads"));
    for (QAction * act : mLoAct)
        loMenu->addAction(act);

    // Actions
    QMenu *acMenu = menuBar()->addMenu(tr("&Actions"));
    for (QAction * act : mLiAct)
        acMenu->addAction(act);

    QMenu *plMenu = menuBar()->addMenu(tr("Do various plots"));
    for (QAction * act : mPlAct)
        plMenu->addAction(act);

}

//===========================================================================
void MainWindow::customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg)
{
    // redifinition on how to handle log messages

    QString txt;
    switch (type) {
    case QtDebugMsg:
        txt = QString("Debug<%1-%2>: %3").arg(context.function).arg(context.line).arg(msg);
        break;
    case QtInfoMsg:
        txt = QString("Info<%1>: %2").arg(context.function).arg(msg);
        break;
    case QtWarningMsg:
        txt = QString("Warning<%1-%2>: %3").arg(context.function).arg(context.line).arg(msg);
        break;
    case QtCriticalMsg:
        txt = QString("Critical:<%1-%2>: %3").arg(context.function).arg(context.line).arg(msg);
        break;
    case QtFatalMsg:
        txt = QString("Fatal:<%1-%2>: %3").arg(context.function).arg(context.line).arg(msg);
        abort();
    }

   Logger::write(txt);

}

//===========================================================================
void MainWindow::setDebugMode(bool val)
{
   // set debug mode on/off
    mDebug = val;
    if (mDebug) {
        mDebugOnAction->setText("✓ On");
        mDebugOffAction->setText("Off");
        mLogConsolView->setVisible(true);
    } else {
        mDebugOnAction->setText("On");
        mDebugOffAction->setText("✓ Off");
        mLogConsolView->setVisible(false);
    }
}

//===========================================================================
void MainWindow::doeReqAndPle(const QString &year)
{
    // the console for table view

    if (mTableConsol) {
        mMdiArea->removeSubWindow(mTableConsol);
        mTableConsol->close();
        mTableConsolView->close();
    }
    mTableConsol = new QTableView(mMdiArea);
    mTableConsol->setAttribute(Qt::WA_DeleteOnClose);
    mTableConsol->setWindowTitle(QString(tr("Ressources in %1")).arg(year));
    mTableConsol->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mTableConsol->setModel(ALICE::instance().getModel());
    mTableConsol->resizeColumnsToContents();
    mTableConsol->horizontalHeader()->setStretchLastSection(true);
    mTableConsol->activateWindow();
    mTableConsol->setAlternatingRowColors(true);
    mTableConsolView = mMdiArea->addSubWindow(mTableConsol);
    mTableConsolView->setVisible(true);
    connect(mTableConsol, SIGNAL(clicked(QModelIndex)), this, SLOT(resizeView()));

    // hide the column when header is clicked
    connect(mTableConsol->horizontalHeader(), SIGNAL(sectionClicked(int)), mTableConsol, SLOT(hideColumn(int)));

    // hide the row when row number is clicked
    connect(mTableConsol->verticalHeader(), SIGNAL(sectionClicked(int)), mTableConsol, SLOT(hideRow(int)));

    // popup a window with the clicked funding agency
    connect(mTableConsol, SIGNAL(clicked(const QModelIndex &)), this, SLOT(onTableClicked(const QModelIndex &)));

    //FixME usefull or not ?
    mTableConsol->addAction(new QAction("Salut"));
    mTableConsol->setContextMenuPolicy(Qt::ActionsContextMenu);

    // read M&O information from glance
    ALICE::instance().doReqAndPle(year);
}

//===========================================================================
void MainWindow::keyPressEvent(QKeyEvent *event)
{
    // do something when a key is pressed

    switch (event->key())
    case 80: {// key cmd-p to print the active window
        printCurrentWindow();
        break;
    default:
        break;
        }
}

//===========================================================================
void MainWindow::load(qint32 opt)
{
    // select various optoions for loading data
    switch (opt) {
    case kEGICPUReportT1:
        selectDates(kEGICPUReportT1);
        break;
    case kEGICPUReportT2:
        selectDates(kEGICPUReportT2);
        break;
    case kMLCPUReport:
        selectDates(kMLCPUReport);
        break;
    case kMLStorageReport:
        selectDates(kMLStorageReport);
        break;
    case kMLRAWProd:
        selectDates(kMLRAWProd);
        break;
    default:
        break;
    }

}

//===========================================================================
void MainWindow::mousePressEvent(QMouseEvent *event)
{
    Q_UNUSED(event);
}

//===========================================================================
void MainWindow::loadUsageML(LoadOptions opt, QDateTime dateS, QDateTime dateE)
{
    // load CPU or storage usage from MonALISA
    // CPU:     http://alimonitor.cern.ch/display?annotation.enabled=true&imgsize=1024x600&interval.max=0&interval.min=2628000000&page=jobResUsageSum_time_si2k&download_data_csv=true
    // Storage: http://alimonitor.cern.ch/display?&interval.max=0&interval.min=30301200000&job_stats.owner=brijesh&modules=SE%2Fhist_used&page=SE%2Fhist&download_data_csv=true

    QDateTime today = QDateTime(QDate::currentDate());
    qint32 max = qAbs(today.msecsTo(dateS));
    qint32 min = qAbs(today.msecsTo(dateE));
    switch (opt) {
    case kMLCPUReport:
        mURL = QString("http://alimonitor.cern.ch/display?interval.max=%1&interval.min=%2&page=jobResUsageSum_time_si2k&download_data_csv=true").arg(max).arg(min);
        break;
    case kMLStorageReport:
        mURL = QString("http://alimonitor.cern.ch/display?&interval.max=%1&interval.min=%2").arg(max).arg(min);
        mURL.append("&job_stats.owner=brijesh&modules=SE%2Fhist_used&page=SE%2Fhist&download_data_csv=true");
        break;
    case kMLRAWProd:
        mURL = QString("https://alimonitor.cern.ch/production/raw.jsp?download_data_csv=true");
        break;
    default:
        break;
    }
    getDataFromWeb(dateS.date(), opt);
}

//===========================================================================
void MainWindow::loadUsageWLCG(QDate dateS, QDate dateE, Tier::TierCat cat)
{
   // load the CPU usage from EGI (new portal).
   // Tier1: https://accounting-support.egi.eu/custom_xml.php?query=normcpu&option=TIER1&sYear=yyyy&sMonth=mm&eYear=yyyy&eMonth=mm&yrange=TIER1&xrange=VO&groupVO=lhc&localJobs=onlyinfrajobs&tree=TIER1&optval=&csv=true
   // Tier2: https://accounting-support.egi.eu/custom_xml.php?query=normcpu&option=COUNTRY_T2&sYear=yyyy&sMonth=mm&eYear=yyyy&eMonth=mm&yrange=FEDERATION&xrange=VO&groupVO=egi&groupVO=egi&localJobs=onlyinfrajobs&tree=TIER2&optval=&csv=true

    qint32 sYear = dateS.year();
    qint32 eYear = dateE.year();
    qint32 sMonth = dateS.month();
    qint32 eMonth = dateE.month();
    QString scat;
    switch (cat) {
    case Tier::kT0:
        scat = "TIER1";
        break;
    case Tier::kT1:
        scat = "TIER1";
        break;
    case Tier::kT2:
        scat = "TIER2";
        break;
    default:
        break;
    }

    scat.remove("k");
    scat = scat.toUpper();
    switch (cat) {
    case Tier::kT0:
        mURL = QString("https://accounting-support.egi.eu/custom_xml.php?query=normcpu&option=%1&"
                       "sYear=%2&sMonth=%3&eYear=%4&eMonth=%5&yrange=%1&xrange=VO&"
                       "groupVO=lhc&localJobs=onlyinfrajobs&tree=%1&optval=&csv=true").
                        arg(scat).arg(sYear).arg(sMonth).arg(eYear).arg(eMonth);
        break;
    case Tier::kT1:
        mURL = QString("https://accounting-support.egi.eu/custom_xml.php?query=normcpu&option=%1&"
                       "sYear=%2&sMonth=%3&eYear=%4&eMonth=%5&yrange=%1&xrange=VO&"
                       "groupVO=lhc&localJobs=onlyinfrajobs&tree=%1&optval=&csv=true").
                        arg(scat).arg(sYear).arg(sMonth).arg(eYear).arg(eMonth);
        break;
    case Tier::kT2:
        mURL = QString("https://accounting-support.egi.eu/custom_xml.php?query=normcpu&option=COUNTRY_T2&"
                       "sYear=%1&sMonth=%2&eYear=%3&eMonth=%4&yrange=FEDERATION&xrange=VO&groupVO=egi&"
                       "groupVO=egi&localJobs=onlyinfrajobs&tree=%5&optval=&csv=true").
                        arg(sYear).arg(sMonth).arg(eYear).arg(eMonth).arg(scat);
        break;
    default:
        break;
    }

    getDataFromWeb(dateS, cat);
}

//===========================================================================
void MainWindow::plBarchart(Resources::Resources_type type)
{
    // Draw a bar chart for Pledged/Required/Used per year

    QMetaEnum me = QMetaEnum::fromType<Resources::Resources_type>();
    QString swhat = me.key(type);
    swhat.remove(0, 1);

    PlTableModel *model = new PlTableModel();

    int years = mDEEnd->date().year() - mDEStart->date().year() + 1;

    // get the data
    qint32 columns = 5; // year in ms; year; Pledged; Required; Used
    qint32 rows   = years;
    model->setColRow(columns, rows);
    qint32 colYears    = 0;
    qint32 colYear     = 1;
    qint32 colPledged  = 2;
    qint32 colRequired = 3;
    qint32 colUsed     = 4;
    QVector<QString> headers(columns);
    headers.replace(colYears, "Year in ms");
    headers.replace(colYear, "Year");
    headers.replace(colPledged, "Pledged");
    headers.replace(colRequired, "Required");
    headers.replace(colUsed, "Used");
    model->setHeader(headers);

    setProgressBar();
    int progressCount = 0;

    QVector<double> *dataVec = new QVector<double>(columns - 2);
    QStringList categories;
    QDate date = mDEStart->date();
    for (int year = mDEStart->date().year(); year <= mDEEnd->date().year(); year++) {
        categories.append(QString::number(year));
        dataVec->replace(colPledged - 2, ALICE::instance().getPledged(Tier::kTOTS, type, QString::number(year)));
        dataVec->replace(colRequired - 2, ALICE::instance().getRequired(Tier::kTOTS, type, QString::number(year)));
        double used = -1.;
        double value = 0.0;
        int    weight = 1;
        int months = date.daysTo(mDEEnd->date()) / 30;
        while (date <= QDate(year, 12, 21) && date <= mDEEnd->date()) {
            transferProgress(progressCount++, months);
             value = ALICE::instance().getUsed(Tier::kT0,   type, date) +
                     ALICE::instance().getUsed(Tier::kT1,   type, date) +
                     ALICE::instance().getUsed(Tier::kT2,   type, date);
            if(value < 0) {
                setProgressBar(false);
                QMessageBox message;
                message.setText(QString("No report found for %1 %2").arg(swhat).arg(date.toString("MM.yyyy")));
                message.exec();
                break;
            }

            switch (type) {
            case Resources::kCPU:
            {
                used += value;
                weight++;
                break;
            }
            case Resources::kDISK:
            {
                if (used < value)
                    used = value;
                break;
            }
            case Resources::kTAPE:
            {
                if (used < value)
                    used = value;
                break;
            }
            default:
                break;
            }

            date = date.addMonths(1);
        }
        if (used != -1) {
            dataVec->replace(colUsed - 2, used/weight);
            model->addData(QString::number(year), dataVec);
        }
    }

    setProgressBar(false);

    if (model->isEmpty()) // no data found
        return;

    setProgressBar(false);

    QTableView *tableView = new QTableView;
    tableView->setModel(model);
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableView->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableView->setAlternatingRowColors(true);
    tableView->hideColumn(0);

    QChart *chart = new QChart;
    chart->setTheme(QChart::ChartThemeHighContrast);

    QString title;
    switch (type) {
    case Resources::kCPU:
        title = QString("ALICE Pledged/Required/Used %1 (kHEPSpec06)").arg(swhat);
        break;
    case Resources::kDISK:
        title = QString("ALICE Pledged/Required/Used %1 (PB)").arg(swhat);
        break;
    case Resources::kTAPE:
        title = QString("ALICE Pledged/Required/Used %1 (PB)").arg(swhat);
        break;
     default:
        break;
    }

    chart->setTitle(title);

    QBarSeries *series = new QBarSeries;
    int first = 0;
    QVBarModelMapper *mapper = new QVBarModelMapper(mMdiArea);
    mapper->setModel(model);
    mapper->setFirstBarSetColumn(2);
    mapper->setLastBarSetColumn(4);
    mapper->setFirstRow(first);
    mapper->setRowCount(rows);
    mapper->setSeries(series);
    chart->addSeries(series);

     QBarCategoryAxis *axis = new QBarCategoryAxis();
     axis->append(categories);
     chart->createDefaultAxes();
     chart->setAxisX(axis, series);

     QChartView *chartView = new QChartView(chart);
     chartView->setRenderHint(QPainter::Antialiasing);
     chartView->setMinimumSize(640, 480);
     chartView->setChart(chart);
     tableView->setMaximumHeight(180);
     tableView->resizeColumnsToContents();

     QSplitter *splitter = new QSplitter(Qt::Vertical);
     splitter->addWidget(tableView);
     splitter->addWidget(chartView);

     QVBoxLayout *allLayout = new QVBoxLayout();
     allLayout->addWidget(splitter);


     for (QMdiSubWindow *sw : mMdiArea->subWindowList())
         if(sw->windowTitle() == title) {
             mMdiArea->removeSubWindow(sw);
             sw->close();
         }

     QWidget *allTogether = new QWidget();
     allTogether->setAttribute(Qt::WA_DeleteOnClose);
     allTogether->setWindowTitle(title);
     allTogether->setLayout(allLayout);

     mMdiArea->addSubWindow(allTogether)->show();
     allTogether->setStyleSheet("background-color:white");
}


//===========================================================================
void MainWindow::plot(qint32 opt)
{
    // do plots according to the directive of what

    switch (opt) {
    case kMandOProfile:
        selectDates(kMandOProfile);
        break;
    case kRequirementsProfile:
        selectDates(kRequirementsProfile, QDate(2025, 1, 1));
        break;
    case kPledgesProfile:
        selectDates(kPledgesProfile);
        break;
    case kRegisteredDataProfile:
        selectDates(kRegisteredDataProfile);
        break;
    case kUsageProfile:
        selectDates(kUsageProfile);
        break;
    case kUsage_PledgesProfile:
        selectDates(kUsage_PledgesProfile);
        break;
    case kUsage_RequiredProfile:
        selectDates(kUsage_RequiredProfile);
        break;
    case kTierEfficiencyProfile:
        selectDates(kTierEfficiencyProfile);
        break;
    case kUserEfficiencyProfile:
        selectDates(kUserEfficiencyProfile);
        break;
    case kEventSizeProfile:
        getDataFromFile(kEventSizeProfile);
        plProfileEventSize();
        break;
    case kPledgedRequiredUsed:
        selectDates(kPledgedRequiredUsed, QDate(2025, 1, 1));
    default:
        break;
   }
}

//===========================================================================
void MainWindow::plRegisteredData(PlotOptions opt)
{
    // plot data (mPlData) as Area plot
    // data is a list of QVector rows,
    //      column 0 is a date (imn ms since...) for x axis and
    //      colums 1,.... are the y values

    // the data are in mPLData


    QChart *chart = new QChart();
    chart->setTheme(QChart::ChartThemeBrownSand);
    QMetaEnum me = QMetaEnum::fromType<PlotOptions>();
    QString sopt = me.key(opt);
    sopt.remove("k");
    sopt.remove("Profile");
    chart->setTitle(QString("ALICE %1").arg(sopt));

    int columns = mPlData.at(0)->size();
    int rows   = mPlData.size();

    QDateTimeAxis *axisX = new QDateTimeAxis;
    axisX->setTickCount(12);
    axisX->setFormat("MM-yyyy");
    axisX->setTitleText("Date");
    chart->addAxis(axisX, Qt::AlignBottom);

    QValueAxis *axisY = new QValueAxis;
    axisY->setTickCount(9);
    axisY->setLabelFormat("%i");
    axisY->setTitleText("Registered Data (PB)");
    chart->addAxis(axisY, Qt::AlignLeft);

    QLineSeries *lowerSeries = Q_NULLPTR;
    double ymax = 0.0;

    QDateTime today(QDate::currentDate());
    double xmin = today.toMSecsSinceEpoch();
    for (qint32 col = 1; col < columns; col++) {
        QLineSeries *upperSeries = new QLineSeries();
        for (qint32 irow = 0; irow < rows; irow++) {
            QVector<double> *row = mPlData.at(irow);
            double x = row->at(0);
            if (x < xmin)
                xmin = x;
            double y;
            if (lowerSeries) {
                const QVector<QPointF>& points = lowerSeries->pointsVector();
                y = points[irow].y() + row->at(col) / 1E9; //change to PB
            } else
                y = row->at(col) / 1E9; //change to PB
            if (y > ymax)
                ymax = y;
            upperSeries->append(x, y);
        }

        QAreaSeries *area = new QAreaSeries(upperSeries, lowerSeries);
        chart->addSeries(area);
        area->attachAxis(axisY);
        area->attachAxis(axisX);
        area->setName(mPlDataName.at(col));

        today.setMSecsSinceEpoch(xmin);
        axisX->setMin(today);

        int exp = qFloor(qLn(ymax) / qLn(10));
        if (ymax  < qPow(10., exp))
            ymax = qPow(10., exp);
        else {
            ymax = qCeil(ymax / qPow(10, exp)) * qPow(10, exp);
        }
        lowerSeries = upperSeries;
    }
    axisY->setMax(ymax);
    axisY->setMin(0.0);

    QString title("Registered Data Profile");
    for (QMdiSubWindow *sw : mMdiArea->subWindowList())
        if(sw->windowTitle() == title) {
            mMdiArea->removeSubWindow(sw);
            sw->close();
        }
    QChartView *chartView = new QChartView();
    chartView->setAttribute(Qt::WA_DeleteOnClose);
    chartView->setWindowTitle(title);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumSize(640, 480);
    chartView->setChart(chart);

    mMdiArea->addSubWindow(chartView)->show();
}

//===========================================================================
void MainWindow::plTierEfficiency(PlotOptions opt)
{
    // plot the efficiency per Tier (Tier0, Tiers1, Tiers2)
    // data is a list of QVector rows,
    //      column 0 is a date (imn ms since...) for x axis and
    //      colums 1,.... are the y values

    // the data are in mPLData
    // the name of colums are in mPLDataName


    QChart *chart = new QChart();
    chart->setTheme(QChart::ChartThemeBrownSand);
    QMetaEnum me = QMetaEnum::fromType<PlotOptions>();
    QString sopt = me.key(opt);
    sopt.remove("k");
    sopt.remove("Profile");
    chart->setTitle(QString("ALICE %1").arg(sopt));

    QDateTimeAxis *axisX = new QDateTimeAxis;
    axisX->setTickCount(12);
    axisX->setFormat("MM-yyyy");
    axisX->setTitleText("Date");
    chart->addAxis(axisX, Qt::AlignBottom);

    QValueAxis *axisY = new QValueAxis;
    axisY->setTickCount(9);
    axisY->setLabelFormat("%i");
    axisY->setTitleText("Efficiency (%)");
    axisY->setMax(100);
    chart->addAxis(axisY, Qt::AlignLeft);


    QVector<Tier::TierCat> tcat(mPlDataName.size() - 1); // omit the time column
    qint32 colIndex = 0;
    for (QString ce : mPlDataName) {
        Tier *t = ALICE::instance().search(ce);
        if (!t) {
            tcat.insert(colIndex, Tier::kT2);
        } else {
            Tier::TierCat cat = t->category();
            tcat.insert(colIndex, cat);
        }
        colIndex++;
    }

    QLineSeries *seriesT0 = new QLineSeries;
    seriesT0->setName("T0");

    QLineSeries *seriesT1 = new QLineSeries;
    seriesT1->setName("T1");

    QLineSeries *seriesT2 = new QLineSeries;
    seriesT2->setName("T2");

    QVector<double> *avEffTiers = new QVector<double>(3);
    QVector<qint32> avNormTiers(3);

    avEffTiers->replace(Tier::kT0, 0.0);
    avEffTiers->replace(Tier::kT1, 0.0);
    avEffTiers->replace(Tier::kT2, 0.0);
    avNormTiers.replace(Tier::kT0, 0);
    avNormTiers.replace(Tier::kT1, 0);
    avNormTiers.replace(Tier::kT2, 0);

    QDateTime today(QDate::currentDate());
    double xmin = today.toMSecsSinceEpoch();

    QVector<double> effTiers(3);
    QVector<qint32> normTiers(3);


    for (QVector<double> *valVec : mPlData) {
        effTiers.replace(Tier::kT0, 0.0);
        effTiers.replace(Tier::kT1, 0.0);
        effTiers.replace(Tier::kT2, 0.0);
        normTiers.replace(Tier::kT0, 0);
        normTiers.replace(Tier::kT1, 0);
        normTiers.replace(Tier::kT2, 0);
        double x = valVec->at(0);
        if (x < xmin)
            xmin = x;
        for (qint32 colIndex = 1; colIndex < valVec->size(); colIndex++) {
            Tier::TierCat cat = tcat.at(colIndex);
            double val = valVec->at(colIndex);
            if ( val > 0.0) {
                effTiers.replace(cat, effTiers.at(cat) + val);
                normTiers.replace(cat, normTiers.at(cat) + 1);
                avEffTiers->replace(cat, avEffTiers->at(cat) + val);
                avNormTiers.replace(cat, avNormTiers.at(cat) + 1);
            }
        }
        if (normTiers.at(Tier::kT0) > 0) {
            effTiers.replace(Tier::kT0, effTiers.at(Tier::kT0) / normTiers.at(Tier::kT0));
            seriesT0->append(x, effTiers.at(Tier::kT0));
        }
        if (normTiers.at(Tier::kT1) > 0) {
            effTiers.replace(Tier::kT1, effTiers.at(Tier::kT1) / normTiers.at(Tier::kT1));
            seriesT1->append(x, effTiers.at(Tier::kT1));
        }
        if (normTiers.at(Tier::kT1) > 1) {
            effTiers.replace(Tier::kT2, effTiers.at(Tier::kT2) / normTiers.at(Tier::kT2));
            seriesT2->append((qint64)x, effTiers.at(Tier::kT2));
        }
    }

    avEffTiers->replace(Tier::kT0, avEffTiers->at(Tier::kT0) / avNormTiers.at(Tier::kT0));
    avEffTiers->replace(Tier::kT1, avEffTiers->at(Tier::kT1) / avNormTiers.at(Tier::kT1));
    avEffTiers->replace(Tier::kT2, avEffTiers->at(Tier::kT2) / avNormTiers.at(Tier::kT2));

    PlTableModel *model = new PlTableModel();
    model->setColRow(5, 1);
    QVector<QString> headers(model->columnCount());
    headers.replace(Tier::kT0 + 2, "Average efficiency at T0 (%)");
    headers.replace(Tier::kT1 + 2, "Average efficiency at T1s (%)");
    headers.replace(Tier::kT2 + 2, "Average efficiency at T2s (%)");
    model->setHeader(headers);
    QString dummy("1970");
    model->addData(dummy, avEffTiers);

    chart->addSeries(seriesT0);
    seriesT0->attachAxis(axisY);
    seriesT0->attachAxis(axisX);

    chart->addSeries(seriesT1);
    seriesT1->attachAxis(axisY);
    seriesT1->attachAxis(axisX);

    chart->addSeries(seriesT2);
    seriesT2->attachAxis(axisY);
    seriesT2->attachAxis(axisX);

    today.setMSecsSinceEpoch(xmin);
    axisX->setMin(today);

    QString title("Tier Efficiency Profile");

    QChartView *chartView = new QChartView();
    chartView->setAttribute(Qt::WA_DeleteOnClose);
    chartView->setWindowTitle(title);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumSize(640, 480);
    chartView->setChart(chart);

    QTableView *tableView = new QTableView;
    tableView->setModel(model);
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableView->resizeColumnsToContents();
    tableView->setMaximumHeight(80);
    tableView->hideColumn(0);
    tableView->hideColumn(1);

    QSplitter *splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(tableView);
    splitter->addWidget(chartView);

    QVBoxLayout *allLayout = new QVBoxLayout();
    allLayout->addWidget(splitter);

    for (QMdiSubWindow *sw : mMdiArea->subWindowList())
        if(sw->windowTitle() == title) {
            mMdiArea->removeSubWindow(sw);
            sw->close();
        }

    QWidget *allTogether = new QWidget();
    allTogether->setWindowTitle(title);
    allTogether->setLayout(allLayout);

    mMdiArea->addSubWindow(allTogether)->show();
    allTogether->setStyleSheet("background-color:white");
}

//===========================================================================
void MainWindow::plUserEfficiency(PlotOptions opt)
{
    // plot the efficiency per user
    // data is a list of QVector rows,
    //      column 0 is a date (imn ms since...) for x axis and
    //      colums 1,.... are the y values

    // the data are in mPLData

    QChart *chart = new QChart();
    chart->setTheme(QChart::ChartThemeBrownSand);
    QMetaEnum me = QMetaEnum::fromType<PlotOptions>();
    QString sopt = me.key(opt);
    sopt.remove("k");
    sopt.remove("Profile");
    chart->setTitle(QString("ALICE %1").arg(sopt));

    QDateTimeAxis *axisX = new QDateTimeAxis;
    axisX->setTickCount(12);
    axisX->setFormat("MM-yyyy");
    axisX->setTitleText("Date");
    chart->addAxis(axisX, Qt::AlignBottom);

    QValueAxis *axisY = new QValueAxis;
    axisY->setTickCount(9);
    axisY->setLabelFormat("%i");
    axisY->setTitleText("Efficiency (%)");
    axisY->setMax(100);
    chart->addAxis(axisY, Qt::AlignLeft);


    QVector<ALICE::UserCat> ucat(mPlDataName.size() - 1); // omit the time column
    qint32 colIndex = 0;
    for (QString ce : mPlDataName) {
        if (ce == "alidaq")
            ucat.insert(colIndex, ALICE::kAliDaq);
        else if (ce == "aliprod")
            ucat.insert(colIndex, ALICE::kAliProd);
        else if (ce == "alitrain")
            ucat.insert(colIndex, ALICE::kAliTrain);
        else
            ucat.insert(colIndex, ALICE::kAliUsers);
        colIndex++;
    }

    QLineSeries *seriesDaq = new QLineSeries;
    seriesDaq->setName("Raw Data Processing");

    QLineSeries *seriesProd = new QLineSeries;
    seriesProd->setName("Monte-Carlo Production");

    QLineSeries *seriesTrain = new QLineSeries;
    seriesTrain->setName("Train Analysis");

    QLineSeries *seriesUsers = new QLineSeries;
    seriesUsers->setName("Users Analysis");

    QVector<double> *avEffUsers = new QVector<double>(ALICE::kAliUsers + 1);
    QVector<qint32> avNormUsers(ALICE::kAliUsers + 1);

    avEffUsers->replace(ALICE::kAliDaq,   0.0);
    avEffUsers->replace(ALICE::kAliProd,  0.0);
    avEffUsers->replace(ALICE::kAliTrain, 0.0);
    avEffUsers->replace(ALICE::kAliUsers, 0.0);
    avNormUsers.replace(ALICE::kAliDaq,   0);
    avNormUsers.replace(ALICE::kAliProd,  0);
    avNormUsers.replace(ALICE::kAliTrain, 0);
    avNormUsers.replace(ALICE::kAliUsers, 0);

    QDateTime today(QDate::currentDate());
    double xmin = today.toMSecsSinceEpoch();

    QVector<qint32> normUsers(ALICE::kAliUsers + 1);
    QVector<double> effUsers(ALICE::kAliUsers  + 1);

    for (QVector<double> *valVec : mPlData) {
        effUsers.replace(ALICE::kAliDaq,   0.0);
        effUsers.replace(ALICE::kAliProd,  0.0);
        effUsers.replace(ALICE::kAliTrain, 0.0);
        effUsers.replace(ALICE::kAliUsers, 0.0);
        normUsers.replace(ALICE::kAliDaq,   0);
        normUsers.replace(ALICE::kAliProd,  0);
        normUsers.replace(ALICE::kAliTrain, 0);
        normUsers.replace(ALICE::kAliUsers, 0);
        double x = valVec->at(0);
        if (x < xmin)
            xmin = x;
        for (qint32 colIndex = 1; colIndex < valVec->size(); colIndex++) {
            ALICE::UserCat user = ucat.at(colIndex);
            double val = valVec->at(colIndex);
            if ( val > 0.0) {
                effUsers.replace(user, effUsers.at(user) + val);
                normUsers.replace(user, normUsers.at(user) + 1);
                avEffUsers->replace(user, avEffUsers->at(user) + val);
                avNormUsers.replace(user, avNormUsers.at(user) + 1);
            }
        }
        if (normUsers.at(ALICE::kAliDaq) > 0) {
            effUsers.replace(ALICE::kAliDaq, effUsers.at(ALICE::kAliDaq) / normUsers.at(ALICE::kAliDaq));
            seriesDaq->append(x, effUsers.at(ALICE::kAliDaq));
        }
        if (normUsers.at(ALICE::kAliProd) > 0) {
            effUsers.replace(ALICE::kAliProd, effUsers.at(ALICE::kAliProd) / normUsers.at(ALICE::kAliProd));
            seriesProd->append(x, effUsers.at(ALICE::kAliProd));
        }
        if (normUsers.at(ALICE::kAliTrain) > 0) {
            effUsers.replace(ALICE::kAliTrain, effUsers.at(ALICE::kAliTrain) / normUsers.at(ALICE::kAliTrain));
            seriesTrain->append((qint64)x, effUsers.at(ALICE::kAliTrain));
        }
        if (normUsers.at(ALICE::kAliUsers) > 0) {
            effUsers.replace(ALICE::kAliUsers, effUsers.at(ALICE::kAliUsers) / normUsers.at(ALICE::kAliUsers));
            seriesUsers->append((qint64)x, effUsers.at(ALICE::kAliUsers));
        }
    }

    avEffUsers->replace(ALICE::kAliDaq, avEffUsers->at(ALICE::kAliDaq) / avNormUsers.at(ALICE::kAliDaq));
    avEffUsers->replace(ALICE::kAliProd, avEffUsers->at(ALICE::kAliProd) / avNormUsers.at(ALICE::kAliProd));
    avEffUsers->replace(ALICE::kAliTrain, avEffUsers->at(ALICE::kAliTrain) / avNormUsers.at(ALICE::kAliTrain));
    avEffUsers->replace(ALICE::kAliUsers, avEffUsers->at(ALICE::kAliUsers) / avNormUsers.at(ALICE::kAliUsers));

    PlTableModel *model = new PlTableModel();
    model->setColRow(ALICE::kAliUsers + 3, 1);
    QVector<QString> headers(model->columnCount());
    headers.replace(ALICE::kAliDaq   + 2, "Average efficiency for Raw Processing (%)");
    headers.replace(ALICE::kAliProd  + 2, "Average efficiency for MC Production (%)");
    headers.replace(ALICE::kAliTrain + 2, "Average efficiency for Train Analysis (%)");
    headers.replace(ALICE::kAliUsers + 2, "Average efficiency for Users Analysis (%)");
    model->setHeader(headers);
    QString dummy("1970");
    model->addData(dummy, avEffUsers);

    chart->addSeries(seriesDaq);
    seriesDaq->attachAxis(axisY);
    seriesDaq->attachAxis(axisX);

    chart->addSeries(seriesProd);
    seriesProd->attachAxis(axisY);
    seriesProd->attachAxis(axisX);

    chart->addSeries(seriesTrain);
    seriesTrain->attachAxis(axisY);
    seriesTrain->attachAxis(axisX);

    chart->addSeries(seriesUsers);
    seriesUsers->attachAxis(axisY);
    seriesUsers->attachAxis(axisX);

    today.setMSecsSinceEpoch(xmin);
    axisX->setMin(today);

    QString title("Users Efficiency Profile");

    QChartView *chartView = new QChartView();
    chartView->setAttribute(Qt::WA_DeleteOnClose);
    chartView->setWindowTitle(title);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumSize(640, 480);
    chartView->setChart(chart);

    QTableView *tableView = new QTableView;
    tableView->setModel(model);
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableView->resizeColumnsToContents();
    tableView->setMaximumHeight(80);
    tableView->hideColumn(0);
    tableView->hideColumn(1);

    QSplitter *splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(tableView);
    splitter->addWidget(chartView);

    QVBoxLayout *allLayout = new QVBoxLayout();
    allLayout->addWidget(splitter);

    for (QMdiSubWindow *sw : mMdiArea->subWindowList())
        if(sw->windowTitle() == title) {
            mMdiArea->removeSubWindow(sw);
            sw->close();
        }

    QWidget *allTogether = new QWidget();
    allTogether->setWindowTitle(title);
    allTogether->setLayout(allLayout);

    mMdiArea->addSubWindow(allTogether)->show();
//    allTogether->setStyleSheet("background-color:white");
}

//===========================================================================
void MainWindow:: getDataFromWeb(PlotOptions opt)
{
    // plot the registered data profile

    setProgressBar(true);

    QNetworkRequest request;
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    request.setUrl(mURL);

    if (!mNetworkManager)
        mNetworkManager = new QNetworkAccessManager(this);
    QNetworkReply *reply = mNetworkManager->get(request);


    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(transferProgress(qint64,qint64)));    
    connect(reply, &QNetworkReply::finished, this, [opt, this]{ parsePlotUrlFile(opt); });
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(showNetworkError(QNetworkReply::NetworkError)));
}

//===========================================================================
void MainWindow::getDataFromWeb(const QDate &date, Tier::TierCat cat)
{
    // connect to the mURL and continue with saveURLFile when connection established

    setProgressBar(true);

    QNetworkRequest request;
    QSslConfiguration conf = request.sslConfiguration();
    conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    request.setSslConfiguration(conf);
    request.setUrl(mURL);

    if (!mNetworkManager)
        mNetworkManager = new QNetworkAccessManager(this);
    QNetworkReply *reply = mNetworkManager->get(request);


    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(transferProgress(qint64,qint64)));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(showNetworkError(QNetworkReply::NetworkError)));
    connect(reply, &QNetworkReply::finished, this, [date, cat, this]{ saveUrlFile(date, cat); });
}

//===========================================================================
void MainWindow::getDataFromWeb(const QDate &date, LoadOptions opt)
{
    // connect to the mURL and continue with saveURLFile when connection established

    setProgressBar(true);

    QNetworkRequest request;

    QSslConfiguration conf;

    if (opt == kMLRAWProd) {
        QNetworkProxyFactory::setUseSystemConfiguration(true);

           QFile privateKeyFile("/Users/schutz/cert/userkey.pem");
           privateKeyFile.open(QIODevice::ReadOnly);

           QFile certificateFile("/Users/schutz/cert/cern.crt");
           certificateFile.open(QIODevice::ReadOnly);

           QSslKey privateKey(&privateKeyFile, QSsl::Opaque);
           QSslCertificate certificate(&certificateFile);

           qWarning() << QSslSocket::supportsSsl();
           qWarning() << certificate.serialNumber();
           qWarning() << certificate.subjectInfo(QSslCertificate::CommonName);
           qWarning() << certificate.expiryDate();

           conf.setPrivateKey(privateKey);
           conf.setLocalCertificate(certificate);
    } else {
        conf = request.sslConfiguration();
        conf.setPeerVerifyMode(QSslSocket::VerifyNone);
    }

    request.setSslConfiguration(conf);
    request.setUrl(mURL);

    if (!mNetworkManager)
        mNetworkManager = new QNetworkAccessManager(this);
    QNetworkReply *reply = mNetworkManager->get(request);


    connect(reply, SIGNAL(downloadProgress(qint64,qint64)), this, SLOT(transferProgress(qint64,qint64)));
    connect(reply, SIGNAL(error(QNetworkReply::NetworkError)), this, SLOT(showNetworkError(QNetworkReply::NetworkError)));
    connect(reply, &QNetworkReply::finished, this, [date, opt, this]{ saveUrlFile(date, opt); });

}

//===========================================================================
void MainWindow::getDataFromFile(PlotOptions opt)
{
    // reads data from a local file into mPLData and mPLDataName

    switch (opt) {
    case kEventSizeProfile:
    {
        mPlDataName.clear();
        qDeleteAll(mPlData.begin(), mPlData.end());
        mPlData.clear();
        QString fileName("/data/EventSize.csv");
        QByteArray dataA = ALICE::instance().getReportFromWeb(fileName);
        if (dataA.isEmpty())
            return;
        QTextStream csvFile(&dataA);

        // read headers in second line
        QString line = csvFile.readLine();
        line = csvFile.readLine();
        QStringList strList = line.split(";");
        // find the event size column
        qint32 esColumn = -1;
        for (qint32 index = 0; index < strList.size(); index++) {
            QString str = strList.at(index);
            if (str.contains("Event Size")) {
                esColumn = index;
                break;
            }
        }
        while(!csvFile.atEnd()) {
            line = csvFile.readLine();
            strList = line.split(";");
            QString period(strList.at(0));
            if (period.contains("LHC")) {
                QVector<double> *dataVec = new QVector<double>(1);
                mPlDataName.append(strList.at(0));
                QString data = strList.at(esColumn);
                data.replace(" ", "");
                dataVec->replace(0, data.toDouble());
                mPlData.append(dataVec);
            }
        }
        break;
    }
    default:
        break;
    }
}

//===========================================================================
void MainWindow::   plProfile(PlotOptions opt, Resources::Resources_type type)
{
    // plot requirements CPU, Disk and tape as a function of year

    QMetaEnum me = QMetaEnum::fromType<Resources::Resources_type>();
    QString swhat = me.key(type);
    swhat.remove(0, 1); // removes the "k"

    PlTableModel *model = new PlTableModel();

    // get the data
    qint32 columns = 6; // year in ms; year; T0; T1; T2; Total smoothed
    qint32 rows    = 5;
    model->setColRow(columns, rows);
    qint32 colYears = 0;
    qint32 colYear  = 1;
    qint32 colT0    = 2;
    qint32 colT1    = 3;
    qint32 colT2    = 4;
    qint32 colTo    = 5;
    QVector<QString> headers(columns);
    headers.replace(colYears, "");
    headers.replace(colYear, "Year");
    headers.replace(colT0, "T0");
    headers.replace(colT1, "T1");
    headers.replace(colT2, "T2");
    headers.replace(colTo, "Total");
    model->setHeader(headers);

    setProgressBar();
    int count = 0;

    QVector<double> *dataVec = new QVector<double>(columns - 2);
    QString xAxisFormat;
    QString xAxisTitle;
    if (opt == kRequirementsProfile || opt == kPledgesProfile) {
        xAxisFormat = "yyyy";
        xAxisTitle = "Year";
        int years = mDEStart->date().daysTo(mDEEnd->date()) / 365;
        for (int year = mDEStart->date().year(); year <= mDEEnd->date().year(); year++) {
            transferProgress(count++, years);
            if (opt == kRequirementsProfile) {
                dataVec->replace(colT0 - 2, ALICE::instance().getRequired(Tier::kT0,   type, QString::number(year)));
                dataVec->replace(colT1 - 2, ALICE::instance().getRequired(Tier::kT1,   type, QString::number(year)));
                dataVec->replace(colT2 - 2, ALICE::instance().getRequired(Tier::kT2,   type, QString::number(year)));
                dataVec->replace(colTo - 2, ALICE::instance().getRequired(Tier::kTOTS, type, QString::number(year)));
                model->addData(QString::number(year), dataVec);
            } else if (opt == kPledgesProfile) {
                dataVec->replace(colT0 - 2, ALICE::instance().getPledged(Tier::kT0,   type, QString::number(year)));
                dataVec->replace(colT1 - 2, ALICE::instance().getPledged(Tier::kT1,   type, QString::number(year)));
                dataVec->replace(colT2 - 2, ALICE::instance().getPledged(Tier::kT2,   type, QString::number(year)));
                dataVec->replace(colTo - 2, ALICE::instance().getPledged(Tier::kTOTS, type, QString::number(year)));
                model->addData(QString::number(year), dataVec);
            }
        }
    } else if (opt == kUsageProfile || opt == kUsage_PledgesProfile || opt == kUsage_RequiredProfile) {
        xAxisFormat = "MM-yyyy";
        xAxisTitle = "Date";
        ALICE::instance().setDrawTable(false);
        QDate date = mDEStart->date();
        int months = date.daysTo(mDEEnd->date()) / 30;
        while (date <= mDEEnd->date()) {
            transferProgress(count++, months);
            double value0 = 0.0;
            double value1 = 0.0;
            double value2 = 0.0;
            double valueo = 0.0;
            if (opt == kUsageProfile) {
                value0 = ALICE::instance().getUsed(Tier::kT0,   type, date);
                value1 = ALICE::instance().getUsed(Tier::kT1,   type, date);
                value2 = ALICE::instance().getUsed(Tier::kT2,   type, date);
                valueo = ALICE::instance().getUsed(Tier::kTOTS, type, date);
            } else if (opt == kUsage_PledgesProfile){
                value0 = 100 * ALICE::instance().getUsed(Tier::kT0,   type, date) / ALICE::instance().getPledged(Tier::kT0,   type, QString::number(date.year()));
                value1 = 100 * ALICE::instance().getUsed(Tier::kT1,   type, date) / ALICE::instance().getPledged(Tier::kT1,   type, QString::number(date.year()));
                value2 = 100 * ALICE::instance().getUsed(Tier::kT2,   type, date) / ALICE::instance().getPledged(Tier::kT2,   type, QString::number(date.year()));
                valueo = 100 * ALICE::instance().getUsed(Tier::kTOTS, type, date) / ALICE::instance().getPledged(Tier::kTOTS, type, QString::number(date.year()));
            } else {
                value0 = 100 * ALICE::instance().getUsed(Tier::kT0,   type, date) / ALICE::instance().getRequired(Tier::kT0,   type, QString::number(date.year()));
                value1 = 100 * ALICE::instance().getUsed(Tier::kT1,   type, date) / ALICE::instance().getRequired(Tier::kT1,   type, QString::number(date.year()));
                value2 = 100 * ALICE::instance().getUsed(Tier::kT2,   type, date) / ALICE::instance().getRequired(Tier::kT2,   type, QString::number(date.year()));
                valueo = 100 * ALICE::instance().getUsed(Tier::kTOTS, type, date) / ALICE::instance().getRequired(Tier::kTOTS, type, QString::number(date.year()));
            }
            if(value0 < 0 || value1 < 0 || value2 < 0 || valueo < 0) {
                setProgressBar(false);
                QMessageBox message;
                message.setText(QString("No report found for %1 %2").arg(swhat).arg(date.toString("MM.yyyy")));
                message.exec();
                break;
            }

            dataVec->replace(colT0 - 2, value0);
            dataVec->replace(colT1 - 2, value1);
            dataVec->replace(colT2 - 2, value2);
            dataVec->replace(colTo - 2, valueo);
            model->addData(QDateTime(date), dataVec);
            date = date.addMonths(1);
        }
        ALICE::instance().setDrawTable(true);
    }
    setProgressBar(false);

    double ymax = model->findMax();
    int exp = qFloor(qLn(ymax) / qLn(10));
    if (ymax  < qPow(10., exp))
        ymax = qPow(10., exp);
    else {
        ymax = qCeil(ymax / qPow(10, exp)) * qPow(10, exp);
    }

    QTableView *tableView = new QTableView;
    tableView->setModel(model);
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableView->setAlternatingRowColors(true);
    tableView->hideColumn(0);

    QChart *chart = new QChart;
    //   chart->setAnimationOptions(QChart::AllAnimations); // if set will not print the chart
    chart->setTheme(QChart::ChartThemeHighContrast);
    me = QMetaEnum::fromType<PlotOptions>();
    QString sopt = me.key(opt);
    sopt.remove("k");
    sopt.remove("Profile");
    chart->setTitle(QString("ALICE %1 %2 for RUN2").arg(swhat).arg(sopt));

    QDateTimeAxis *axisX = new QDateTimeAxis;
    axisX->setTickCount(rows);
    axisX->setFormat(xAxisFormat);
    axisX->setTitleText(xAxisTitle);
    chart->addAxis(axisX, Qt::AlignBottom);

    QValueAxis *axisY = new QValueAxis;
    axisY->setLabelFormat("%.1f");
    QString title;
    if (opt == kUsage_PledgesProfile || opt == kUsage_RequiredProfile)
        title = "\%";
    else {
        switch (type) {
        case Resources::kCPU:
            title = "CPU (kHEPSpec06)";
            break;
        case Resources::kDISK:
            title = "DISK (PB)";
            break;
        case Resources::kTAPE:
            title = "TAPE (PB)";
            break;
        default:
            break;
        }
    }
    axisY->setTitleText(title);
    axisY->setMax(ymax);
    axisY->setMin(0.0);
    chart->addAxis(axisY, Qt::AlignLeft);

    // Tot
    QLineSeries *series = new QLineSeries;
    series->setName("Total");
    QVXYModelMapper *mapper = new QVXYModelMapper(mMdiArea);
    mapper->setModel(model);
    mapper->setXColumn(colYears);
    mapper->setYColumn(colTo);
    mapper->setSeries(series);
    chart->addSeries(series);
    series->attachAxis(axisY);
    series->attachAxis(axisX);

    QScatterSeries *sseries = new QScatterSeries(mMdiArea);
    mapper = new QVXYModelMapper;
    mapper->setModel(model);
    mapper->setXColumn(colYears);
    mapper->setYColumn(colTo);
    mapper->setSeries(sseries);
    chart->addSeries(sseries);
    sseries->attachAxis(axisY);
    sseries->attachAxis(axisX);
    sseries->setColor(series->color());

    QList<QLegendMarker*> markers = chart->legend()->markers(sseries);
    QLegendMarker *marker = markers.at(0);
    marker->setVisible(false);

    // T0
    series = new QLineSeries;
    series->setName("T0");
    mapper = new QVXYModelMapper(mMdiArea);
    mapper->setModel(model);
    mapper->setXColumn(colYears);
    mapper->setYColumn(colT0);
    mapper->setSeries(series);
    chart->addSeries(series);
    series->attachAxis(axisY);
    series->attachAxis(axisX);

    sseries = new QScatterSeries;
    mapper = new QVXYModelMapper(mMdiArea);
    mapper->setModel(model);
    mapper->setXColumn(colYears);
    mapper->setYColumn(colT0);
    mapper->setSeries(sseries);
    chart->addSeries(sseries);
    sseries->attachAxis(axisY);
    sseries->attachAxis(axisX);
    sseries->setColor(series->color());

    markers = chart->legend()->markers(sseries);
    marker = markers.at(0);
    marker->setVisible(false);

    // T1
    series = new QLineSeries;
    series->setName("T1");
    mapper = new QVXYModelMapper(mMdiArea);
    mapper->setModel(model);
    mapper->setXColumn(colYears);
    mapper->setYColumn(colT1);
    mapper->setSeries(series);
    chart->addSeries(series);
    series->attachAxis(axisY);
    series->attachAxis(axisX);

    sseries = new QScatterSeries;
    mapper = new QVXYModelMapper(mMdiArea);
    mapper->setModel(model);
    mapper->setXColumn(colYears);
    mapper->setYColumn(colT1);
    mapper->setSeries(sseries);
    chart->addSeries(sseries);
    sseries->attachAxis(axisY);
    sseries->attachAxis(axisX);
    sseries->setColor(series->color());

    markers = chart->legend()->markers(sseries);
    marker = markers.at(0);
    marker->setVisible(false);

    // T2
    if (type != Resources::kTAPE) {
        series = new QLineSeries;
        series->setName("T2");
        mapper = new QVXYModelMapper(mMdiArea);
        mapper->setModel(model);
        mapper->setXColumn(colYears);
        mapper->setYColumn(colT2);
        mapper->setSeries(series);
        chart->addSeries(series);
        series->attachAxis(axisY);
        series->attachAxis(axisX);

        sseries = new QScatterSeries;
        mapper = new QVXYModelMapper(mMdiArea);
        mapper->setModel(model);
        mapper->setXColumn(colYears);
        mapper->setYColumn(colT2);
        mapper->setSeries(sseries);
        chart->addSeries(sseries);
        sseries->attachAxis(axisY);
        sseries->attachAxis(axisX);
        sseries->setColor(series->color());

        markers = chart->legend()->markers(sseries);
        marker = markers.at(0);
        marker->setVisible(false);
    }

    QChartView *chartView = new QChartView();
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumSize(640, 480);
    chartView->setChart(chart);
    tableView->setMaximumHeight(180);
    tableView->resizeColumnsToContents();

    QSplitter *splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(tableView);
    splitter->addWidget(chartView);

    QVBoxLayout *allLayout = new QVBoxLayout();
    allLayout->addWidget(splitter);

    switch (type) {
    case Resources::kCPU:
        title = QString("%1 CPU profile during RUN2 (unit = kHEPSpec06)").arg(sopt);
        break;
    case Resources::kDISK:
        title = QString("%1 Disk profile during RUN2 (unit = PB)").arg(sopt);
        break;
    case Resources::kTAPE:
        title = QString("%1 Tape profile during RUN2 (unit = PB)").arg(sopt);
        break;
    default:
        break;
    }

    for (QMdiSubWindow *sw : mMdiArea->subWindowList())
        if(sw->windowTitle() == title) {
            mMdiArea->removeSubWindow(sw);
            sw->close();
        }

    QWidget *allTogether = new QWidget();
    allTogether->setAttribute(Qt::WA_DeleteOnClose);
    allTogether->setWindowTitle(title);
    allTogether->setLayout(allLayout);

    mMdiArea->addSubWindow(allTogether)->show();
    allTogether->setStyleSheet("background-color:white");
}

//===========================================================================
void MainWindow::printCurrentWindow() const
{
    // print the current window
    QMdiSubWindow *sw = mMdiArea->currentSubWindow();
    //FIXME: directory name
    QString oFileName = QString("/Users/schutz/Desktop/%1.pdf").arg(sw->windowTitle());
    QPdfWriter writer(oFileName);
    writer.setPageSize(QPagedPaintDevice::A4);
    QPainter painter(&writer);

    QPixmap pix = sw->grab();
    qint32 horg = sw->height();
    qint32 worg = sw->width();
    double scale = 0.9;
    qint32 w    = painter.window().width()  * scale;
    qint32 h    = painter.window().width() * scale * horg / worg ;
    qint32 xoff = (painter.window().width() / 2) - (w / 2);
    qint32 yoff = (painter.window().height() / 2) - (h / 2);
    sw->resize(w, h);
    painter.drawPixmap(xoff, yoff, w, h, pix);
    painter.end();
    sw->resize(worg, horg);
    QMessageBox *mb = new QMessageBox;
    mb->setAttribute(Qt::WA_DeleteOnClose);
    mb->setText(QString("Window saved as %1").arg(oFileName));
    mb->show();
}

//===========================================================================
void MainWindow::saveUrlFile(const QDate &date, LoadOptions opt)
{
    Q_UNUSED(date);
    // save the url as a file
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    QByteArray data = reply->readAll();
    if (data.isEmpty()) {
        qCritical() << Q_FUNC_INFO << "no data found";
        return;
    }

    mDownLoadText->setText("DONE");

    QTextStream instream(&data);
    QString line;
    //FIXME: save file to the righ place
//    QString dir(QString("data/%1/%2/").arg(date.year()).arg(date.month()));
    QString dir("/Users/schutz/Desktop/");
    QString fileName;
    if (opt == kMLCPUReport)
        fileName = "CPU_Usage.csv";
    else if (opt == kMLStorageReport)
        fileName = "Disk_Tape_Usage.csv";
    QFile file( fileName.prepend(dir));
    if ( file.open(QIODevice::ReadWrite) )
    {
        QTextStream oustream( &file );
        while (instream.readLineInto(&line)) {
            oustream << line << endl;
        }
        file.close();
    }

    setProgressBar(false);
}

//===========================================================================
void MainWindow::saveUrlFile(const QDate &date, Tier::TierCat cat)
{
    Q_UNUSED(date);
    // save the url as a file
    QNetworkReply *reply = qobject_cast<QNetworkReply*>(sender());
    QByteArray data = reply->readAll();
    if (data.isEmpty()) {
        qCritical() << Q_FUNC_INFO << "no data found";
        return;
    }

    mDownLoadText->setText("DONE");

    QTextStream instream(&data);
    QString line;
    //FIXME: save file to the righ place
//    QString dir(QString("data/%1/%2/").arg(date.year()).arg(date.month()));
    QString dir("/Users/schutz/Desktop/");
    QString fileName;
    if (cat == Tier::kT0 || cat == Tier::kT1)
        fileName = "TIER1_TIER1_sum_normcpu_TIER1_VO.csv";
    else if (cat == Tier::kT2)
        fileName = "reptier2.csv";
    QFile file( fileName.prepend(dir));
    if ( file.open(QIODevice::ReadWrite) )
    {
        QTextStream oustream( &file );
        while (instream.readLineInto(&line)) {
            oustream << line << endl;
        }
        file.close();
    }
    setProgressBar(false);
}

//===========================================================================
void MainWindow::plProfileEventSize()
{
    // draws the event size as a function of the LHC period
    // the data are in mPLData
    // the name of LHC periods are in mPLDataName

    QChart *chart = new QChart();
    chart->setTheme(QChart::ChartThemeBlueIcy);
    QMetaEnum me = QMetaEnum::fromType<PlotOptions>();
    QString sopt = me.key(kEventSizeProfile);
    sopt.remove("k");
    sopt.remove("Profile");
    chart->setTitle(QString("ALICE %1 during RUN2").arg(sopt));


    QCategoryAxis *axisX = new QCategoryAxis();
    for (qint32 index = 0; index < mPlDataName.size(); index++)
        axisX->append(mPlDataName.at(index), index);
    axisX->setRange(0, mPlDataName.size());
    axisX->setTitleText("LHC period");
    chart->addAxis(axisX, Qt::AlignBottom);

    QValueAxis *axisY = new QValueAxis;
    axisY->setTickCount(9);
    axisY->setLabelFormat("%i");
    axisY->setTitleText("Event Size (kB)");
    chart->addAxis(axisY, Qt::AlignLeft);

    QLineSeries *series = new QLineSeries;
    series->setName("Average Event Size (kB)");
    double ymax = 0;
    for (qint32 x = 0; x < mPlData.size(); x++) {
        QVector<double> *data = mPlData.at(x);
        double y = data->at(0);
        if (y > ymax)
            ymax = y;
        series->append(x, y);
    }

    int exp = qFloor(qLn(ymax) / qLn(10));
    if (ymax  < qPow(10., exp))
            ymax = qPow(10., exp);
    else {
            ymax = qCeil(ymax / qPow(10, exp)) * qPow(10, exp);
    }

    axisY->setMax(ymax);
    axisY->setMin(0);
    chart->addSeries(series);
    series->attachAxis(axisY);
    series->attachAxis(axisX);

    QString title("Average Event Size");

    QChartView *chartView = new QChartView();
    chartView->setAttribute(Qt::WA_DeleteOnClose);
    chartView->setWindowTitle(title);
    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumSize(640, 480);
    chartView->setChart(chart);

    for (QMdiSubWindow *sw : mMdiArea->subWindowList())
        if(sw->windowTitle() == title) {
            mMdiArea->removeSubWindow(sw);
            sw->close();
        }

    mMdiArea->addSubWindow(chartView)->show();
    chartView->setStyleSheet("background-color:white");
}

//===========================================================================
void MainWindow::plProfileMandO()
{
    // specific plot for M&O data

    PlTableModel *model = new PlTableModel();

    // get the data
    qint32 columns = 4; // year in ms; year; M&O payers; CPU/M&O payer
    qint32 rows    = 5;
    model->setColRow(columns, rows);
    qint32 colYears       = 0;
    qint32 colYear        = 1;
    qint32 colMaO         = 2;
    qint32 colCPUperMaO   = 3;
    QVector<QString> headers(columns);
    headers.replace(colYears, "");
    headers.replace(colYear, "Year");
    headers.replace(colMaO, "M&O payers");
    headers.replace(colCPUperMaO, "CPU per M&O (HEPSpec06)");
    model->setHeader(headers);
    double payersMax = 0.0;
    double cpupPayerMax = 0.0;
    for (int year = mDEStart->date().year(); year <= mDEEnd->date().year(); year++) {
        QVector<double> *dataVec = new QVector<double>(columns - 2);
        ALICE::instance().doReqAndPle(QString::number(year));
        qint32 payers = ALICE::instance().countMOPayers();
        if (payers > payersMax)
            payersMax = payers;
        double cpuPerPayers = ALICE::instance().getRequired(Tier::kT1, Resources::kCPU, QString::number(year)) +
                              ALICE::instance().getRequired(Tier::kT2, Resources::kCPU, QString::number(year)) +
                              ALICE::instance().getRequired(Tier::kT1, Resources::kCPU, QString::number(year));
        if (payers == 0)
            cpuPerPayers = 0;
         else
            cpuPerPayers = cpuPerPayers * 1000 / payers;
        if (cpuPerPayers > cpupPayerMax)
            cpupPayerMax = cpuPerPayers;
        dataVec->replace(colMaO - 2, payers);
        dataVec->replace(colCPUperMaO - 2, cpuPerPayers);
        model->addData(QString::number(year), dataVec);
    }
    double y1max = payersMax;
    int exp = qFloor(qLn(y1max) / qLn(10));
    if (y1max  < qPow(10., exp))
            y1max = qPow(10., exp);
    else {
            y1max = qCeil(y1max / qPow(10, exp)) * qPow(10, exp);
    }

    double y2max = cpupPayerMax;
    exp = qFloor(qLn(y2max) / qLn(10));
    if (y2max  < qPow(10., exp))
            y2max = qPow(10., exp);
    else
            y2max = qCeil(y2max / qPow(10, exp)) * qPow(10, exp);

    QTableView *tableView = new QTableView;
    tableView->setModel(model);
    tableView->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    tableView->setAlternatingRowColors(true);
    tableView->hideColumn(0);

    QChart *chart = new QChart;
    chart->setTheme(QChart::ChartThemeHighContrast);
    QMetaEnum me = QMetaEnum::fromType<PlotOptions>();
    QString sopt = me.key(kMandOProfile);
    sopt.remove("k");
    sopt.remove("Profile");
    chart->setTitle(QString("ALICE %1 for RUN2").arg(sopt));

    QDateTimeAxis *axisX = new QDateTimeAxis;
    axisX->setTickCount(rows);
    axisX->setFormat("yyyy");
    axisX->setTitleText("Year");
    chart->addAxis(axisX, Qt::AlignBottom);

    QValueAxis *axisY = new QValueAxis;
    axisY->setLabelFormat("%i");
    axisY->setTitleText("M&O payers");
    axisY->setMax(y1max);
    axisY->setMin(0.0);
    chart->addAxis(axisY, Qt::AlignLeft);

    QLineSeries *series = new QLineSeries;
    series->setName("M&O payers");
    QVXYModelMapper *mapper = new QVXYModelMapper(mMdiArea);
    mapper->setModel(model);
    mapper->setXColumn(colYears);
    mapper->setYColumn(colMaO);
    mapper->setSeries(series);
    chart->addSeries(series);
    series->attachAxis(axisY);
    series->attachAxis(axisX);

    QScatterSeries *sseries = new QScatterSeries(mMdiArea);
    mapper = new QVXYModelMapper;
    mapper->setModel(model);
    mapper->setXColumn(colYears);
    mapper->setYColumn(colMaO);
    mapper->setSeries(sseries);
    chart->addSeries(sseries);
    sseries->attachAxis(axisY);
    sseries->attachAxis(axisX);
    sseries->setColor(series->color());

    QList<QLegendMarker*> markers = chart->legend()->markers(sseries);
    QLegendMarker *marker = markers.at(0);
    marker->setVisible(false);

    series = new QLineSeries;
    series->setName("Required CPU resources per M&O payers");
    mapper = new QVXYModelMapper(mMdiArea);
    mapper->setModel(model);
    mapper->setXColumn(colYears);
    mapper->setYColumn(colCPUperMaO);
    mapper->setSeries(series);
    chart->addSeries(series);

    axisY = new QValueAxis;
    axisY->setLabelFormat("%i");
    axisY->setTitleText("CPU per M&O payers (HEPSpec06)");
    axisY->setMax(y2max);
    axisY->setMin(0.0);
    axisY->setLinePenColor(series->pen().color());

    chart->addAxis(axisY, Qt::AlignRight);
    series->attachAxis(axisY);
    series->attachAxis(axisX);

    sseries = new QScatterSeries(mMdiArea);
    mapper = new QVXYModelMapper;
    mapper->setModel(model);
    mapper->setXColumn(colYears);
    mapper->setYColumn(colCPUperMaO);
    mapper->setSeries(sseries);
    chart->addSeries(sseries);
    sseries->attachAxis(axisY);
    sseries->attachAxis(axisX);
    sseries->setColor(series->color());

    markers = chart->legend()->markers(sseries);
    marker = markers.at(0);
    marker->setVisible(false);
    QChartView *chartView = new QChartView();

    chartView->setRenderHint(QPainter::Antialiasing);
    chartView->setMinimumSize(640, 480);
    chartView->setChart(chart);

    tableView->resizeColumnsToContents();
    tableView->setMaximumHeight(180);

    QSplitter *splitter = new QSplitter(Qt::Vertical);
    splitter->addWidget(tableView);
    splitter->addWidget(chartView);

    QVBoxLayout *allLayout = new QVBoxLayout();
    allLayout->addWidget(splitter);

    QList<QMdiSubWindow*> swList = mMdiArea->subWindowList();
    QString title = "ALICE M&O payers";

    for (QMdiSubWindow *sw : swList)
        if (sw->windowTitle() == title)
            mMdiArea->removeSubWindow(sw);

    QWidget *allTogether = new QWidget();
    allTogether->setWindowTitle(title);
    allTogether->setLayout(allLayout);

    mMdiArea->addSubWindow(allTogether)->show();
    allTogether->setStyleSheet("background-color:white");
}


//===========================================================================
void MainWindow::readMonthlyReport(QDate date)
{
    // read the montly reports for CPU, disk and tape usage provided by ML

    ALICE::instance().readMonthlyReport(date);
}

//===========================================================================
void MainWindow::saveData(PlotOptions opt)
{
    Q_UNUSED(opt);
    QString filename="Data.txt";
    QFile file( filename );
    if ( file.open(QIODevice::ReadWrite) )
    {
        QTextStream stream( &file );
        stream << "something" << endl;
    }
}

//===========================================================================
void MainWindow::selectDate(ActionOptions opt)
{
    //selects a date

    QWidget *datesel = new QWidget();
    datesel->setAttribute(Qt::WA_DeleteOnClose);
    datesel->setWindowTitle("Select a date");

    QLabel *lDate = new QLabel("Date", datesel);
    QDate date(QDate::currentDate());
    mDEStart = new QDateEdit(datesel);
    mDEStart->setMaximumDate(QDate::currentDate());
    mDEStart->setMinimumDate(QDate(2014, 1, 1));
    mDEStart->setCalendarPopup(true);
    mDEStart->setDate(date);

    QPushButton *okButton = new QPushButton("OK", datesel);
    connect(okButton, &QPushButton::clicked, this, [this, opt]{ validateDate(opt); });

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(lDate, 0, 0, 1, 1, Qt::AlignLeft);
    layout->addWidget(mDEStart, 0, 1, 1, 1, Qt::AlignLeft);
    layout->addWidget(okButton, 2, 0, 1, 2, Qt::AlignHCenter);
    datesel->setLayout(layout);
    datesel->show();
}

//===========================================================================
void MainWindow::selectDates(PlotOptions opt, QDate dateMax)
{
    //selects a start and end date

    QWidget *datesel = new QWidget();
    datesel->setAttribute(Qt::WA_DeleteOnClose);
    datesel->setWindowTitle("Select start and end date");

    QLabel *startDate = new QLabel("Start Date");
    QLabel *endDate   = new QLabel("End Date");
    QDate dateStart(QDate::currentDate());
    mDEStart = new QDateEdit();
    mDEStart->setMaximumDate(QDate::currentDate());
    mDEStart->setMinimumDate(QDate(2014, 1, 1));
    mDEStart->setCalendarPopup(true);
    mDEStart->setDate(dateStart);
    QDate dateEnd = mDEStart->date().addMonths(-1);
    mDEEnd = new QDateEdit();
    mDEEnd->setMaximumDate(dateMax);
    mDEEnd->setMinimumDate(QDate(2014, 2, 1));
    mDEEnd->setCalendarPopup(true);
    mDEEnd->setDate(dateEnd);

    QPushButton *okButton = new QPushButton("OK", datesel);
    connect(okButton, &QPushButton::clicked, this, [opt, this]{ validateDates(opt); });

    QGridLayout *layout = new QGridLayout;
    layout->addWidget(startDate, 0, 0, 1, 1, Qt::AlignLeft);
    layout->addWidget(mDEStart, 0, 1, 1, 1, Qt::AlignLeft);
    layout->addWidget(endDate,   1, 0, 1, 1, Qt::AlignLeft);
    layout->addWidget(mDEEnd, 1, 1, 1, 1, Qt::AlignLeft);
    layout->addWidget(okButton, 2, 0, 1, 2, Qt::AlignHCenter);
    datesel->setLayout(layout);
    datesel->show();
}

//===========================================================================
void MainWindow::selectDates(LoadOptions opt)
{
    //selects a start and end date

    QWidget *datesel = new QWidget();

    datesel->setAttribute(Qt::WA_DeleteOnClose);
    datesel->setWindowTitle("Select start and end date");

    mDEStart = new QDateEdit();

    QLabel *startDate = new QLabel;
    QGridLayout *layout = new QGridLayout;
    layout->addWidget(startDate, 0, 0, 1, 1, Qt::AlignLeft);

    startDate->setText("Start Date");
    QDate today = QDate::currentDate();
    mDEStart->setDate(today.addMonths(-1));
    mDEStart->setMaximumDate(today);
    mDEStart->setMinimumDate(QDate(2014, 9, 1));
    mDEStart->setCalendarPopup(true);
    mDEEnd = new QDateEdit();
    mDEEnd->setDate(QDate(mDEStart->date().year(), mDEStart->date().month(), mDEStart->date().daysInMonth()));
    layout->addWidget(mDEStart, 0, 1, 1, 1, Qt::AlignLeft);

    QPushButton *okButton = new QPushButton("OK", datesel);

    connect(okButton, &QPushButton::clicked, this, [opt, this]{ validateDates(opt); });


    layout->addWidget(okButton, 2, 0, 1, 2, Qt::AlignHCenter);
    datesel->setLayout(layout);
    datesel->show();
}

//===========================================================================
void MainWindow::setProgressBar(bool on)
{
    // activate and desactivate progress bar when downloading data

    if (on) {
        mProgressBarWidget = new QWidget();
        mProgressBarWidget->setAttribute(Qt::WA_DeleteOnClose);
        mProgressBarWidget->setLayout(new QVBoxLayout);

        mProgressBar = new QProgressBar(mProgressBarWidget);
        mProgressBarWidget->layout()->addWidget(mProgressBar);

        mDownLoadText = new QLabel(mProgressBarWidget);
        mDownLoadText->setText(QString("Downloading from %1").arg(mURL));
        mDownLoadText->setAlignment(Qt::AlignHCenter);
        mProgressBarWidget->layout()->addWidget(mDownLoadText);
        mProgressBarWidget->show();
    } else {
        mProgressBarWidget->close();
    }
}
