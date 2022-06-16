#ifndef OSC_H
#define OSC_H

#include <QObject>
#include <QTcpSocket>

class OSC : public QObject
{
    Q_OBJECT
    Q_ENUMS(QAbstractSocket::SocketState)
    Q_PROPERTY(QString host MEMBER host_ NOTIFY hostChanged)
    Q_PROPERTY(quint16 port MEMBER port_ NOTIFY portChanged)
    Q_PROPERTY(QAbstractSocket::SocketState state MEMBER state_ NOTIFY stateChanged)

signals:
    void hostChanged();
    void portChanged();
    void stateChanged();

    void connected();
    void disconnected();

    void message(const QString &addr, const QVariantList &args);

public:
    explicit OSC(QObject *parent = nullptr);

public slots:
    void connect();
    void disconnect();
    void sendMessage(const QString &addr, const QVariantList &args = {});

private slots:
    void sendPacket(const QByteArray &data);
    void onReadyRead();
    void processIncomingPacket();

private:
    void pushString(QDataStream &stream, QString v);
    void pushInt(QDataStream &stream, int32_t v);
    void pushFloat(QDataStream &stream, float v);
    QString popString(QDataStream &stream);
    int32_t popInt(QDataStream &stream);
    float popFloat(QDataStream &stream);

private:
    QString host_;
    quint16 port_;
    QTcpSocket *socket_;
    QAbstractSocket::SocketState state_;
    QByteArray buf_;
    bool escape_{false};
};

#endif // OSC_H
