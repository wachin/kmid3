/*
    KMid2 MIDI/Karaoke Player - Updated for KDE5/Qt5
    Copyright (C) 2009-2010 Pedro Lopez-Cabanillas <plcl@users.sf.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef KMID_PARTVIEW_H
#define KMID_PARTVIEW_H

#include <QWidget>

#ifndef KMIDPART_EXPORT
# if defined(kmid_part_EXPORTS) || defined(MAKE_KMID_PART_LIB)
#  define KMIDPART_EXPORT Q_DECL_EXPORT
# else
#  define KMIDPART_EXPORT Q_DECL_IMPORT
# endif
#endif

class KMIDPART_EXPORT KMidPartView : public QWidget {
    Q_OBJECT
public:
    KMidPartView(QWidget *parent);
    virtual ~KMidPartView();

    enum Button {
        Play = 1,
        Stop = 2,
        Pause = 4,
        Seeker = 8
    };

    enum ExtraControl {
        Volume = 16,
        Transpose = 32,
        Tempo = 64
    };

    void resetTimePosition(qint64 totalTime);
    void setPosition(qint64 tick);
    void setPlayingState(bool playing);
    void setButtons(int buttons);

private slots:
    void slotPlayStop();
    void slotTimeSliderPressed();
    void slotTimeSlider(int);
    void slotVolumeSlider(int value);
    void slotPitchSlider(int value);
    void slotTempoSlider(int value);
    void slotTempoReset();

signals:
    void play();
    void pause();
    void stop();
    void seek(int pos);
    void volume(double volfactor);
    void transpose(int pitch);
    void speed(double tempofactor);
    void buttonsChanged(int);

private:
    class ViewPrivate;
    ViewPrivate* d;
};

#endif /* KMID_PARTVIEW_H */
