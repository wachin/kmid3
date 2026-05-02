/*
    KMid2 MIDI/Karaoke Player - Updated for KDE5/Qt5
    Copyright (C) 2009-2010 Pedro Lopez-Cabanillas <plcl@users.sf.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef KMID_PART_H
#define KMID_PART_H

#include <KParts/ReadOnlyPart>
#include <QDBusVariant>
#include <QUrl>

#include "midiobject.h"

#ifndef KMIDPART_EXPORT
# if defined(kmid_part_EXPORTS) || defined(MAKE_KMID_PART_LIB)
#  define KMIDPART_EXPORT Q_DECL_EXPORT
# else
#  define KMIDPART_EXPORT Q_DECL_IMPORT
# endif
#endif

class KAboutData;

namespace KMid {
    class Backend;
}
using namespace KMid;

/**
 * KMidPart is a KParts::ReadOnlyPart implementation for the MIDI/Karaoke player.
 */
class KMIDPART_EXPORT KMidPart : public KParts::ReadOnlyPart
{
    Q_OBJECT
    Q_PROPERTY(bool autoStart READ autoStart WRITE setAutoStart)
    Q_PROPERTY(QString midiConnection READ midiConnection WRITE setMidiConnection)
    Q_PROPERTY(double tempoFactor READ tempoFactor WRITE setTempoFactor)
    Q_PROPERTY(double volumeFactor READ volumeFactor WRITE setVolumeFactor)
    Q_PROPERTY(int transpose READ transpose WRITE setTranspose)

public:
    KMidPart(QWidget *parentWidget, QObject *parent, const QVariantList&);
    virtual ~KMidPart();

    bool isSeekable() const;
    qlonglong position() const;
    bool hasLength() const;
    qlonglong length() const;
    void seek(qlonglong ticks);

    static KAboutData *createAboutData();

    bool autoStart();
    QString midiConnection();
    QStringList metaData(const QString& key);
    double tempoFactor();
    double volumeFactor();
    int transpose();
    bool isMuted(int channel);
    QString currentSource() const;
    QDBusVariant songProperty(const QString& key);
    QDBusVariant channelProperty(int channel, const QString& key);

public slots:
    void pause();
    void play();
    void stop();
    void setAutoStart(bool start);
    void setMidiConnection(const QString conn);
    void setTempoFactor(double f);
    void setVolumeFactor(double f);
    void setTranspose(int t);
    void setMuted(int channel, bool muted);
    void reload();
    bool openUrl(const QUrl& url);
    bool openUrl(const QString& str);

signals:
    void tempoEvent(double tempo);
    void timeSignatureEvent(int num, int den);
    void midiTextEvent(int type, const QString& text);
    void midiNoteOnEvent(int channel, int note, int velocity);
    void midiNoteOffEvent(int channel, int note, int velocity);
    void midiControllerEvent(int channel, int ctl, int value);
    void midiKeyPressureEvent(int channel, int note, int value);
    void midiProgramEvent(int channel, int program);
    void midiChannelPressureEvent(int channel, int value);
    void midiPitchBendEvent(int channel, int value);
    void beat(int bar, int beat, int max);
    void tick(qlonglong time);
    void finished();
    void sourceChanged(const QString& source);

private slots:
    void slotLoaded(Backend *backend, const QString& library, const QString& name);
    void slotUpdateState(KMid::State, KMid::State);
    void slotSoftSynthStarted(const QString& pgm, const QStringList& messages);
    void slotSoftSynthErrors(const QString& pgm, const QStringList& messages);
    void slotSeek(int value);
    void slotTick(qint64);
    void slotFinished();
    void slotSourceChanged(QString);
    void slotTempoChanged(qreal);

protected:
    bool openFile() override;

private:
    void setupActions();
    void initialize();
    void connectMidiOutput();

    class KMidPartPrivate;
    KMidPartPrivate *d;
};

#endif // KMID_PART_H
