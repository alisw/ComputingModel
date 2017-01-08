// Y. Schutz November 2016

#include <QAction>
#include <QAreaSeries>
#include <QCalendarWidget>
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
#include <QValueAxis>
#include <QVBoxLayout>
#include <QVXYModelMapper>

#include "consolewidget.h"
#include "logger.h"
#include "mainwindow.h"
#include "mymdiarea.h"
#include "pltablemodel.h"

QT_CHARTS_USE_NAMESPACE

bool MainWindow::mDebug = false;

//===========================================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // ctor

    mDebug = false;
    mDownLoadText   = Q_NULLPTR;
    mNetworkManager = Q_NULLPTR;
    mProgressBar    = Q_NULLPTR;
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

    QTextStream stream(&data);
    QString line;

    qDeleteAll(mPlData.begin(), mPlData.end());
    mPlData.clear();
    mPlDataName.clear();

    QVector<qint64> dates;
    stream.readLineInto(&line); // skip the header line
    QStringList strList = line.split(",");
    for (QString str : strList)
        mPlDataName.append(str);
    while (stream.readLineInto(&line)) {
        QVector<double> *dataVec = new QVector<double>(3);
        QStringList strlist = line.split(",");
        QString date = strlist.at(0);
        dates.append(date.toLongLong());
        QString sdata1 = strlist.at(1);
        double data1 = sdata1.toDouble() / 1E9; // in PB;
        dataVec->replace(1, data1);
        QString sdata2 = strlist.at(2);
        double data2 = sdata2.toDouble() / 1E9; // in PB
        dataVec->replace(2, data2);
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
        plData(opt);
        break;
    case kPledgesProfile:
        saveData(opt);
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
void MainWindow::validateDates(PlotOptions opt)
{
    //sets start and end date and launches data collection from url
    qobject_cast<QWidget*>(sender()->parent())->close();
    QDate dStart = mDEStart->date();
    QDate dEnd   = mDEEnd->date();
    qint64 diffS = dStart.daysTo(QDate::currentDate()) * 24 * 3600 * 1000;
    qint64 diffE = dEnd.daysTo(QDate::currentDate()) * 24 *3600 * 1000;

    switch (opt) {
    case kRegisteredDataProfile:
    {
        mURL = QString("http://alimonitor.cern.ch/display");
        mURL += "?imgsize=1024x600";
        mURL += QString("&interval.max=%1&interval.min=%2").arg(diffS).arg(diffE);
        mURL += "&page=DAQ2%2Fdaq_size&plot_series=aldaqgw01-daq03.cern.ch&plot_series=aldaqgw02-daq03.cern.ch&download_data_csv=true";
        plProfile(kRegisteredDataProfile);
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

    QDir dataDir(":/data/");

    // Glance loading action
//    for (QString subDir : dataDir.entryList(QDir::Dirs)) {
//        QAction *act = new QAction(this);
//        act->setText(QString("%1 status").arg(subDir));
//        connect(act, &QAction::triggered, this, [this, subDir]{ loadGlance(subDir); });
//        mGlanceData.append(act);
//    }

    // Required resources loading action
//    dataDir.setPath(":/data/");
//    for (QString subDir : dataDir.entryList(QDir::Dirs)) {
//        QAction *act = new QAction(this);
//        act->setText(QString("%1 required").arg(subDir));
//        connect(act, &QAction::triggered, this, [subDir, this]{ loadRequirements(subDir); });
//        mRequiredResources.append(act);
//    }

    // pledges loading action
    dataDir.setPath(":/data/");
    for (QString subDir : dataDir.entryList(QDir::Dirs)) {
        QAction *act = new QAction(this);
        act->setText(QString("%1 pledged").arg(subDir));
        connect(act, &QAction::triggered, this, [subDir, this]{ loadPledges(subDir); });
        mPledgedResources.append(act);
    }

    // monthly reports reading action
    dataDir.setPath(":/data/");
    dataDir.setFilter(QDir::AllDirs);
    for (QString subDir : dataDir.entryList(QDir::Dirs)) {
        QMenu *menu = new QMenu(subDir);
        QDir dirDir(QString(":/data/%1/").arg(subDir));
        QStringList monthes = dirDir.entryList(QDir::Dirs);
        MyLessThan lt;
        qSort(monthes.begin(), monthes.end(), lt);
        for (QString subsubDir : monthes) {
            QAction *act = new QAction(this);
            QDate date(subDir.toInt(), subsubDir.toInt(), 1);
            QString month = QDate::longMonthName(subsubDir.toInt());
            act->setText(QString("%1").arg(month));
            connect(act, &QAction::triggered, this, [date, this]{ readMonthlyReport(date); });
            menu->addAction(act);
        }
        mReportsMenus.append(menu);
    }

    // list actions
    mListAction = new QAction(this);
    mListAction->setText("List FA + MandO");
    mListAction->setStatusTip("list all funding agencies and their respective M&O payers");
    connect(mListAction, &QAction::triggered, this, [this]{list(ALICE::kFA);});

    // do the requirements and pledges table
    dataDir.setPath(":/data/");
    for (QString subDir : dataDir.entryList(QDir::Dirs)) {
        QAction *act = new QAction(this);
        act->setText(QString("%1").arg(subDir));
        connect(act, &QAction::triggered, this, [subDir, this]{ doeReqAndPle(subDir); });
        mDoReqPle.append(act);
    }

    // plots
    QMetaEnum me = QMetaEnum::fromType<PlotOptions>();
    for (qint32 index = 0; index < me.keyCount(); index++) {
        QString     swhat  = me.key(index);
        swhat.remove(0, 1); // removes the "k"
        qint32      what   = me.value(index);
        QAction *act = new QAction(this);
        act->setText(QString("plot %1").arg(swhat));
        connect(act, &QAction::triggered, this, [what, this]{ plot(what); });
        mPlAct.append(act);
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

    // Actions
    mActionsMenu = menuBar()->addMenu(tr("&Actions"));

    QMenu *readG = new QMenu(tr("Load Glance data"));
    for (QAction * act : mGlanceData)
        readG->addAction(act);
    mActionsMenu->addMenu(readG);

    QMenu *readRQ = new QMenu(tr("Load requirements"));
    for (QAction * act : mRequiredResources)
        readRQ->addAction(act);
    mActionsMenu->addMenu(readRQ);

    QMenu *readPL = new QMenu(tr("Load Pledges"));
    for (QAction * act : mPledgedResources)
        readPL->addAction(act);
    mActionsMenu->addMenu(readPL);

    QMenu *doReqPle = new QMenu(tr("Requirements and Pledges"));
    for (QAction * act : mDoReqPle)
        doReqPle->addAction(act);
    mActionsMenu->addMenu(doReqPle);

    QMenu *readRP = new QMenu(tr("read reports"));
    for (QMenu *menu : mReportsMenus) {
        readRP->addMenu(menu);
    }
    mActionsMenu->addMenu(readRP);

    mPlMenu = menuBar()->addMenu(tr("Do various plots"));
    for (QAction * act : mPlAct)
        mPlMenu->addAction(act);

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
void MainWindow::doeReqAndPle(QString year)
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

    switch (event->key()) {
    case 80: {// key cmd-p to print the active window
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
        QMessageBox *mb = new QMessageBox(this);
        mb->setText(QString("Window saved as %1").arg(oFileName));
        mb->show();
    }
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
void MainWindow::loadGlance(QString year)
{
    // read data from Glance and generated csv

    ALICE::instance().loadGlanceData(year);
}

//===========================================================================
void MainWindow::loadPledges(QString year)
{
    // read pledges from CSV file

    ALICE::instance().loadPledges(year);
}

//===========================================================================
void MainWindow::loadRequirements(QString year)
{
    // read data from csv file

    ALICE::instance().loadRequirements(year);

}

//===========================================================================
void MainWindow::plot(qint32 opt)
{
    // do plots according to the directive of what

    switch (opt) {
    case kMandOProfile:
        plProfile(kMandOProfile);
        break;
    case kRequirementsProfile:
        plProfile(kRequirementsProfile);
        break;
    case kPledgesProfile:
        plProfile(kPledgesProfile);
        break;
    case kRegisteredDataProfile:
        selectDates(kRegisteredDataProfile);
        break;
    case kUsageProfile:
        plProfile(kUsageProfile);
        break;
    default:
        break;
   }
}

//===========================================================================
void MainWindow::plData(PlotOptions opt)
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
    chart->setTitle(QString("ALICE %1 during past year").arg(sopt));

    int columns = mPlData.at(0)->size();
    int rows   = mPlData.size();

    QDateTimeAxis *axisX = new QDateTimeAxis;
    axisX->setTickCount(12); // 12 months
    axisX->setFormat("MM-yyyy");
    axisX->setTitleText("Date");
    chart->addAxis(axisX, Qt::AlignBottom);

    QValueAxis *axisY = new QValueAxis;
    axisY->setTickCount(9);
    axisY->setLabelFormat("%i");
    QString title;
    switch (opt) {
    case kRegisteredDataProfile:
        title = "Registered Data (PB)";
        break;
    default:
        break;
    }
    axisY->setTitleText(title);
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
                y = points[irow].y() + row->at(col);
            } else
                y = row->at(col);
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
void MainWindow::getDataFromWeb(PlotOptions opt)
{
    // plot the registered data profile during the past year

    QWidget *w = new QWidget();
    w->setAttribute(Qt::WA_DeleteOnClose);
    w->setLayout(new QVBoxLayout);

    mProgressBar = new QProgressBar(w);
    w->layout()->addWidget(mProgressBar);

    mDownLoadText = new QLabel;
    mDownLoadText->setText(QString("Downloading from %1").arg(mURL));
    mDownLoadText->setAlignment(Qt::AlignHCenter);
    w->layout()->addWidget(mDownLoadText);
    w->show();

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
void MainWindow::plProfile(PlotOptions opt, Resources::Resources_type type)
{
    // plot requirements CPU, Disk and tape as a function of year

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

    QVector<double> *dataVec = new QVector<double>(columns - 2);
    QString xAxisFormat;
    QString xAxisTitle;
    if (opt == kRequirementsProfile || opt == kPledgesProfile) {
        xAxisFormat = "yyyy";
        xAxisTitle = "Year";
        QDir dataDir(":/data/");
        for (QString year : dataDir.entryList(QDir::Dirs)) {
            if (opt == kRequirementsProfile) {
                dataVec->replace(colT0 - 2, ALICE::instance().getRequired(Tier::kT0,   type, year));
                dataVec->replace(colT1 - 2, ALICE::instance().getRequired(Tier::kT1,   type, year));
                dataVec->replace(colT2 - 2, ALICE::instance().getRequired(Tier::kT2,   type, year));
                dataVec->replace(colTo - 2, ALICE::instance().getRequired(Tier::kTOTS, type, year));
                model->addData(year, dataVec);
            } else if (opt == kPledgesProfile) {
                dataVec->replace(colT0 - 2, ALICE::instance().getPledged(Tier::kT0,   type, year));
                dataVec->replace(colT1 - 2, ALICE::instance().getPledged(Tier::kT1,   type, year));
                dataVec->replace(colT2 - 2, ALICE::instance().getPledged(Tier::kT2,   type, year));
                dataVec->replace(colTo - 2, ALICE::instance().getPledged(Tier::kTOTS, type, year));
                model->addData(year, dataVec);
            }
        }
    } else if (opt == kUsageProfile) {
        xAxisFormat = "MM-yyyy";
        xAxisTitle = "Date";
        ALICE::instance().setDrawTable(false);
        QDir dataDir(":/data/");
        for (QString year : dataDir.entryList(QDir::Dirs)) {
            QDir subDataDir(QString("%1/%2").arg(dataDir.path()).arg(year));
            QStringList monthes = subDataDir.entryList(QDir::Dirs);
            MyLessThan lt;
            qSort(monthes.begin(), monthes.end(), lt);
            for (QString month : monthes) {
                QDate date(year.toInt(), month.toInt(), 1);
                dataVec->replace(colT0 - 2, ALICE::instance().getUsed(Tier::kT0,   type, date));
                dataVec->replace(colT1 - 2, ALICE::instance().getUsed(Tier::kT1,   type, date));
                dataVec->replace(colT2 - 2, ALICE::instance().getUsed(Tier::kT2,   type, date));
                dataVec->replace(colTo - 2, ALICE::instance().getUsed(Tier::kTOTS, type, date));
                model->addData(QDateTime(date), dataVec);
            }
        }
        ALICE::instance().setDrawTable(true);
    }

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
    QMetaEnum me = QMetaEnum::fromType<Resources::Resources_type>();
    QString swhat = me.key(type);
    swhat.remove(0, 1); // removes the "k"
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
    axisY->setLabelFormat("%i");
    QString title;
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
void MainWindow::plProfile(PlotOptions opt)
{
    // plot CPU, disk and tape required profile

    switch (opt) {
    case kMandOProfile:
    {
        plProfileMandO();
        break;
    }
    case kRequirementsProfile:
    {
        plProfile(kRequirementsProfile, Resources::kCPU);
        plProfile(kRequirementsProfile, Resources::kDISK);
        plProfile(kRequirementsProfile, Resources::kTAPE);

        break;
    }
    case kPledgesProfile:
    {
        plProfile(kPledgesProfile, Resources::kCPU);
        plProfile(kPledgesProfile, Resources::kDISK);
        plProfile(kPledgesProfile, Resources::kTAPE);

        break;
    }

    case kRegisteredDataProfile:
    {
        getDataFromWeb(kRegisteredDataProfile);

        break;
    }
    case kUsageProfile:
    {
        plProfile(kUsageProfile, Resources::kCPU);
        plProfile(kUsageProfile, Resources::kDISK);
        plProfile(kUsageProfile, Resources::kTAPE);

        break;
    }
    default:
        break;
    }

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
    QDir dataDir(":/data/");
    double payersMax = 0.0;
    double cpupPayerMax = 0.0;
    for (QString year : dataDir.entryList(QDir::Dirs)) {
        QVector<double> *dataVec = new QVector<double>(columns - 2);
        ALICE::instance().doReqAndPle(year);
        qint32 payers = ALICE::instance().countMOPayers();
        if (payers > payersMax)
            payersMax = payers;
        double cpuPerPayers = ALICE::instance().getRequired(Tier::kT1, Resources::kCPU, year) +
                              ALICE::instance().getRequired(Tier::kT2, Resources::kCPU, year) +
                              ALICE::instance().getRequired(Tier::kT1, Resources::kCPU, year);
        if (payers == 0)
            cpuPerPayers = 0;
         else
            cpuPerPayers = cpuPerPayers * 1000 / payers;
        if (cpuPerPayers > cpupPayerMax)
            cpupPayerMax = cpuPerPayers;
        dataVec->replace(colMaO - 2, payers);
        dataVec->replace(colCPUperMaO - 2, cpuPerPayers);
        model->addData(year, dataVec);
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
    QString filename="Data.txt";
    QFile file( filename );
    if ( file.open(QIODevice::ReadWrite) )
    {
        QTextStream stream( &file );
        stream << "something" << endl;
    }
}

//===========================================================================
void MainWindow::selectDates(PlotOptions opt)
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
    mDEStart->setMinimumDate(QDate(2014, 9, 1));
    mDEStart->setCalendarPopup(true);
    mDEStart->setDate(dateStart);
    QDate dateEnd = mDEStart->date().addMonths(-1);
    mDEEnd = new QDateEdit();
    mDEEnd->setMaximumDate(QDate::currentDate());
    mDEEnd->setMinimumDate(QDate(2014, 9, 1));
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
