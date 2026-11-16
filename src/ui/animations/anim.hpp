#ifndef ANIM_HPP
#define ANIM_HPP

#include <QWidget>
#include <qabstractanimation.h>
#include <qanimationgroup.h>
#include <qeasingcurve.h>
#include <qgraphicseffect.h>
#include <qlayout.h>
#include <qlayoutitem.h>
#include <qlist.h>
#include <qlogging.h>
#include <qobject.h>
#include <qparallelanimationgroup.h>
#include <qpropertyanimation.h>
#include <qtmetamacros.h>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QParallelAnimationGroup>
#include <qwidget.h>

class Animation : public QObject
{
    Q_OBJECT

public:
    Animation(QObject *parent = nullptr)
        : QObject(parent)
    {

    }

public slots:
    static void fadeOut(QWidget *widget, QParallelAnimationGroup *group = nullptr, int duration = 500)
    {
        qDebug() << "widget:" << widget;

        QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(widget);
        widget->setGraphicsEffect(effect);

        QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity", widget);
        anim->setDuration(duration);
        anim->setStartValue(1.0);
        anim->setEndValue(0.0);

        if (group) {  
            group->addAnimation(anim);
            return;
        }
        
        anim->setEasingCurve(QEasingCurve::OutQuad);
        connect(anim, &QPropertyAnimation::finished, widget, [widget] () {
            widget->hide();
            widget->setGraphicsEffect(nullptr);
        });

        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void fadeOutNestedLayout(QLayout *layout, int duration = 500)
    {
        QParallelAnimationGroup *group = new QParallelAnimationGroup(layout);

        QList<QWidget *> widgets;
        collectFade(layout, group, widgets, duration);

        if (!group->animationCount()) {
            delete group;
            return;
        }

        connect(group, &QAnimationGroup::finished, layout, [widgets] () {
            for (QWidget *w : widgets) {
                w->hide();
                w->setGraphicsEffect(nullptr);
            }
        });

        group->start(QAbstractAnimation::DeleteWhenStopped);
    }

private:
    static void collectFade(QLayout *layout, QParallelAnimationGroup *group,
                            QList<QWidget *> &widgets, int duration = 500)
    {
        for (int i = 0; i < layout->count(); ++i)
        {
            QLayoutItem *item = layout->itemAt(i);
            if (QLayout *l = item->layout())
                collectFade(l, group, widgets, duration);
            else if (QWidget *w = item->widget()) {
                fadeOut(w, group, duration);
                widgets << w;
            }
        }
    }

    
};

#endif // ANIM_HPP