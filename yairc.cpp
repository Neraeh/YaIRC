#include "yairc.h"
#include "ircmessageformatter.h"

YaIRC::YaIRC(QWidget *parent) : QMainWindow(parent), ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    ui->lineEdit->installEventFilter(this);

    settings = new QSettings("TheShayy", "YaIRC", this);

    if (settings->value("fullscreen", false).toBool())
        this->showFullScreen();
    else
        this->setGeometry(settings->value("window", this->geometry()).toRect());

    ui->topic->setEnabled(settings->value("topic", true).toBool());
    ui->topic->setEnabled(settings->value("userlist", true).toBool());

    connection = new IrcConnection(this);

    parser = new IrcCommandParser(this);
    parser->setTolerant(true);
    parser->setTriggers(QStringList("/"));
    parser->addCommand(IrcCommand::Join, "JOIN <#channel> (<key>)");
    parser->addCommand(IrcCommand::CtcpAction, "ME [target] <message...>");
    parser->addCommand(IrcCommand::Mode, "MODE (<channel/user>) (<mode>) (<arg>)");
    parser->addCommand(IrcCommand::Nick, "NICK <nick>");
    parser->addCommand(IrcCommand::Part, "PART (<#channel>) (<message...>)");
    parser->addCommand(IrcCommand::Ping, "PING <user> [content]");
    // TODO: Compléter les commandes dispos
    // TODO: Affichage des évènements CTCP
    // TODO: Auto-reconnect si déconnecté

    completer = new IrcCompleter(this);
    completer->setParser(parser);
    connect(completer, SIGNAL(completed(QString, int)), this, SLOT(onCompleted(QString, int)));
    QShortcut* shortcut = new QShortcut(Qt::Key_Tab, this);
    connect(shortcut, SIGNAL(activated()), this, SLOT(onCompletion()));

    connect(ui->userList, SIGNAL(doubleClicked(QModelIndex)), this, SLOT(onUserActivated(QModelIndex)));

    bufferModel = new IrcBufferModel(connection);
    connect(bufferModel, SIGNAL(added(IrcBuffer*)), this, SLOT(onBufferAdded(IrcBuffer*)));
    connect(bufferModel, SIGNAL(removed(IrcBuffer*)), this, SLOT(onBufferRemoved(IrcBuffer*)));

    ui->chansList->setModel(bufferModel);
    connect(bufferModel, SIGNAL(channelsChanged(QStringList)), parser, SLOT(setChannels(QStringList)));
    connect(ui->chansList->selectionModel(), SIGNAL(currentChanged(QModelIndex, QModelIndex)), this, SLOT(onBufferActivated(QModelIndex)));

    connect(ui->lineEdit, SIGNAL(returnPressed()), this, SLOT(onTextEntered()));
    connect(ui->lineEdit, SIGNAL(textEdited(QString)), this, SLOT(onTextEdited()));

    connect(connection, SIGNAL(connected()), this, SLOT(onConnected()));
    connect(connection, SIGNAL(connecting()), this, SLOT(onConnecting()));
    connect(connection, SIGNAL(disconnected()), this, SLOT(onDisconnected()));

    connect(connection, SIGNAL(messageReceived(IrcMessage*)), this, SLOT(onMessageReceived(IrcMessage*)));
    connect(connection, SIGNAL(topicMessageReceived(IrcTopicMessage*)), this, SLOT(onTopicReceived(IrcTopicMessage*)));

    connect(ui->actionConnecter, SIGNAL(triggered()), this, SLOT(actionConnect()));
    connect(ui->actionD_connecter, SIGNAL(triggered()), this, SLOT(actionDisconnect()));
    connect(ui->actionPlein_cran, SIGNAL(triggered(bool)), this, SLOT(actionFullscreen(bool)));
    connect(ui->actionFermer_l_onglet, SIGNAL(triggered()), this, SLOT(actionCloseCurrent()));

    qputenv("IRC_DEBUG", "1");
    qsrand(QTime::currentTime().msec());

    QVariantMap replies;
    replies.insert("VERSION", "YaIRC alpha client");
    connection->setCtcpReplies(replies);

    connection->setHost("irc.freenode.net");
    connection->setUserName("YaIRC");
    connection->setNickName("YaIRC" + QString::number(qrand() % 9999));
    connection->setRealName("YaIRC alpha client");

    connection->sendCommand(IrcCommand::createJoin("#unobot"));
    connection->open();
    ui->textEdit->append(IrcMessageFormatter::formatMessage(": YaIRC alpha client initialized"));
}

YaIRC::~YaIRC()
{
    if (connection->isActive()) {
        connection->quit(connection->realName());
        connection->close();
    }
    delete parser;
    delete completer;
    delete bufferModel;
    delete connection;
    delete ui;
}

bool YaIRC::eventFilter(QObject* obj, QEvent* event)
{
    if (obj == ui->lineEdit) {
        if (event->type() == QEvent::KeyPress) {
            QKeyEvent* keyEvent = static_cast<QKeyEvent*>(event);
            if (keyEvent->key() == Qt::Key_Up) {
                ui->lineEdit->setText(inputHistory.previous());
            }
            else if (keyEvent->key() == Qt::Key_Down) {
                ui->lineEdit->setText(inputHistory.next());
            }
        }
        return false;
    }
    return QMainWindow::eventFilter(obj, event);
}

void YaIRC::closeEvent(QCloseEvent *event)
{
    if (connection->isActive()) {
        connection->quit(connection->realName());
        connection->close();
    }

    if (this->isFullScreen())
        settings->setValue("fullscreen", this->isFullScreen());
    else
        settings->setValue("window", this->geometry());

    settings->setValue("topic", ui->topic->isEnabled());
    settings->setValue("userlist", ui->userList->isEnabled());

    event->accept();
}

void YaIRC::onConnected()
{
    // TODO: Auto-join channels
    ui->actionConnecter->setDisabled(true);
    ui->actionD_connecter->setEnabled(true);
    ui->actionRe_joindre_un_channel->setEnabled(true);
    ui->textEdit->append(IrcMessageFormatter::formatMessage(tr(": Connected to %1").arg(connection->host())));
    ui->textEdit->append(IrcMessageFormatter::formatMessage(tr(": Joining channels...")));
}

void YaIRC::onConnecting()
{
    ui->actionConnecter->setDisabled(true);
    ui->textEdit->append(IrcMessageFormatter::formatMessage(tr(": Connecting to %1...").arg(connection->host())));
}

void YaIRC::onDisconnected()
{
    // TODO: Save channels and restore
    ui->actionConnecter->setEnabled(true);
    ui->actionD_connecter->setDisabled(true);
    ui->actionRe_joindre_un_channel->setDisabled(true);
    ui->textEdit->append(IrcMessageFormatter::formatMessage(tr(": Disconnected from %1").arg(connection->host())));
}

void YaIRC::onTextEdited()
{
    ui->lineEdit->setStyleSheet(QString());
}

void YaIRC::onTextEntered()
{
    QString input = ui->lineEdit->text();
    inputHistory.append(input);
    IrcCommand* command = parser->parse(input);
    if (command) {
        connection->sendCommand(command);

        if (command->type() == IrcCommand::Message || command->type() == IrcCommand::CtcpAction) {
            IrcMessage* msg = command->toMessage(connection->nickName(), connection);
            receiveMessage(msg);
            delete msg;
        }

        ui->lineEdit->clear();
    }
    else if (input.length() > 1) {
        QString error;
        QString command = ui->lineEdit->text().mid(1).split(" ", QString::SkipEmptyParts).value(0).toUpper();
        if (parser->commands().contains(command))
            error = tr("! Syntax: %1").arg(parser->syntax(command).replace("<", "&lt;").replace(">", "&gt;"));
        else
            error = tr("! Unknown command: %1").arg(command);
        ui->textEdit->append(IrcMessageFormatter::formatMessage(error));
        ui->lineEdit->setStyleSheet("background: salmon");
    }
}

void YaIRC::onCompletion()
{
    completer->complete(ui->lineEdit->text(), ui->lineEdit->cursorPosition());
}

void YaIRC::onCompleted(const QString &text, int cursor)
{
    ui->lineEdit->setText(text);
    ui->lineEdit->setCursorPosition(cursor);
}

void YaIRC::onBufferAdded(IrcBuffer* buffer)
{
    connect(buffer, SIGNAL(messageReceived(IrcMessage*)), this, SLOT(receiveMessage(IrcMessage*)));

    ui->actionFermer_l_onglet->setEnabled(true);

    QTextDocument* document = new QTextDocument(buffer);
    documents.insert(buffer, document);

    IrcUserModel* userModel = new IrcUserModel(buffer);
    userModel->setSortMethod(Irc::SortByTitle);
    userModels.insert(buffer, userModel);

    if (buffer->title().at(0) == '#') {
        int i = bufferModel->buffers().indexOf(buffer);
        if (i != -1)
            ui->chansList->setCurrentIndex(bufferModel->index(i));
    }
}

void YaIRC::onBufferRemoved(IrcBuffer *buffer)
{
    if (bufferModel->buffers().isEmpty())
        ui->actionFermer_l_onglet->setDisabled(true);
    topics.remove(buffer);
    delete userModels.take(buffer);
    delete documents.take(buffer);
}

void YaIRC::onBufferActivated(const QModelIndex &index)
{
    IrcBuffer* buffer = index.data(Irc::BufferRole).value<IrcBuffer*>();

    ui->textEdit->setDocument(documents.value(buffer));
    ui->textEdit->verticalScrollBar()->triggerAction(QScrollBar::SliderToMaximum);
    ui->userList->setModel(userModels.value(buffer));
    ui->topic->setText(topics.value(buffer));
    ui->topic->setCursorPosition(0);
    completer->setBuffer(buffer);

    if (buffer)
        parser->setTarget(buffer->title());
}

void YaIRC::onUserActivated(const QModelIndex &index)
{
    IrcUser* user = index.data(Irc::UserRole).value<IrcUser*>();

    if (user) {
        IrcBuffer* buffer = bufferModel->add(user->name());

        int i = bufferModel->buffers().indexOf(buffer);
        if (i != -1)
            ui->chansList->setCurrentIndex(bufferModel->index(i));
    }
}

static void appendHtml(QTextDocument* document, const QString& html)
{
    QTextCursor cursor(document);
    cursor.beginEditBlock();
    cursor.movePosition(QTextCursor::End);
    if (!document->isEmpty())
        cursor.insertBlock();
    cursor.insertHtml(html);
    cursor.endEditBlock();
}

void YaIRC::onMessageReceived(IrcMessage *message)
{
    bool ok;
    int code = message->command().toInt(&ok);
    if (!ok) return;

    switch (code) {
    case 431:
        ui->textEdit->append(IrcMessageFormatter::formatMessage(tr("! %1 said nickname is empty, connection aborted").arg(connection->host())));
        break;
    case 432:
        ui->textEdit->append(IrcMessageFormatter::formatMessage(tr("! Erroneous nickname, connection aborted")));
        break;
    case 433:
        ui->textEdit->append(IrcMessageFormatter::formatMessage(tr("! %1 is already in use, please pick another one or abort the connection").arg(connection->nickName())));
        break;
    }
}

void YaIRC::onTopicReceived(IrcTopicMessage *message)
{
    IrcBuffer* buffer = qobject_cast<IrcBuffer*>(sender());
    if (!buffer)
        buffer = ui->chansList->currentIndex().data(Irc::BufferRole).value<IrcBuffer*>();

    topics.insert(buffer, message->topic());
    if (buffer == ui->chansList->currentIndex().data(Irc::BufferRole).value<IrcBuffer*>()) {
        ui->topic->setText(message->topic());
        ui->topic->setCursorPosition(0);
    }
}

void YaIRC::receiveMessage(IrcMessage *message)
{
    IrcBuffer* buffer = qobject_cast<IrcBuffer*>(sender());
    if (!buffer)
        buffer = ui->chansList->currentIndex().data(Irc::BufferRole).value<IrcBuffer*>();

    QTextDocument* document = documents.value(buffer);
    if (document) {
        QString html = IrcMessageFormatter::formatMessage(message);
        if (!html.isEmpty()) {
            if (document == ui->textEdit->document())
                ui->textEdit->append(html);
            else
                appendHtml(document, html);
        }
    }
}

void YaIRC::actionConnect()
{
    connection->open();
}

void YaIRC::actionDisconnect()
{
    connection->quit(connection->realName());
    connection->close();
}

void YaIRC::actionFullscreen(bool state)
{
    if (state)
        this->showFullScreen();
    else
        this->showNormal();
}

void YaIRC::actionCloseCurrent()
{
    IrcBuffer* buffer = ui->chansList->currentIndex().data(Irc::BufferRole).value<IrcBuffer*>();
    if (buffer->isChannel())
        connection->sendCommand(IrcCommand::createPart(buffer->title()));
    else
        bufferModel->remove(buffer->title());
}
