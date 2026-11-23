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
#include <qmargins.h>
#include <qobject.h>
#include <qparallelanimationgroup.h>
#include <qpropertyanimation.h>
#include <qtmetamacros.h>
#include <QPropertyAnimation>
#include <QGraphicsOpacityEffect>
#include <QParallelAnimationGroup>
#include <qwidget.h>

enum Fade {
    Out = 0,
    In  = 1
};

class Animation : public QObject
{
    Q_OBJECT

public:
    Animation(QObject *parent = nullptr)
        : QObject(parent)
    {

    }

public slots:
    static void fadeOutIn(QWidget *widget, QParallelAnimationGroup *group = nullptr, 
                          Fade fade = Fade::Out, int duration = 500)
    {
        QGraphicsOpacityEffect *effect = new QGraphicsOpacityEffect(widget);
        widget->setGraphicsEffect(effect);

        QPropertyAnimation *anim = new QPropertyAnimation(effect, "opacity", widget);
        anim->setDuration(duration);

        float startVal; float endVal;
        if (fade == Fade::In) {
            startVal = 0.0;
            endVal   = 1.0;
        } else if(fade == Fade::Out) {
            startVal = 1.0;
            endVal   = 0.0;
        }
        anim->setStartValue(startVal);
        anim->setEndValue(endVal);

        if (group) {  
            group->addAnimation(anim);
            return;
        }
        
        anim->setEasingCurve(QEasingCurve::OutQuad);
        connect(anim, &QPropertyAnimation::finished, widget, [widget, fade] () {
            if      (fade == Fade::In)   widget->show();
            else if (fade == Fade::Out)  widget->hide();
            widget->setGraphicsEffect(nullptr);
        });

        anim->start(QAbstractAnimation::DeleteWhenStopped);
    }

    void fadeNestedLayout(QLayout *layout, Fade fade, int duration = 500)
    {
        QList<QWidget *> widgets = collectWidgets(layout);
        if (widgets.isEmpty())  return;

        if (layout->contentsMargins() != QMargins(0, 0, 0, 0))
            m_margins = layout->contentsMargins();

        QParallelAnimationGroup *group = new QParallelAnimationGroup(layout);
        for (QWidget *w : widgets)
            fadeOutIn(w, group, fade, duration);
        
        if (!group->animationCount()) {
            delete group;
            return;
        }

        connect(group, &QAnimationGroup::finished, this, [this, layout, widgets, fade] () {
            for (QWidget *w : widgets)
            {
                if      (fade == Fade::In)   w->show();
                else if (fade == Fade::Out)  w->hide();

                w->setGraphicsEffect(nullptr);
            }

            if (fade == Fade::In) {
                layout->setContentsMargins(m_margins);
            } else if (fade == Fade::Out) {
                QMargins margins = layout->contentsMargins();
                if (!margins.isNull())  
                    m_margins = margins;
                layout->setContentsMargins(0, 0, 0, 0);
            }
        });

        group->start(QAbstractAnimation::DeleteWhenStopped);
    }

private:
    static QList<QWidget *> collectWidgets(QLayout *layout)
    {
        QList<QWidget *> result;
        for (int i = 0; i < layout->count(); ++i)
        {
            QLayoutItem *item = layout->itemAt(i);
            if (QLayout *l = item->layout())
                result += collectWidgets(l);
            else if (QWidget *w = item->widget())
                result << w;
        }

        return result;
    }

    QMargins m_margins;
};

#endif // ANIM_HPP