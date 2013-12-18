#include "remotedisplaywidget.h"
#include "remotedisplaywidget_p.h"
#include "freerdpclient.h"

#include <QDebug>
#include <QThread>
#include <QPointer>
#include <QPaintEvent>

RemoteDisplayWidgetPrivate::RemoteDisplayWidgetPrivate(RemoteDisplayWidget *q)
    : q_ptr(q) {
    processorThread = new QThread(q);
    processorThread->start();
}

void RemoteDisplayWidgetPrivate::onAboutToConnect() {
    qDebug() << "ON CONNECT";
}

void RemoteDisplayWidgetPrivate::onConnected() {
    qDebug() << "ON CONNECTED";
}

void RemoteDisplayWidgetPrivate::onDisconnected() {
    qDebug() << "ON DISCONNECTED";
}

typedef RemoteDisplayWidgetPrivate Pimpl;

RemoteDisplayWidget::RemoteDisplayWidget(QWidget *parent)
    : QWidget(parent), d_ptr(new RemoteDisplayWidgetPrivate(this)) {
    Q_D(RemoteDisplayWidget);
    setAttribute(Qt::WA_OpaquePaintEvent);
    setAttribute(Qt::WA_NoSystemBackground);
    d->eventProcessor = new FreeRdpClient;
    d->eventProcessor->moveToThread(d->processorThread);

    connect(d->eventProcessor, SIGNAL(aboutToConnect()), d, SLOT(onAboutToConnect()));
    connect(d->eventProcessor, SIGNAL(connected()), d, SLOT(onConnected()));
    connect(d->eventProcessor, SIGNAL(disconnected()), d, SLOT(onDisconnected()));
    connect(d->eventProcessor, SIGNAL(desktopUpdated()), this, SLOT(update()));
}

RemoteDisplayWidget::~RemoteDisplayWidget() {
    Q_D(RemoteDisplayWidget);
    if (d->eventProcessor) {
        QMetaObject::invokeMethod(d->eventProcessor, "requestStop");
    }
    d->processorThread->quit();
    d->processorThread->wait();

    delete d_ptr;
}

void RemoteDisplayWidget::setDesktopSize(quint16 width, quint16 height) {
    Q_D(RemoteDisplayWidget);
    d->desktopSize = QSize(width, height);
    QMetaObject::invokeMethod(d->eventProcessor, "setSettingDesktopSize",
        Q_ARG(quint16, width), Q_ARG(quint16, height));
}

void RemoteDisplayWidget::connectToHost(const QString &host, quint16 port) {
    Q_D(RemoteDisplayWidget);

    QMetaObject::invokeMethod(d->eventProcessor, "setSettingServerHostName",
        Q_ARG(QString, host));
    QMetaObject::invokeMethod(d->eventProcessor, "setSettingServerPort",
        Q_ARG(quint16, port));

    qDebug() << "Connecting to" << host << ":" << port;
    QMetaObject::invokeMethod(d->eventProcessor, "run");
}

QSize RemoteDisplayWidget::sizeHint() const {
    Q_D(const RemoteDisplayWidget);
    if (d->desktopSize.isValid()) {
        return d->desktopSize;
    }
    return QWidget::sizeHint();
}

void RemoteDisplayWidget::paintEvent(QPaintEvent *event) {
    Q_D(RemoteDisplayWidget);
    d->eventProcessor->paintDesktopTo(this, event->rect());
}
