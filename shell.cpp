#include <QApplication>
#include <QDebug>
#include <QLabel>
#include <QRect>
#include <QScreen>
#include <QMap>
#include <QPixmap>
#include <QWindow>

#include <KF5/KConfigCore/KConfigGroup>
#include <KF5/KConfigCore/KSharedConfig>
#include <KF5/KWindowSystem/KWindowSystem>

#include <signal.h>

#include "shell.h"
#include "desktop.h"
#include "panel.h"
#include "tasks.h"
#include "model.h"
#include "dash.h"
#include "volume.h"
#include "network.h"
#include "date.h"

Q::ShellApplication::ShellApplication(int &argc, char **argv) : QApplication(argc, argv)
{
     QCoreApplication::setApplicationName("qshell");
     QCoreApplication::setApplicationVersion("0.1");
     Shell *myShell = new Shell();
};

// ----------

Q::Shell::Shell() :
QWidget( 0, Qt::Window | Qt::FramelessWindowHint ),
strut_left(0),
strut_right(0),
strut_top(0),
strut_bottom(0)
{
    setAttribute(Qt::WA_X11NetWmWindowTypeDock, true);
    KWindowSystem::setState(winId(), NET::SkipTaskbar);
    KWindowSystem::setOnAllDesktops( winId(), true );
    setMask( QRegion(-1,-1,1,1) );
    show();
    
    myDesktop = new Desktop(this);
    myDesktop->show();
    
    myDash = new Dash(this);
    
    loadAll();
    
    connect( QGuiApplication::primaryScreen(), SIGNAL(geometryChanged(QRect)), this, SLOT(geometryChanged()) );
};

// Slots
void Q::Shell::geometryChanged()
{
    calculateStruts();
};

// Configurations
void Q::Shell::saveAll()
{
    KSharedConfig::Ptr sharedConfig = KSharedConfig::openConfig("qshellrc");
    
    // models
    foreach (Model *m, myModels) {
        KConfigGroup grp = sharedConfig->group(m->name());
        m->save(&grp);
    }
    
    // desktop
    KConfigGroup desktopGroup = sharedConfig->group("Q::Desktop");
    myDesktop->save(&desktopGroup);
    
    // shell
    KConfigGroup shGroup = sharedConfig->group("Q::Shell");
    QStringList list;
    foreach (Panel *p, myPanels)
        list.append(p->name());
    shGroup.writeEntry("Panels", list.join(","));
};

void Q::Shell::save(Model *m)
{
    KConfigGroup grp = KSharedConfig::openConfig("qshellrc")->group(m->name());
    m->save(&grp);
};

void Q::Shell::loadAll()
{
    KSharedConfig::Ptr sharedConfig = KSharedConfig::openConfig("qshellrc");
    
    // models
    QStringList groups = sharedConfig->groupList();
    foreach (const QString &group, groups)
    {
        Model *m = getModelByName(group);
    }
    
    KConfigGroup grp;
    
    // desktop
    grp = sharedConfig->group("Q::Desktop");
    myDesktop->load(&grp);
    
    // shell
    KConfigGroup shGroup = sharedConfig->group("Q::Shell");
    QStringList panels = shGroup.readEntry("Panels", QStringList());
    foreach (const QString &panel, panels)
    {
        Panel *m = static_cast<Panel *>(getModelByName(panel));
        if(m)
            addPanel(m);
    }
    calculateStruts();
    QString styleSheetLocation = shGroup.readEntry("Stylesheet", QString());
    if(!styleSheetLocation.isEmpty())
    {
        QFile file(styleSheetLocation);
        if (file.open(QIODevice::ReadOnly | QIODevice::Text))
        {
            styleSheet = QString::fromUtf8(file.readAll());
            setStyleSheet(styleSheet);
        }
    }
    
    // dash
    grp = sharedConfig->group("Q::Dash");
    myDash->load(&grp);
};

#define COND_LOAD_MODEL(s,m_) else if(type == s) { m = new m_(name, this); static_cast<m_ *>(m)->load(&group); }

Q::Model *Q::Shell::getModelByName(const QString& name, Model *parent)
{
    KSharedConfig::Ptr sharedConfig = KSharedConfig::openConfig("qshellrc");
    if(myModels.contains(name))
        return myModels.value(name);
    else if(sharedConfig->hasGroup(name))
    {
        Model *m;
        KConfigGroup group = sharedConfig->group(name);
        QString type = group.readEntry("Type", "");
        if(type.isEmpty())
            return 0;
        COND_LOAD_MODEL("Panel", Panel)
        COND_LOAD_MODEL("Tasks", Tasks)
        else if(type == "Task" && parent) { // prevents loading outside of Tasks
            m = new Task(dynamic_cast<Tasks*>(parent), name);
            static_cast<Task *>(m)->load(&group);
        }
        COND_LOAD_MODEL("DashButton", DashButton)
        COND_LOAD_MODEL("Volume", Volume)
        COND_LOAD_MODEL("Network", Network)
        COND_LOAD_MODEL("Date", Date)
        else
            return 0;
        
        myModels.insert(name, m);
        return m;
    }
    else
        return 0;
};

// Panels
void Q::Shell::addPanel(Q::Panel *panel)
{
    qDebug() << "add panel" << panel->name();
    myPanels.append(panel);
    panel->show(); //TODO
};

// Struts
void Q::Shell::calculateStruts()
{
    QSize geometry = QGuiApplication::primaryScreen()->availableSize();
    strut_left   = 0;
    strut_right  = 0;
    strut_top    = 0;
    strut_bottom = 0;
    foreach (Q::Panel *panel, myPanels)
        if(panel->struts())
            if(panel->position() == Q::PanelPosition::Left)
                strut_left += panel->width();
            else if(panel->position() == Q::PanelPosition::Right)
                strut_right += panel->width();
            else if(panel->position() == Q::PanelPosition::Top)
                strut_top += panel->height();
            else
                strut_bottom += panel->height();
    KWindowSystem::setStrut(winId(),strut_left,strut_right,strut_top,strut_bottom);
};

// ----------

void signalHandler(int signal)
{
    if (signal == SIGTERM || signal == SIGQUIT || signal == SIGINT)
        QApplication::quit();
}

int main (int argc, char *argv[])
{
     signal(SIGTERM, signalHandler);
     signal(SIGQUIT, signalHandler);
     signal(SIGINT, signalHandler);
    // SIGSEG by KCrash

    Q::ShellApplication app(argc, argv);
    return app.exec();
}