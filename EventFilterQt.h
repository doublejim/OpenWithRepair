#ifndef SHORTCUTS
#define SHORTCUTS

#include <QObject>
#include <QEvent>
#include <QKeyEvent>

enum Events {K_DELETE};

class EventFilterQt : public QObject
{
private:
    Q_OBJECT

signals:
    void eventActivated( Events e);

protected:
    bool eventFilter(QObject *obj, QEvent *event)
    {
        if (event->type() == QEvent::KeyPress) {

            QKeyEvent *keyEvent = static_cast<QKeyEvent *>(event);

            switch(keyEvent->key())
            {
            case Qt::Key_Delete:
                emit eventActivated( K_DELETE);
                return true;
            default:
                return QObject::eventFilter(obj, event);
            }

        } else {
            return QObject::eventFilter(obj, event);        // standard event processing
        }
    }
};

#endif // SHORTCUTS

