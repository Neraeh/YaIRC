#ifndef YAIRC_H
#define YAIRC_H

#include "ui_mainwindow.h"
#include "inputhistory.h"
#include <QSettings>
#include <QCloseEvent>
#include <QErrorMessage>
#include <QHash>
#include <QShortcut>
#include <QScrollBar>
#include <IrcConnection>
#include <IrcCommandParser>
#include <IrcCompleter>
#include <IrcBufferModel>
#include <IrcUserModel>
#include <IrcBuffer>
#include <IrcUser>
#include <IrcChannel>

namespace Ui {
    class YaIRC;
}

class YaIRC : public QMainWindow
{
    Q_OBJECT

public:
    explicit YaIRC(QWidget* parent = 0);
    ~YaIRC();
    QString getNick();

private:
    bool eventFilter(QObject *obj, QEvent *event);
    void closeEvent(QCloseEvent* event);

private slots:
    void onConnected();
    void onConnecting();
    void onDisconnected();

    void onTextEdited();
    void onTextEntered();

    void onCompletion();
    void onCompleted(const QString& text, int cursor);

    void onBufferAdded(IrcBuffer* buffer);
    void onBufferRemoved(IrcBuffer* buffer);

    void onBufferActivated(const QModelIndex& index);
    void onUserActivated(const QModelIndex& index);

    void onMessageReceived(IrcMessage* message);
    void onTopicReceived(IrcTopicMessage* message);
    void receiveMessage(IrcMessage* message);

    void actionConnect();
    void actionDisconnect();
    void actionFullscreen(bool state);
    void actionCloseCurrent();

private:
    Ui::MainWindow* ui;
    IrcConnection* connection;
    IrcCommandParser* parser;
    IrcCompleter* completer;
    IrcBufferModel* bufferModel;
    QHash<IrcBuffer*, IrcUserModel*> userModels;
    QHash<IrcBuffer*, QTextDocument*> documents;
    QHash<IrcBuffer*, QString> topics;
    InputHistory inputHistory;
    QSettings* settings;
};

#endif // YAIRC_H
