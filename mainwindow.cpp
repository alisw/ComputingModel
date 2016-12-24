// Y. Schutz November 2016

#include <QtCore>
#include <QAction>
#include <QDebug>
#include <QDir>
#include <QHeaderView>
#include <QLabel>
#include <QMenuBar>
#include <QMdiArea>
#include <QMdiSubWindow>
#include <QMessageBox>
#include <QPainter>
#include <QStatusBar>
#include <QVBoxLayout>

#include "consolewidget.h"
#include "logger.h"
#include "mainwindow.h"
#include "mymdiarea.h"

bool MainWindow::mDebug = false;

//===========================================================================
MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
{
    // ctor

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
    setCentralWidget(mMdiArea);

    createConsol();

    createActions();
    createMenu();

    QString message = tr("Welcome to ALICE Computing Resources tool");
    statusBar()->showMessage(message);

    setMinimumSize(160, 160);
    resize(480, 320);

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

    // Glance loading action
    QDir dataDir(":/data/");
    for (QString subDir : dataDir.entryList(QDir::Dirs)) {
        QAction *act = new QAction(this);
        act->setText(QString("%1 status").arg(subDir));
        connect(act, &QAction::triggered, this, [this, subDir]{ loadGlance(subDir); });
        mGlanceData.append(act);
    }

    // Required resources loading action
    dataDir.setPath(":/data/");
    for (QString subDir : dataDir.entryList(QDir::Dirs)) {
        QAction *act = new QAction(this);
        act->setText(QString("%1 required").arg(subDir));
        connect(act, &QAction::triggered, this, [subDir, this]{ loadRequirements(subDir); });
        mRequiredResources.append(act);
    }

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
    connect(Logger::instance(), SIGNAL(messageReceived(QString)), mLogConsol,SLOT(setMessage(QString)));

    // direct the log info to the console
//    qInstallMessageHandler(customMessageHandler);

    // the console for table view
    ALICE::instance().initTableViewModel();
    mView = new QTableView(mMdiArea);
    mView->setEditTriggers(QAbstractItemView::NoEditTriggers);
    mView->setModel(ALICE::instance().getModel());
    mMdiArea->addSubWindow(mView);
    mView->resizeColumnsToContents();
    mView->activateWindow();
    mView->setAlternatingRowColors(true);
    connect(mView, SIGNAL(clicked(QModelIndex)), this, SLOT(resizeView()));

    // hide the column when header is clicked

    connect(mView->horizontalHeader(), SIGNAL(sectionClicked(int)), mView, SLOT(hideColumn(int)));

    // hide the row when row number is clicked

    connect(mView->verticalHeader(), SIGNAL(sectionClicked(int)), mView, SLOT(hideRow(int)));

    // popup a window with the clicked funding agency
    connect(mView, SIGNAL(clicked(const QModelIndex &)), this, SLOT(onTableClicked(const QModelIndex &)));


    mView->addAction(new QAction("Salut"));
    mView->setContextMenuPolicy(Qt::ActionsContextMenu);

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

    QMenu *readG = new QMenu("Load Glance data");
    for (QAction * act : mGlanceData)
        readG->addAction(act);
    mActionsMenu->addMenu(readG);

    QMenu *readRQ = new QMenu("Load requirements");
    for (QAction * act : mRequiredResources)
        readRQ->addAction(act);
    mActionsMenu->addMenu(readRQ);

    QMenu *readPL = new QMenu("Load Pledges");
    for (QAction * act : mPledgedResources)
        readPL->addAction(act);
    mActionsMenu->addMenu(readPL);

    QMenu *doReqPle = new QMenu("Requirements and Pledges");
    for (QAction * act : mDoReqPle)
        doReqPle->addAction(act);
    mActionsMenu->addMenu(doReqPle);

    QMenu *readRP = new QMenu("read reports");
    for (QMenu *menu : mReportsMenus) {
        readRP->addMenu(menu);
    }
    mActionsMenu->addMenu(readRP);

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

    }
    else {
        mDebugOnAction->setText("On");
        mDebugOffAction->setText("✓ Off");
        mLogConsolView->setVisible(false);
    }
}

//===========================================================================
void MainWindow::doeReqAndPle(QString year)
{

    // read M&O information from glance
    mView->setWindowTitle(QString(tr("Ressources in %1")).arg(year));
    ALICE::instance().doReqAndPle(year);
    mView->horizontalHeader()->setStretchLastSection(true);
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
void MainWindow::readMonthlyReport(QDate date)
{
    // read the montly reports for CPU, disk and tape usage provided by ML


    ALICE::instance().readMonthlyReport(date);

}
