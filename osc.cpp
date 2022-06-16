#include "osc.h"

#include <QNetworkProxy>
#include <QtEndian>
#include <QDebug>

static const char END = 0300;
static const char ESC = 0333;
static const char ESC_END = 0334;
static const char ESC_ESC = 0335;

#ifdef DEBUG_DATA

static QString dumpBin(const QByteArray &data)
{
    QString ret = "";
    for(int i = 0; i < data.size(); i++)
    {
        char c = data.at(i);
        if(c >= 32 && c < 127) ret += c;
        else ret += "\\x" + QString("%1").arg(int(c), 2, 16, QLatin1Char('0'));
    }
    return ret;
}

#endif

OSC::OSC(QObject *parent)
    : QObject{parent}
{
    socket_ = new QTcpSocket(parent);
    socket_->setProxy(QNetworkProxy::NoProxy);
    socket_->setSocketOption(QAbstractSocket::LowDelayOption, 1);
    socket_->setSocketOption(QAbstractSocket::KeepAliveOption, 1);
    QObject::connect(socket_, &QAbstractSocket::connected, this, [=]() {emit connected();});
    QObject::connect(socket_, &QAbstractSocket::disconnected, this, [=]() {emit disconnected();});
    QObject::connect(socket_, &QAbstractSocket::stateChanged, this, [=](QAbstractSocket::SocketState s) {setProperty("state", s);});
    QObject::connect(socket_, &QAbstractSocket::readyRead, this, &OSC::onReadyRead);
}

void OSC::connect()
{
    socket_->connectToHost(host_, port_);
}

void OSC::disconnect()
{
    socket_->disconnectFromHost();
}

void OSC::sendMessage(const QString &addr, const QVariantList &args)
{
    QString typeSpec = ",";
    for(const QVariant &v : args)
    {
        switch(v.typeId())
        {
        case QMetaType::Int:
            typeSpec += 'i';
            break;
        case QMetaType::Float:
            typeSpec += 'f';
            break;
        case QMetaType::QString:
            typeSpec += 's';
            break;
        default:
            qWarning() << "Unsupported type" << v.typeId();
            return;
        }
    }

    QByteArray buf;
    QDataStream stream(&buf, QIODevice::WriteOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    pushString(stream, addr);
    pushString(stream, typeSpec);
    for(const QVariant &v : args)
    {
        switch(v.typeId())
        {
        case QMetaType::Int:
            pushInt(stream, v.toInt());
            break;
        case QMetaType::Float:
            pushFloat(stream, v.toFloat());
            break;
        case QMetaType::QString:
            pushString(stream, v.toString());
            break;
        }
    }

    sendPacket(buf);
}

void OSC::sendPacket(const QByteArray &data)
{
#ifdef DEBUG_DATA
    qDebug() << "SENDING PACKET:" << dumpBin(data);
#endif

    // handle SLIP encoding here:

    QByteArray buf;
    buf.append(END);
    for(int i = 0; i < data.size(); i++)
    {
        char c = data.at(i);
        if(c == END)
        {
            buf.append(ESC);
            buf.append(ESC_END);
        }
        else if(c == ESC)
        {
            buf.append(ESC);
            buf.append(ESC_ESC);
        }
        else buf.append(c);
    }
    buf.append(END);
    socket_->write(buf);
}

void OSC::onReadyRead()
{
    // handle SLIP decoding here:

    QByteArray data = socket_->readAll();
    for(int i = 0; i < data.size(); i++)
    {
        char c = data.at(i);
        if(escape_)
        {
            escape_ = false;
            if(c == ESC_END)
                buf_.append(END);
            else if(c == ESC_ESC)
                buf_.append(ESC);
            else
            {
                qWarning() << "SLIP error: ESC followed by a byte that is not ESC_END or ESC_ESC";
                buf_.append(c);
            }
        }
        else if(c == END)
        {
            if(!buf_.isEmpty())
                processIncomingPacket();
            buf_.clear();
        }
        else if(c == ESC)
        {
            escape_ = true;
        }
        else
        {
            buf_.append(c);
        }
    }
}

void OSC::processIncomingPacket()
{
#ifdef DEBUG_DATA
    qDebug() << "RECEIVED PACKET:" << dumpBin(buf_);
#endif

    QDataStream stream(&buf_, QIODevice::ReadOnly);
    stream.setByteOrder(QDataStream::BigEndian);
    stream.setFloatingPointPrecision(QDataStream::SinglePrecision);

    QString addr = popString(stream);
    QByteArray typeSpec = popString(stream).mid(1).toLocal8Bit();
    QVariantList args;
    for(int i = 0; i < typeSpec.length(); i++)
    {
        char t = typeSpec.at(i);
        switch(t)
        {
        case 'i':
            args.append(QVariant::fromValue(popInt(stream)));
            break;
        case 'f':
            args.append(QVariant::fromValue(popFloat(stream)));
            break;
        case 's':
            args.append(QVariant::fromValue(popString(stream)));
            break;
        default:
            qWarning() << "Unsupported type" << QString(t);
            return;
        }
    }
    emit message(addr, args);
}

void OSC::pushString(QDataStream &stream, QString v)
{
    QByteArray vbuf = v.toUtf8();
    int l = vbuf.length();
    for(int i = 0; i < l; i++)
        stream << vbuf.at(i);
    stream << '\0';
    l++;
    while(l % 4)
    {
        stream << '\0';
        l++;
    }
}

void OSC::pushInt(QDataStream &stream, int32_t v)
{
    stream << v;
}

void OSC::pushFloat(QDataStream &stream, float v)
{
    stream << v;
}

QString OSC::popString(QDataStream &stream)
{
    QByteArray buf;
    char c;
    while(true)
    {
        for(int i = 0; i < 4; i++)
        {
            stream >> c;
            if(c)
                buf.append(c);
        }
        if(c == '\0') break;
    }
    return QString(buf);
}

int32_t OSC::popInt(QDataStream &stream)
{
    int32_t ret;
    stream >> ret;
    return ret;
}

float OSC::popFloat(QDataStream &stream)
{
    float ret;
    stream >> ret;
    return ret;
}
