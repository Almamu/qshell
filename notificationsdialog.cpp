#include <QWidget>
#include <QApplication>
#include <QScreen>
#include <QDebug>
#include <algorithm>

#include <KF5/KWindowSystem/KWindowSystem>

#include "notificationsdialog.h"
#include "frame.h"
#include "panel.h"
#include "shell.h"
#include "model.h"

Q::NotificationsDialog::NotificationsDialog(QWidget *button) : QWidget(), myButton(button)
{
    Model *m = dynamic_cast<Model*>(button);
    if(m)
        frame = new Q::Frame(m->shell());
    else
        frame = new Q::Frame();
    frame->setCentralWidget(this);
    move(0, 0);
    
    connect(button, SIGNAL(clicked()), this, SLOT(toggle()));
    connect(KWindowSystem::self(), &KWindowSystem::activeWindowChanged, [this](WId wid) {
        if(frame->isVisible() && frame->winId() != wid)
            frame->hide();
    });
};

void Q::NotificationsDialog::updateDialog()
{
    resize(frame->width(), frame->height());
    QRect geo = QGuiApplication::primaryScreen()->geometry();
    Model *m = dynamic_cast<Model*>(myButton);
    if(!m) return;
    Shell *shell = m->shell();
    frame->move(
        std::min(geo.width() - frame->width(), shell->getStrutLeft() + myButton->x() + frame->width() / 2),
        std::min(geo.height() - frame->height(), shell->getStrutTop() + myButton->y())
    );

    Display *display = QX11Info::display();
    Atom atom = XInternAtom(display, "_KDE_SLIDE", false);

    QVarLengthArray<long, 1024> data(4);

    data[0] = 0;
    data[1] = 1;
    data[2] = 200;
    data[3] = 200;

    XChangeProperty(display, frame->winId(), atom, atom, 32, PropModeReplace,
            reinterpret_cast<unsigned char *>(data.data()), data.size());
};


void Q::NotificationsDialog::toggle()
{
    frame->setVisible(!frame->isVisible());
    KWindowSystem::setState(frame->winId(), NET::SkipTaskbar);
};
