#pragma once

#include <QPushButton>
#include <QCalendarWidget>

#include <KF5/KConfigCore/KConfigGroup>

#include "model.h"
#include "panel.h"
#include "notificationsdialog.h"

namespace Q {

class Shell;
class Date;
class DateDialog : public NotificationsDialog {
public:
    DateDialog(Date *date);
    inline QBoxLayout *boxLayout() { return static_cast<QBoxLayout*>(layout()); };
private:
    Date *myDate;
    QCalendarWidget *calendar;
};

class Date : public QPushButton, public Model {
    Q_OBJECT
public:
    Date(const QString &name, Shell *shell);
    ~Date() { myDateDialog->deleteLater(); };
    void load(KConfigGroup *grp) override;
private slots:
    void update();
private:
    QString format;
    DateDialog *myDateDialog;
};

}
