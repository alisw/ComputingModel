// Y. Schutz November 2016

#ifndef MAINWINDOW_H
#define MAINWINDOW_H


#include <QtNetwork>

#include <QDate>
#include <QMainWindow>
#include <QPlainTextEdit>
#include <QString>
#include <QTableView>

#include "alice.h"

class QMdiArea;
class QMdiSubWindow;
class ConsoleWidget;

struct MyLessThan {
    bool operator()(const QString &s1, const QString &s2) const {
        int it1 = s1.toInt();
        int it2 = s2.toInt();
        if (it1 < it2)
            return true;
        return false;
    }
};

class QLabel;
class QProgressBar;
class QDateEdit;

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    enum PlotOptions {kMandOProfile, kRequirementsProfile, kPledgesProfile, kRegisteredDataProfile, kUsageProfile, kTierEfficiencyProfile, kUserEfficiencyProfile,
                      kEventSizeProfile};

    Q_ENUM (PlotOptions)

    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void        createConsol();
    static bool isDebug() { return mDebug; }
    void        list(ALICE::ListOptions val);

protected:
    void paintEvent(QPaintEvent *);

private slots:
    void        mousePressEvent(QMouseEvent *event);
    void        onTableClicked(const QModelIndex &index);
    void        parsePlotUrlFile(PlotOptions opt);
    void        printCurrentWindow() const;
    void        resizeView() {mTableConsol->resizeColumnsToContents();}
    void        selectDates(PlotOptions opt);
    void        showNetworkError(QNetworkReply::NetworkError er);
    void        transferProgress(qint64 readBytes, qint64 totalBytes);
    void        validateDates(MainWindow::PlotOptions opt);

private:
    void        createActions();
    void        createMenu();
    static void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    void        setDebugMode(bool val);
    void        doeReqAndPle(QString year);
    void        getDataFromWeb(MainWindow::PlotOptions opt);
    void        getDataFromFile(MainWindow::PlotOptions opt);
    void        keyPressEvent(QKeyEvent *event);
    void        loadGlance(QString year);
    void        loadPledges(QString year);
    void        loadRequirements(QString year);
    void        plot(qint32 opt);
    void        plProfile(PlotOptions opt);
    void        plProfileEventSize();
    void        plProfileMandO();
    void        plProfile(PlotOptions opt,  Resources::Resources_type type);
    void        plRegisteredData(PlotOptions opt);
    void        plTierEfficiency(PlotOptions opt);
    void        plUserEfficiency(PlotOptions opt);
    void        readMonthlyReport(QDate date);
    void        saveData(PlotOptions opt);



    QMenu                   *mActionsMenu;       // Menu Tab with various actions
    static bool             mDebug;              // True for running debug mode
    QAction                 *mDebugOffAction;    // Action for debug mode off
    QAction                 *mDebugOnAction;     // Action for debug mode on
    QMenu                   *mDebugMenu;         // Menu Tab to set the debug on/off
    QList<QAction*>         mDoReqPle;           // Triggers doing the final table of requirements and pledges
    QLabel                  *mDownLoadText;      // The text associated with the download status window + mProgressBar
    QDateEdit               *mDEEnd;             // End date for the data to be plotted
    QList<QAction*>         mGlanceData;         // Triggers Glance data reading
    QAction                 *mListAction;        // Action to list various stuff
    ConsoleWidget           *mLogConsol;         // The console where to write logging info
    QMdiSubWindow           *mLogConsolView;     // The view of the log consol in the mdi area
    QMdiArea                *mMdiArea;           // The mdi area in the centralwidget
    QNetworkAccessManager   *mNetworkManager;    // The network manager
    QList<QVector<double>*> mPlData;             // Data to be plotted
    QList<QString>          mPlDataName;         // Name of the data to be plotted
    QList<QAction*>         mPledgedResources;   // Triggers reading pledged ressources
    QList<QAction*>         mPlAct;              // Triggers plots
    QMenu                   *mPlMenu;            // Menu for plotting
    QProgressBar            *mProgressBar;       // A progress bar used when downloading files from www
    QList<QMenu*>           mReportsMenus;       // Menus for reading reports/year
    QList<QAction*>         mRequiredResources;  // Triggers reading required ressources
    QDateEdit               *mDEStart;           // Start date for the data to be plotted
    QTableView              *mTableConsol;       // The table where the all stuff table is displayed
    QMdiSubWindow           *mTableConsolView;   // The view of the previous table
    QString                 mURL;                // URL name where to get data from
};

#endif // MAINWINDOW_H
