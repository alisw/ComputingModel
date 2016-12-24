// Y. Schutz November 2016

#ifndef MAINWINDOW_H
#define MAINWINDOW_H



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

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void        createConsol();
    static bool isDebug() { return mDebug; }
    void        list(ALICE::ListOptions val);

protected:
    void paintEvent(QPaintEvent *);

private slots:
    void        resizeView() {mView->resizeColumnsToContents();}
    void        onTableClicked(const QModelIndex &index);
private:
    void        createActions();
    void        createMenu();
    static void customMessageHandler(QtMsgType type, const QMessageLogContext &context, const QString &msg);
    void        setDebugMode(bool val);
    void        doeReqAndPle(QString year);
    void        loadGlance(QString year);
    void        loadPledges(QString year);
    void        loadRequirements(QString year);
    void        readMonthlyReport(QDate date);



    QMenu*           mActionsMenu;       // Menu Tab with various actions
    ConsoleWidget*   mLogConsol;         // the console where to write logging info
    static bool      mDebug;             // True for running debug mode
    QAction*         mDebugOffAction;    // Action for debug mode off
    QAction*         mDebugOnAction;     // Action for debug mode on
    QMenu*           mDebugMenu;         // Menu Tab to set the debug on/off
    QList<QAction*>  mDoReqPle;          // Triggers doing the final table of requirements and pledges
    QList<QAction*>  mGlanceData;        // Triggers Glance data reading
    QMdiArea*        mMdiArea;           // the mdi area in the centralwidget
    QMdiSubWindow*   mLogConsolView;     // the view of the log consol in the mdi area
    QList<QAction*>  mPledgedResources;  // Triggers reading pledged ressources
    QList<QMenu*>    mReportsMenus;      // Menus for reading reports/year
    QList<QAction*>  mRequiredResources; // Triggers reading required ressources
    QAction*         mListAction;        // Action to list various stuff
    QTableView*      mView;              // the view where the all stuff table is displayed
};

#endif // MAINWINDOW_H
