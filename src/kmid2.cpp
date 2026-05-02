/*
    KMid2 MIDI/Karaoke Player - Updated for KDE5/Qt5
    Copyright (C) 2009-2010 Pedro Lopez-Cabanillas <plcl@users.sf.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "kmid2.h"
#include "backend.h"
#include "backendloader.h"
#include "settings.h"
#include "pianola.h"
#include "channels.h"
#include "rhythmview.h"
#include "timelabel.h"
#include "playlistdialog.h"
#include "kmidadaptor.h"

#include <algorithm>
#include <QDropEvent>
#include <QPainter>
#include <QPrinter>
#include <QPrintDialog>
#include <QTextDocument>
#include <QToolTip>
#include <QTextCursor>
#include <QScrollBar>
#include <QDockWidget>
#include <QTimer>
#include <QTextCodec>
#include <QGridLayout>
#include <QVBoxLayout>
#include <QFrame>
#include <QFileInfo>
#include <QFile>
#include <QTextStream>
#include <QSaveFile>
#include <QFileDialog>
#include <QInputDialog>
#include <QMessageBox>
#include <QApplication>
#include <QListWidget>
#include <QListWidgetItem>
#include <QStandardPaths>
#include <QDBusConnection>

#include <KConfigDialog>
#include <KToggleAction>
#include <KActionCollection>
#include <KStandardAction>
#include <KLocalizedString>
#include <KMessageBox>
#include <KRecentFilesAction>
#include <KConfig>
#include <KConfigGroup>
#include <KSharedConfig>
#include <KIconLoader>
#include <KCodecs>

using namespace KMid;

KMid2::KMid2() : KXmlGuiWindow(),
      m_connected(false),
      m_seeking(false),
      m_seekamt(0),
      m_rtempo(0.0),
      m_loader(nullptr),
      m_currentBackend(nullptr),
      m_midiobj(nullptr),
      m_midiout(nullptr),
      m_settings(new Settings)
{
    (void) new KMidAdaptor(this);
    QDBusConnection::sessionBus().registerObject(QLatin1String("/KMid"), this);
    setAcceptDrops(true);
    setupDockWidgets();
    setupActions();
    setupGUI();
    initialize();
}

KMid2::~KMid2()
{
    delete m_settings;
}

void KMid2::slotLoaded(Backend *backend, const QString& library, const QString& name)
{
    MidiBackend midiBackend;
    midiBackend.backend = backend;
    midiBackend.library = library;
    midiBackend.name = name;
    m_backends.append(midiBackend);
    backend->setParent(this);
    qDebug() << library << name << backend->initialized();
    if ( backend != nullptr && backend->initialized() &&
         m_currentBackend == nullptr &&
         (m_settings->midi_backend().isEmpty() ||
          m_settings->midi_backend() == library) ) {
        m_midiobj = backend->midiObject();
        m_midiout = backend->midiOutput();
        m_midiout->setMidiMap(&m_mapper);
        connect(m_midiobj, SIGNAL(stateChanged(State,State)),
                SLOT(slotUpdateState(State,State)));
        connect(m_midiobj, SIGNAL(midiText(int,const QString&)),
                SLOT(slotMidiTextEvent(int,const QString&)));
        connect(m_midiobj, SIGNAL(tick(qint64)), SLOT(slotTick(qint64)));
        connect(m_midiobj, SIGNAL(finished()), SLOT(finished()));
        connect(m_midiobj, SIGNAL(currentSourceChanged(QString)),
                SLOT(slotSourceChanged(QString)));
        connect(m_midiobj, SIGNAL(timeSignatureChanged(int,int)),
                SLOT(slotTimeSignatureEvent(int,int)));
        connect(m_midiobj, SIGNAL(beat(int,int,int)),
                SLOT(slotBeat(int,int,int)));
        connect(m_midiobj, SIGNAL(tempoChanged(qreal)),
                SLOT(slotTempoChanged(qreal)));

        connect( m_midiobj, SIGNAL(midiText(int,const QString&)),
                 SIGNAL(midiTextEvent(int,const QString&)) );
        connect( m_midiobj, SIGNAL(finished()), SIGNAL(playerFinished()) );
        connect( m_midiobj, SIGNAL(currentSourceChanged(QString)),
                 SIGNAL(sourceChanged(QString)) );
        connect( m_midiobj, SIGNAL(timeSignatureChanged(int,int)),
                 SIGNAL(timeSignatureEvent(int,int)) );
        connect( m_midiobj, SIGNAL(beat(int,int,int)),
                 SIGNAL(beat(int,int,int)) );
        connect( m_midiobj, SIGNAL(midiNoteOn(int,int,int)),
                 SIGNAL(midiNoteOnEvent(int,int,int)) );
        connect( m_midiobj, SIGNAL(midiNoteOff(int,int,int)),
                 SIGNAL(midiNoteOffEvent(int,int,int)) );
        connect( m_midiobj, SIGNAL(midiController(int,int,int)),
                 SIGNAL(midiControllerEvent(int,int,int)) );
        connect( m_midiobj, SIGNAL(midiKeyPressure(int,int,int)),
                 SIGNAL(midiKeyPressureEvent(int,int,int)) );
        connect( m_midiobj, SIGNAL(midiProgram(int,int)),
                 SIGNAL(midiProgramEvent(int,int)) );
        connect( m_midiobj, SIGNAL(midiChannelPressure(int,int)),
                 SIGNAL(midiChannelPressureEvent(int,int)) );
        connect( m_midiobj, SIGNAL(midiPitchBend(int,int)),
                 SIGNAL(midiPitchBendEvent(int,int)) );

        if (backend->hasSoftSynths())
            backend->initializeSoftSynths(m_settings);
        m_currentBackend = backend;
        m_currentBackendLibrary = library;
    }
}

void KMid2::initialize()
{
    m_songName.clear();
    m_songEncoding.clear();
    m_playList.clear();
    m_loader = new BackendLoader(this);
    connect(m_loader, SIGNAL(loaded(Backend*,const QString&,const QString&)),
                      SLOT(slotLoaded(Backend*,const QString&,const QString&)));
    m_loader->loadAllBackends();
    if (m_currentBackend == nullptr) {
        KMessageBox::error(this, i18nc("@info","No MIDI backend loaded."),
                i18nc("@title:window","Fatal"));
        close();
        return;
    }

    connect(m_comboCodecs, SIGNAL(activated(int)), SLOT(slotSelectEncoding(int)));
    connect(m_timeSlider, SIGNAL(sliderPressed()), SLOT(slotTimeSliderPressed()));
    connect(m_timeSlider, SIGNAL(valueChanged(int)), SLOT(slotTimeSlider(int)));
    connect(m_tempoSlider, SIGNAL(sliderMoved(int)), SLOT(slotTempoSliderMoved(int)));
    connect(m_tempoSlider, SIGNAL(valueChanged(int)), SLOT(slotTempoSlider(int)));
    connect(m_volumeSlider, SIGNAL(sliderMoved(int)), SLOT(slotVolumeSliderMoved(int)));
    connect(m_volumeSlider, SIGNAL(valueChanged(int)), SLOT(slotVolumeSlider(int)));
    connect(m_pitchSlider, SIGNAL(sliderMoved(int)), SLOT(slotPitchSliderMoved(int)));
    connect(m_pitchSlider, SIGNAL(valueChanged(int)), SLOT(slotPitchSlider(int)));
    connect(m_btnResetTempo, SIGNAL(clicked()), SLOT(slotTempoReset()));

    m_pianola = new Pianola(this);
    connect(m_pianola, SIGNAL(closed()), SLOT(slotPianolaClosed()));
    connect(m_midiobj, SIGNAL(midiNoteOn(int,int,int)),
            m_pianola, SLOT(slotNoteOn(int,int,int)),
            Qt::QueuedConnection);
    connect(m_midiobj, SIGNAL(midiNoteOff(int,int,int)),
            m_pianola, SLOT(slotNoteOff(int,int,int)),
            Qt::QueuedConnection);
    connect(m_pianola, SIGNAL(noteOn(int,int,int)),
            m_midiout, SLOT(sendNoteOn(int,int,int)),
            Qt::QueuedConnection);
    connect(m_pianola, SIGNAL(noteOff(int,int,int)),
            m_midiout, SLOT(sendNoteOff(int,int,int)),
            Qt::QueuedConnection);

    m_channels = new Channels(this);
    connect(m_channels, SIGNAL(closed()), SLOT(slotChannelsClosed()));
    connect(m_midiobj, SIGNAL(midiNoteOn(int,int,int)),
            m_channels, SLOT(slotNoteOn(int,int,int)),
            Qt::QueuedConnection);
    connect(m_midiobj, SIGNAL(midiNoteOff(int,int,int)),
            m_channels, SLOT(slotNoteOff(int,int,int)),
            Qt::QueuedConnection);
    connect(m_midiobj, SIGNAL(midiProgram(int,int)),
            m_channels, SLOT(slotPatch(int,int)),
            Qt::QueuedConnection);
    connect(m_channels, SIGNAL(mute(int,bool)),
            m_midiout, SLOT(setMuted(int,bool)),
            Qt::QueuedConnection);
    connect(m_channels, SIGNAL(patch(int,int)),
            m_midiout, SLOT(sendProgram(int,int)),
            Qt::QueuedConnection);
    connect(m_channels, SIGNAL(volume(int,qreal)),
            m_midiout, SLOT(setVolume(int,qreal)),
            Qt::QueuedConnection);
    connect(m_channels, SIGNAL(lock(int,bool)),
            m_midiout, SLOT(setLocked(int,bool)),
            Qt::QueuedConnection);
    connect(m_channels, SIGNAL(name(int,const QString&)),
            m_pianola, SLOT(slotLabel(int,const QString&)),
            Qt::QueuedConnection);

    if ( m_currentBackend != nullptr && m_currentBackend->hasSoftSynths()) {
        connect( m_currentBackend, SIGNAL(softSynthStarted(const QString&,const QStringList&)),
                 SLOT(slotSoftSynthStarted(const QString&,const QStringList&)));
        connect( m_currentBackend, SIGNAL(softSynthErrors(const QString&,const QStringList&)),
                 SLOT(slotSoftSynthErrors(const QString&,const QStringList&)));
    }

    Qt::DockWidgetArea area = dockWidgetArea(m_volDock);
    if (area != Qt::NoDockWidgetArea)
        slotDockVolLocationChanged(area);

    finished();
    if (!m_settings->exec_fluid() && !m_settings->exec_timidity())
        connectMidiOutput();
}

void KMid2::setupActions()
{
    QAction* a;

    a = KStandardAction::quit(qApp, SLOT(quit()), actionCollection());
    a->setWhatsThis(i18nc("@info:whatsthis","Exit the application"));
    a = KStandardAction::open(this, SLOT(fileOpen()), actionCollection());
    a->setWhatsThis(i18nc("@info:whatsthis","Open a file from disk"));
    a = KStandardAction::preferences(this, SLOT(slotEditSettings()), actionCollection());
    a->setWhatsThis(i18nc("@info:whatsthis","Configure the program settings"));
    m_print = KStandardAction::print(this, SLOT(filePrint()), actionCollection());
    m_print->setWhatsThis(i18nc("@info:whatsthis","Print the lyrics"));
    m_recentFiles = KStandardAction::openRecent(this,
            SLOT(slotURLSelected(const QUrl&)), actionCollection());

    m_fileSaveLyrics = new QAction(this);
    m_fileSaveLyrics->setText(i18nc("@action:inmenu","Save Lyrics..."));
    m_fileSaveLyrics->setIcon(QIcon::fromTheme(QStringLiteral("document-save-as")));
    connect(m_fileSaveLyrics, SIGNAL(triggered()), SLOT(fileSaveLyrics()));
    actionCollection()->addAction("file_save_lyrics", m_fileSaveLyrics);

    m_play = new QAction(this);
    m_play->setText(i18nc("@action player play", "Play") );
    m_play->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-start")));
    m_play->setShortcut( Qt::Key_MediaPlay );
    m_play->setWhatsThis(i18nc("@info:whatsthis","Start playback of the current session"));
    m_play->setEnabled(false);
    connect(m_play, SIGNAL(triggered()), SLOT(play()));
    actionCollection()->addAction("play", m_play);

    m_pause = new KToggleAction(this);
    m_pause->setText(i18nc("@action player pause", "Pause") );
    m_pause->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-pause")));
    m_pause->setWhatsThis(i18nc("@info:whatsthis","Pause the playback"));
    m_pause->setEnabled(false);
    connect(m_pause, SIGNAL(triggered()), SLOT(pause()));
    actionCollection()->addAction("pause", m_pause);

    m_next = new QAction(this);
    m_next->setText(i18nc("@action player next song", "Next song") );
    m_next->setIcon(QIcon::fromTheme(QStringLiteral("media-skip-forward")));
    m_next->setShortcut( Qt::Key_MediaNext );
    m_next->setWhatsThis(i18nc("@info:whatsthis","Next song"));
    m_next->setEnabled(false);
    connect(m_next, SIGNAL(triggered()), SLOT(next()));
    actionCollection()->addAction("next", m_next);

    m_previous = new QAction(this);
    m_previous->setText(i18nc("@action player previous song", "Previous song") );
    m_previous->setIcon(QIcon::fromTheme(QStringLiteral("media-skip-backward")));
    m_previous->setShortcut( Qt::Key_MediaPrevious );
    m_previous->setWhatsThis(i18nc("@info:whatsthis","Previous song"));
    m_previous->setEnabled(false);
    connect(m_previous, SIGNAL(triggered()), SLOT(previous()));
    actionCollection()->addAction("previous", m_previous);

    m_forward = new QAction(this);
    m_forward->setText(i18nc("@action player skip forward", "Forward") );
    m_forward->setIcon(QIcon::fromTheme(QStringLiteral("media-seek-forward")));
    m_forward->setWhatsThis(i18nc("@info:whatsthis", "Player seek forward"));
    m_forward->setEnabled(false);
    connect(m_forward, SIGNAL(triggered()), SLOT(forward()));
    actionCollection()->addAction("forward", m_forward);

    m_rewind = new QAction(this);
    m_rewind->setText(i18nc("@action player skip backward", "Rewind") );
    m_rewind->setIcon(QIcon::fromTheme(QStringLiteral("media-seek-backward")));
    m_rewind->setWhatsThis(i18nc("@info:whatsthis", "Player seek backward"));
    m_rewind->setEnabled(false);
    connect(m_rewind, SIGNAL(triggered()), SLOT(rewind()));
    actionCollection()->addAction("backward", m_rewind);

    m_stop = new QAction(this);
    m_stop->setText(i18nc("@action player stop", "Stop") );
    m_stop->setIcon(QIcon::fromTheme(QStringLiteral("media-playback-stop")));
    m_stop->setShortcut( Qt::Key_MediaStop );
    m_stop->setWhatsThis(i18nc("@info:whatsthis","Stop playback or recording") );
    m_stop->setEnabled(false);
    connect(m_stop, SIGNAL(triggered()), SLOT(stop()));
    actionCollection()->addAction("stop", m_stop);

    m_fileInfo = new QAction(this);
    m_fileInfo->setText(i18nc("@action:inmenu","File info..."));
    m_fileInfo->setIcon(QIcon::fromTheme(QStringLiteral("document-properties")));
    connect(m_fileInfo, SIGNAL(triggered()), SLOT(slotFileInfo()));
    actionCollection()->addAction("file_info", m_fileInfo);

    m_loop = new KToggleAction(this);
    m_loop->setText(i18nc("@action playlist repeat", "Repeat") );
    m_loop->setIcon(QIcon::fromTheme(QStringLiteral("media-playlist-repeat")));
    m_loop->setChecked(m_settings->player_loop());
    actionCollection()->addAction("loop", m_loop);

    m_shuffle = new KToggleAction(this);
    m_shuffle->setText(i18nc("@action playlist shuffle", "Shuffle") );
    m_shuffle->setIcon(QIcon::fromTheme(QStringLiteral("media-playlist-shuffle")));
    m_shuffle->setChecked(m_settings->playlist_shuffle());
    connect(m_shuffle, SIGNAL(toggled(bool)), SLOT(slotShuffle(bool)));
    actionCollection()->addAction("shuffle", m_shuffle);

    m_autostart = new KToggleAction(this);
    m_autostart->setText(i18nc("@action player auto-start", "Auto-Start") );
    m_autostart->setChecked(m_settings->player_autostart());
    actionCollection()->addAction("autostart", m_autostart);

    m_playListAuto = new KToggleAction(this);
    m_playListAuto->setText(i18nc("@action", "Auto-Add to Playlist") );
    actionCollection()->addAction("autoadd_playlist", m_playListAuto);

    m_playListLoad = new QAction(this);
    m_playListLoad->setText(i18nc("@action","Load Playlist...") );
    m_playListLoad->setIcon(QIcon::fromTheme(QStringLiteral("document-open")));
    connect(m_playListLoad, SIGNAL(triggered()), SLOT(slotLoadPlaylist()));
    actionCollection()->addAction("load_playlist", m_playListLoad);

    m_playListSave = new QAction(this);
    m_playListSave->setText(i18nc("@action","Save Playlist...") );
    m_playListSave->setIcon(QIcon::fromTheme(QStringLiteral("document-save")));
    connect(m_playListSave, SIGNAL(triggered()), SLOT(slotSavePlaylist()));
    actionCollection()->addAction("save_playlist", m_playListSave);

    m_playListEdit = new QAction(this);
    m_playListEdit->setText(i18nc("@action","Edit Playlist..." ));
    m_playListEdit->setIcon(QIcon::fromTheme(QStringLiteral("view-media-playlist")));
    connect(m_playListEdit, SIGNAL(triggered()), SLOT(slotManagePlaylist()));
    actionCollection()->addAction("edit_playlist", m_playListEdit);

    m_showPianola = new KToggleAction(this);
    m_showPianola->setText(i18nc("@action Pianola","Player Piano") );
    m_showPianola->setWhatsThis(i18nc("@info:whatsthis Pianola","Show or hide the player piano window"));
    connect(m_showPianola, SIGNAL(toggled(bool)), SLOT(slotShowPianola(bool)));
    actionCollection()->addAction("show_pianola", m_showPianola);

    m_showChannels = new KToggleAction(this);
    m_showChannels->setText(i18nc("@action","Channels") );
    m_showChannels->setWhatsThis(i18nc("@info:whatsthis","Show or hide the channels window"));
    connect(m_showChannels, SIGNAL(toggled(bool)), SLOT(slotShowChannels(bool)));
    actionCollection()->addAction("show_channels", m_showChannels);

    m_loadSongSettings = new QAction(this);
    m_loadSongSettings->setText(i18nc("@action load song settings","Load"));
    m_loadSongSettings->setIcon(QIcon::fromTheme(QStringLiteral("document-open")));
    m_loadSongSettings->setWhatsThis(i18nc("@info:whatsthis", "Load the stored settings for the current song"));
    connect(m_loadSongSettings, SIGNAL(triggered()), SLOT(slotLoadSongSettings()));
    actionCollection()->addAction("load_songsettings", m_loadSongSettings);

    m_saveSongSettings = new QAction(this);
    m_saveSongSettings->setText(i18nc("@action save song settings","Save"));
    m_saveSongSettings->setIcon(QIcon::fromTheme(QStringLiteral("document-save")));
    m_saveSongSettings->setWhatsThis(i18nc("@info:whatsthis", "Save the settings of the current song"));
    connect(m_saveSongSettings, SIGNAL(triggered()), SLOT(slotSaveSongSettings()));
    actionCollection()->addAction("save_songsettings", m_saveSongSettings);

    m_autoSongSettings = new KToggleAction(this);
    m_autoSongSettings->setText(i18nc("@action auto load/save song settings","Automatic"));
    m_autoSongSettings->setWhatsThis(i18nc("@info:whatsthis","Automatically load and save song settings"));
    m_autoSongSettings->setChecked(m_settings->auto_song_settings());
    actionCollection()->addAction("auto_songsettings", m_autoSongSettings);
}

void KMid2::fileOpen()
{
    QList<QUrl> urls = QFileDialog::getOpenFileUrls(
            this, i18nc("@title:window","Open MIDI/Karaoke files"),
            QUrl(), QStringLiteral("MIDI files (*.mid *.midi *.kar);;All files (*)"));
    setPlayList(urls);
}

void KMid2::setPlayList(const QList<QUrl> &urls)
{
    QList<QUrl> list;
    if (!urls.empty() && (m_midiobj != nullptr)) {
        stop();
        if (m_autoSongSettings->isChecked())
            slotSaveSongSettings();
        if (m_playListAuto->isChecked())
            for (const QString& s : m_midiobj->queue())
                list += QUrl(s);
        for (const QUrl& u : urls)
            list += u;
        if (m_shuffle->isChecked())
            std::random_shuffle(list.begin(), list.end());
        m_midiobj->setQueue(list);
        m_midiobj->setCurrentSource(urls.first().toLocalFile());
        for (const QUrl& u : urls)
            m_recentFiles->addUrl(u);
    }
}

void KMid2::setUrlsLater(const QList<QUrl> &urls)
{
    m_pendingList = urls;
}

void KMid2::dragEnterEvent( QDragEnterEvent * event )
{
    if (event->mimeData()->hasUrls()) {
        event->acceptProposedAction();
    }
}

void KMid2::dropEvent( QDropEvent * event )
{
    if (event->mimeData()->hasUrls() && (m_midiobj != nullptr)) {
        QList<QUrl> urls = event->mimeData()->urls();
        if (!urls.empty()) {
            setPlayList(urls);
        }
        event->accept();
    }
}

void KMid2::slotSourceChanged(const QString &src)
{
    QFileInfo finfo(src);
    m_songEncoding.clear();
    m_songName = finfo.fileName();
    QStringList q = m_midiobj->queue();
    m_previous->setEnabled(q.indexOf(src) > 0);
    m_next->setEnabled(q.indexOf(src) < (q.count() - 1));
    m_timeSlider->setRange(0, m_midiobj->totalTime());
    m_timeSlider->setTickInterval(m_midiobj->totalTime() / 8);
    m_timeSlider->setSliderPosition(0);
    m_timeLabel->setMarkers(8);
    m_timeLabel->setTotalTime(m_midiobj->duration() * 1000);
    m_midiobj->seek(0);
    displayBeat(1,1);
    updateTempoLabel();
    m_seekamt = m_midiobj->totalTime() / 10;
    m_seeking = false;
    if (m_pianola != nullptr) {
        int loNote = m_midiobj->lowestMidiNote();
        int hiNote = m_midiobj->highestMidiNote();
        m_pianola->setNoteRange(loNote, hiNote);
        for(int i = 0; i < MIDI_CHANNELS; ++i ) {
            m_pianola->enableChannel(i, m_midiobj->channelUsed(i));
            m_pianola->slotLabel(i, m_midiobj->channelLabel(i));
        }
    }
    if (m_channels != nullptr)
        for(int i = 0; i < MIDI_CHANNELS; ++i ) {
            m_midiout->setLocked(i, false);
            m_midiout->setMuted(i, false);
            m_channels->setLockChannel(i, false);
            m_channels->enableChannel(i, m_midiobj->channelUsed(i));
            m_channels->setChannelName(i, m_midiobj->channelLabel(i));
        }
    if (m_autoSongSettings->isChecked())
        slotLoadSongSettings();
    if (m_autostart->isChecked())
        play();
    else {
        slotUpdateState(m_midiobj->state(), ErrorState);
        displayLyrics();
    }
}

void KMid2::displayLyrics()
{
    qint64 time = m_midiobj->currentTime();
    if ( m_songEncoding.isEmpty() &&
         m_midiobj->guessTextEncoding() ) {
        QByteArray encoding = m_midiobj->getTextEncoding().toLatin1();
        // Find matching codec name in available codecs
        const auto codecs = QTextCodec::availableCodecs();
        for (const QByteArray &codecName : codecs) {
            QTextCodec* codec = QTextCodec::codecForName(codecName);
            if (codec != nullptr && encoding == codec->name()) {
                slotSelectEncoding(QString::fromLatin1(codecName));
                break;
            }
        }
    }
    QString s = m_midiobj->metaData("SMF_LYRICS").join("");
    if (s.isEmpty()) s = m_midiobj->metaData("SMF_TEXT").join("");
    m_lyricsText->clear();
    m_fileSaveLyrics->setEnabled(!s.isEmpty());
    m_print->setEnabled(!s.isEmpty());
    m_comboCodecs->setEnabled(!s.isEmpty());
    if (s.isEmpty())
        return;
    m_lyricsText->setFont(m_settings->font());
    m_lyricsText->setPlainText(s);
    if (time != 0) {
        m_lyricsText->moveCursor(QTextCursor::Start);
        s = m_midiobj->getLyrics(time).join("");
        if (m_lyricsText->find(s, QTextDocument::FindCaseSensitively)) {
            QTextCharFormat fmt;
            fmt.setForeground(m_settings->color());
            m_lyricsText->textCursor().mergeCharFormat(fmt);
        }
        QTextCursor csr = m_lyricsText->textCursor();
        csr.clearSelection();
        m_lyricsText->setTextCursor(csr);
    }
}

void KMid2::slotUpdateState( State newState, State /*oldState*/ )
{
    if (newState != PausedState) m_pause->setChecked(false);
    switch(newState) {
    case PlayingState:
        updateState("playing_state", i18nc("@info:status player playing", "playing"));
        break;
    case PausedState:
        updateState("paused_state", i18nc("@info:status player paused", "paused"));
        break;
    case StoppedState:
        updateState("stopped_state", i18nc("@info:status player stopped", "stopped"));
        break;
    case BufferingState:
    case LoadingState:
        updateState("disabled_state", i18nc("@info:status player loading", "loading"));
        break;
    case ErrorState:
        updateState("disabled_state", i18nc("@info:status player invalid status", "not ready"));
        break;
    default:
        break;
    }
    emit playerStateChanged(newState);
}

void KMid2::updateState(const QString &newState, const QString &stateName)
{
    setCaption(i18nc("@info:status", "%1 [%2]", m_songName, stateName));
    stateChanged(newState);
}

void KMid2::play()
{
    if (m_midiobj != nullptr) {
        slotCheckOutput();
        displayLyrics();
        m_midiobj->play();
        m_rhythm->beat(1);
    }
}

void KMid2::pause()
{
    if (m_midiobj != nullptr) {
        if (m_midiobj->state() == PlayingState) {
            m_midiobj->pause();
            m_pianola->allNotesOff();
            m_channels->allNotesOff();
        }
        else
            m_midiobj->play();
    }
}

void KMid2::stop()
{
    if (m_midiobj != nullptr)
        m_midiobj->stop();
    if (m_pianola != nullptr)
        m_pianola->allNotesOff();
    if (m_channels != nullptr)
        m_channels->allNotesOff();
    m_rtempo = 0;
}

void KMid2::forward()
{
    if (m_midiobj != nullptr) {
        qint64 s = m_midiobj->currentTime() + m_seekamt;
        if (s < m_midiobj->totalTime()) {
            m_timeSlider->setValue(s);
        }
    }
}

void KMid2::rewind()
{
    if (m_midiobj != nullptr) {
        qint64 s = m_midiobj->currentTime() - m_seekamt;
        if (s > 0) {
            m_timeSlider->setValue(s);
        }
    }
}

void KMid2::finished()
{
    if (m_autoSongSettings->isChecked())
        slotSaveSongSettings();
    m_timeSlider->setValue(0);
    if ((m_midiobj != nullptr) && m_loop->isChecked()) {
        QStringList queue = m_midiobj->queue();
        if ((queue.count() > 0) &&
            (queue.indexOf(m_midiobj->currentSource()) == queue.count()-1))
            m_midiobj->setCurrentSource(queue.at(0));
    }
}

void KMid2::slotSelectEncoding(int i)
{
    if (i <= 0)
        m_songEncoding.clear();
    else {
        // Get codec name from combo item text
        QString itemText = m_comboCodecs->itemText(i);
        // Try to find codec by descriptive name
        QTextCodec *codec = QTextCodec::codecForName(itemText.toLatin1());
        if (codec)
            m_songEncoding = QString::fromLatin1(codec->name());
        else
            m_songEncoding = itemText;
    }
    if (m_midiobj != nullptr)
        m_midiobj->setTextEncoding(m_songEncoding);
    displayLyrics();
}

void KMid2::slotSelectEncoding(const QString& encoding)
{
    if (encoding.isEmpty()) {
        m_songEncoding.clear();
        m_comboCodecs->setCurrentIndex(0);
    } else {
        int idx = m_comboCodecs->findText(encoding, Qt::MatchContains);
        if (idx >= 0)
            m_comboCodecs->setCurrentIndex(idx);
        m_songEncoding = encoding;
    }
    if (m_midiobj != nullptr)
        m_midiobj->setTextEncoding(m_songEncoding);
}

void KMid2::slotMidiTextEvent(const int type, const QString &txt)
{
    if ( (type == 5) &&
         (m_lyricsText->find(txt.trimmed(), QTextDocument::FindCaseSensitively)) ) {
        QTextCharFormat fmt;
        fmt.setForeground(m_settings->color());
        m_lyricsText->textCursor().mergeCharFormat(fmt);
        QRect r = m_lyricsText->cursorRect();
        QScrollBar *s = m_lyricsText->verticalScrollBar();
        int half = m_lyricsText->viewport()->height() / 2;
        int newpos = s->value() + r.top() - half;
        if ((r.top() > half) && (newpos < s->maximum()))
            s->setValue(newpos);
    }
}

void KMid2::updateTempoLabel()
{
    qreal rtempo = m_midiobj->currentTempo();
    if (rtempo != m_rtempo) {
        m_rtempo = rtempo;
        m_labelTempo->setText(QString("%1 bpm").arg(rtempo, 5, 'f', 1));
    }
}

void KMid2::slotTempoReset()
{
    m_midiobj->setTimeSkew(1.0);
    m_tempoSlider->setValue(100);
    m_tempoSlider->setToolTip("100%");
}

inline double sliderToTempoFactor(int sliderValue)
{
    return (sliderValue*sliderValue + 100.0*sliderValue + 20000.0) / 400.0;
}

void KMid2::slotTempoSlider(int value)
{
    m_midiobj->setTimeSkew( sliderToTempoFactor(value)/100.0 );
    if (m_midiobj->state() != PlayingState)
        updateTempoLabel();
    slotTempoSliderMoved(value);
}

void KMid2::slotTempoSliderMoved(int value)
{
    QString tip = QString::number(sliderToTempoFactor(value), 'f', 0) + '%';
    m_tempoSlider->setToolTip(tip);
    QToolTip::showText(QCursor::pos(), tip, this);
}

void KMid2::slotTimeSlider(int value)
{
    m_midiobj->seek(value);
    m_pianola->allNotesOff();
    m_channels->allNotesOff();
    displayBeat(0,0);
    displayLyrics();
    m_seeking = false;
}

void KMid2::slotTimeSliderPressed()
{
    m_seeking = true;
}

void KMid2::slotVolumeSlider(int value)
{
    m_midiout->setVolume( -1, value*0.01 );
    if (m_channels != nullptr)
        m_channels->setVolumeFactor( value*0.01 );
    slotVolumeSliderMoved(value);
}

void KMid2::slotVolumeSliderMoved(int value)
{
    QString tip = QString::number(value*1.0, 'f', 0) + '%';
    m_volumeSlider->setToolTip(tip);
    QToolTip::showText(QCursor::pos(), tip, this);
}

void KMid2::slotPitchSlider(int value)
{
    m_midiout->setPitchShift( value );
    slotPitchSliderMoved(value);
}

void KMid2::slotPitchSliderMoved(int value)
{
    QString tip = QString::number(value);
    m_pitchSlider->setToolTip(tip);
    QToolTip::showText(QCursor::pos(), tip, this);
}

void KMid2::slotTick(qint64 time)
{
    if (!m_seeking)
        m_timeSlider->setSliderPosition(time);
    emit tick(time);
}

void KMid2::slotTempoChanged(qreal tempo)
{
    emit tempoEvent(tempo);
    updateTempoLabel();
}

void KMid2::slotEditSettings()
{
    if (KConfigDialog::exists("settings")) {
        slotUpdateConfigDialogWidgets();
        KConfigDialog::showDialog("settings");
        return;
    }
    KConfigDialog *dialog = new KConfigDialog( this, "settings", m_settings );
    QWidget *lyricsSettings = new QWidget(dialog);
    QWidget *midiSettings = new QWidget(dialog);
    ui_prefs_lyrics.setupUi(lyricsSettings);
    ui_prefs_midi.setupUi(midiSettings);
    connect(ui_prefs_midi.connList, &QListWidget::currentItemChanged,
            this, [this](QListWidgetItem *cur, QListWidgetItem *) {
        if (cur != nullptr)
            ui_prefs_midi.kcfg_output_connection->setText(cur->text());
    });
    ui_prefs_midi.kcfg_midi_backend->setVisible(false);
    for (const MidiBackend& backend : m_backends) {
        ui_prefs_midi.backendsCombo->addItem(backend.name);
        if (backend.library == m_settings->midi_backend())
            ui_prefs_midi.backendsCombo->setCurrentText(backend.name);
    }
    ui_prefs_midi.backendsCombo->setEnabled( m_backends.size() > 1 );
    ui_prefs_midi.kcfg_output_connection->setVisible(false);
    // Find MIDI map files
    QStringList maps = QStandardPaths::locateAll(QStandardPaths::AppDataLocation, "*.map");
    ui_prefs_midi.mapsCombo->addItems(maps);
    ui_prefs_midi.mapsCombo->insertItem(0, QString());
    ui_prefs_midi.mapsCombo->setCurrentText(m_settings->midi_mapper());
    ui_prefs_midi.kcfg_midi_mapper->setVisible(false);
    ui_prefs_midi.kcfg_sysex_file->setEnabled(m_settings->reset_mode() == Settings::Syx);
    dialog->addPage(lyricsSettings, i18nc("@title:group","Lyrics"), "format-text-color");
    dialog->addPage(midiSettings, i18nc("@title:group","MIDI"), "audio-midi");
    if ( m_currentBackend != nullptr && m_currentBackend->hasSoftSynths()) {
        QWidget *progsSettings = new QWidget(dialog);
        m_currentBackend->setupConfigurationWidget(progsSettings);
        dialog->addPage(progsSettings, i18nc("@title:group","Soft Synths"), "applications-system");
    }
    connect(dialog, SIGNAL(settingsChanged(const QString&)),
            SLOT(slotApplySettings(const QString&)));
    connect(ui_prefs_midi.kcfg_reset_mode, SIGNAL(activated(int)),
            SLOT(slotModeChanged(int)));
    connect(ui_prefs_midi.backendsCombo, SIGNAL(activated(int)),
            SLOT(slotBackendChanged(int)));
    slotUpdateConfigDialogWidgets();
    dialog->show();
}

void KMid2::slotApplySettings(const QString& name)
{
    Q_UNUSED(name);
    QApplication::setOverrideCursor(QCursor(Qt::WaitCursor));
    if ( m_currentBackend != nullptr &&
         ( !m_currentBackend->hasSoftSynths() ||
           !m_currentBackend->applySoftSynthSettings() ))
        connectMidiOutput();
    else
        slotUpdateConfigDialogWidgets();
    QApplication::restoreOverrideCursor();
}

void KMid2::slotModeChanged(int mode)
{
    ui_prefs_midi.kcfg_sysex_file->setEnabled(mode == Settings::Syx);
}

void KMid2::slotBackendChanged(int index)
{
    QString library = m_backends.at(index).library;
    if ( library != m_currentBackendLibrary ) {
        ui_prefs_midi.kcfg_midi_backend->setText(library);
        KMessageBox::information( this,
            i18nc( "@info", "The new MIDI backend "
                   "will take effect after restarting the program." ),
            i18nc( "@title:window", "MIDI Backend Changed" ) );
    }
}

void KMid2::slotUpdateConfigDialogWidgets()
{
    QStringList lst = m_midiout->outputDeviceList(!m_settings->advanced_ports());
    ui_prefs_midi.connList->clear();
    ui_prefs_midi.connList->addItems(lst);
    ui_prefs_midi.connList->setCurrentRow(m_midiout->outputDevice());
    if ( m_currentBackend != nullptr && m_currentBackend->hasSoftSynths())
        m_currentBackend->updateConfigWidget();
}

void KMid2::slotReadSettings()
{
    if ( m_currentBackend != nullptr && m_currentBackend->hasSoftSynths())
        m_currentBackend->saveSettings();

    KConfigGroup config = KSharedConfig::openConfig()->group("RecentFiles");
    m_recentFiles->loadEntries(config);

    QString mapperFile = m_settings->midi_mapper();
    if (mapperFile.isEmpty())
        m_mapper.clear();
    else {
        m_mapper.loadFile(mapperFile);
        if (m_midiout != nullptr) m_midiout->setMidiMap(&m_mapper);
    }
    switch(m_settings->reset_mode()) {
    case Settings::None:
        m_resetMessage.clear();
        break;
    case Settings::GM:
        m_resetMessage = QByteArray::fromRawData(
                reinterpret_cast<const char *>(gmreset), sizeof(gmreset));
        break;
    case Settings::GS:
        m_resetMessage = QByteArray::fromRawData(
                reinterpret_cast<const char *>(gsreset), sizeof(gsreset));
        break;
    case Settings::XG:
        m_resetMessage = QByteArray::fromRawData(
                reinterpret_cast<const char *>(xgreset), sizeof(xgreset));
        break;
    case Settings::Syx: {
        QFile file(m_settings->sysex_file().toLocalFile());
        file.open(QIODevice::ReadOnly);
        m_resetMessage = file.readAll();
        file.close();
        break;
    }
    }
    if (m_midiout != nullptr)
        m_midiout->setResetMessage(m_resetMessage);
    m_autoSongSettings->setChecked(m_settings->auto_song_settings());
    slotSelectEncoding(m_settings->encoding());
    displayLyrics();
}

void KMid2::slotWriteSettings()
{
    if (m_mapper.isOK())
        m_settings->setMidi_mapper(m_mapper.fileName());
    m_settings->setEncoding(m_comboCodecs->currentText());
    m_settings->setPlayer_loop(m_loop->isChecked());
    m_settings->setPlaylist_shuffle(m_shuffle->isChecked());
    m_settings->setPlayer_autostart(m_autostart->isChecked());
    m_settings->setAuto_song_settings(m_autoSongSettings->isChecked());
    if (m_settings->reset_mode() != Settings::Syx)
        m_settings->setSysex_file(QUrl());
    m_settings->save();
    KConfigGroup config = KSharedConfig::openConfig()->group("RecentFiles");
    m_recentFiles->saveEntries(config);
    config.sync();
}

bool KMid2::queryClose()
{
    stop();
    if (m_midiout != nullptr) {
        m_midiout->allNotesOff();
        m_midiout->resetControllers();
    }
    if (m_autoSongSettings->isChecked())
        slotSaveSongSettings();
    if ( m_currentBackend != nullptr &&
         m_currentBackend->hasSoftSynths() )
        m_currentBackend->terminateSoftSynths();
    slotWriteSettings();
    return true;
}

void KMid2::slotFileInfo()
{
    QString infostr;
    if (m_midiobj->currentSource().isEmpty())
        infostr = i18nc("@info","No file loaded");
    else {
        infostr = i18nc("@info","File: %1\n", m_songName);
        QString s = m_midiobj->metaData("SMF_COPYRIGHT").join("\n");
        if (!s.isEmpty())
            infostr += i18nc("@info","Copyright: %1\n", s);
        s = m_midiobj->metaData("KAR_FILETYPE").join("\n");
        if (!s.isEmpty())
            infostr += i18nc("@info","Karaoke type: %1\n", s);
        s = m_midiobj->metaData("KAR_VERSION").join("\n");
        if (!s.isEmpty())
            infostr += i18nc("@info","Karaoke version: %1\n", s);
        s = m_midiobj->metaData("KAR_INFORMATION").join("\n");
        if (!s.isEmpty())
            infostr += i18nc("@info","Karaoke info: %1\n", s);
        s = m_midiobj->metaData("KAR_LANGUAGE").join("\n");
        if (!s.isEmpty())
            infostr += i18nc("@info","Karaoke language: %1\n", s);
        s = m_midiobj->metaData("KAR_TITLES").join("\n");
        if (!s.isEmpty())
            infostr += i18nc("@info","Karaoke title: %1\n", s);
        s = m_midiobj->metaData("KAR_WARNINGS").join("\n");
        if (!s.isEmpty())
            infostr += i18nc("@info","Karaoke warnings: %1\n", s);
    }
    KMessageBox::information(this, infostr, i18nc("@title:window","Sequence Information"));
}

void KMid2::fileSaveLyrics()
{
    QString lyrics = m_lyricsText->toPlainText();
    if (!lyrics.isEmpty()) {
        QString fileName = QFileDialog::getSaveFileName(
                this, i18nc("@title:window","Save lyrics to file"),
                QString(), QStringLiteral("Text files (*.txt);;All files (*)"));
        if (!fileName.isEmpty()) {
            QSaveFile file(fileName);
            file.open(QIODevice::WriteOnly);
            file.write(lyrics.toLocal8Bit());
            file.commit();
        }
    }
}

void KMid2::filePrint()
{
    QPrinter printer;
    QPointer<QPrintDialog> printDialog = new QPrintDialog(&printer, this);
    if (printDialog->exec() == QDialog::Accepted)
        m_lyricsText->print(&printer);
    delete printDialog;
}

void KMid2::previous()
{
    QStringList queue = m_midiobj->queue();
    int i = queue.indexOf(m_midiobj->currentSource());
    if (i > 0) {
        stop();
        if (m_autoSongSettings->isChecked())
            slotSaveSongSettings();
        m_midiobj->setCurrentSource(queue.at(i-1));
    }
}

void KMid2::next()
{
    QStringList queue = m_midiobj->queue();
    int i = queue.indexOf(m_midiobj->currentSource());
    if (i < queue.count()-1) {
        stop();
        if (m_autoSongSettings->isChecked())
            slotSaveSongSettings();
        m_midiobj->setCurrentSource(queue.at(i+1));
    }
}

QString KMid2::chooseInitialConnection(const QStringList& items)
{
    bool ok;
    QString result = QInputDialog::getItem(this,
        i18nc("@title:window", "MIDI Player connection"),
        i18nc("@info","Please select one MIDI port:"),
        items, 0, false, &ok);
    if (!ok) result.clear();
    return result;
}

void KMid2::slotCheckOutput()
{
    if (!m_connected || (m_midiout->outputDevice() < 0)) {
        QString item;
        QStringList items = m_midiout->outputDeviceList(!m_settings->advanced_ports());
        if (items.isEmpty()) {
            KMessageBox::sorry(this,
                    i18nc("@info", "There are no available MIDI ports in your system. "
                    "You can specify a software synthesizer, such as TiMidity++ or FluidSynth, "
                    "to be launched automatically when KMid starts. "
                    "This option is located in the dialog Settings->Configure KMid."),
                    i18nc("@title:window", "MIDI Player connection"));
        } else {
            if ((items.count() == 1) && !m_settings->advanced_ports())
                item = items.first();
            else
                item = chooseInitialConnection(items);
            if (!item.isEmpty()) {
                m_connected = m_midiout->setOutputDeviceName(item);
                if (m_connected)
                    m_settings->setOutput_connection(item);
            }
        }
    }
}

void KMid2::loadPlaylist(const QString &fileName)
{
    QStringList list;
    QFile file(fileName);
    if (!file.open(QIODevice::ReadOnly | QIODevice::Text))
        return;
    QTextStream in(&file);
    while (!in.atEnd())
        list << in.readLine();
    if (!list.empty()) {
        QList<QUrl> urls;
        for (const QString &s : list)
            urls.append(QUrl::fromLocalFile(s));
        setPlayList(urls);
        m_playList = fileName;
    }
}

void KMid2::slotLoadPlaylist()
{
    QString fileName = QFileDialog::getOpenFileName(
            this, i18nc("@title:window","Open playlist from file"),
            QString(), QStringLiteral("Text files (*.txt);;All files (*)"));
    if (!fileName.isEmpty() && (m_midiobj != nullptr))
        loadPlaylist(fileName);
}

void KMid2::slotSavePlaylist()
{
    QString fileName = QFileDialog::getSaveFileName(
            this, i18nc("@title:window","Save playlist to file"),
            QString(), QStringLiteral("Text files (*.txt);;All files (*)"));
    if ( !fileName.isEmpty() && (m_midiobj != nullptr) ) {
        QSaveFile file(fileName);
        file.open(QIODevice::WriteOnly);
        QTextStream stream(&file);
        for (const QString& line : m_midiobj->queue())
            stream << line << "\n";
        stream.flush();
        file.commit();
        m_playList = fileName;
    }
}

void KMid2::slotManagePlaylist()
{
    QPointer<PlayListDialog> dialog = new PlayListDialog(this);
    dialog->setItems( m_midiobj->queue() );
    dialog->setCurrentItem( m_midiobj->currentSource() );
    if (dialog->exec() == QDialog::Accepted) {
        if (dialog != nullptr) {
            stop();
            if (dialog->itemCount() > 0) {
                m_midiobj->setQueue(dialog->items());
                QString selected = dialog->currentItem();
                if (selected.isEmpty())
                    selected = dialog->items().first();
                m_midiobj->setCurrentSource(selected);
            } else
                m_midiobj->clearQueue();
        }
    }
    delete dialog;
}

void KMid2::slotShuffle(bool checked)
{
    if (checked) {
        stop();
        QStringList list = m_midiobj->queue();
        if (!list.isEmpty()) {
            std::random_shuffle(list.begin(), list.end());
            m_midiobj->setQueue(list);
            m_midiobj->setCurrentSource(list.first());
        }
    }
}

void KMid2::displayBeat(const int bars, const int beats)
{
    m_labelBeatCount->setText(QString("%1:%2")
            .arg(bars, 5,10,QLatin1Char(' '))
            .arg(beats,2,10,QLatin1Char('0')));
}

void KMid2::slotBeat(const int bars, const int beats, const int maxbeats)
{
    m_rhythm->setRhythm( maxbeats );
    m_rhythm->beat( beats );
    displayBeat(bars, beats);
}

void KMid2::slotTimeSignatureEvent(const int numerator, const int denominator)
{
    m_rhythm->setRhythm( numerator );
    Q_UNUSED(denominator)
}

void KMid2::slotShowPianola(bool checked)
{
    if (m_pianola != nullptr)
        m_pianola->setVisible(checked);
}

void KMid2::slotPianolaClosed()
{
    m_showPianola->setChecked(false);
}

void KMid2::slotShowChannels(bool checked)
{
    if (m_channels != nullptr)
        m_channels->setVisible(checked);
}

void KMid2::slotChannelsClosed()
{
    m_showChannels->setChecked(false);
}

void KMid2::setupDockWidgets()
{
    /* Volume and Pitch dock */
    m_volDock = new QDockWidget(i18nc("@title:window", "Volume and Pitch"), this);
    m_volDock->setObjectName(QLatin1String("volumeAndPitchDock"));
    m_volDock->setFeatures(QDockWidget::DockWidgetClosable |
            QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable |
            QDockWidget::DockWidgetVerticalTitleBar);
    QWidget *dw = new QWidget(m_volDock);
    QGridLayout *gridLayout = new QGridLayout(dw);

    QLabel *labelVol = new QLabel(this);
    labelVol->setText(i18nc("@label:slider Sound master volume","Vol."));
    gridLayout->addWidget(labelVol, 0, 0);

    QLabel *labelPitch = new QLabel(this);
    labelPitch->setText(i18nc("@label:slider Musical tone, note height (transpose)","Pitch"));
    gridLayout->addWidget(labelPitch, 0, 1);

    m_volumeSlider = new QSlider(this);
    m_volumeSlider->setMinimum(0);
    m_volumeSlider->setMaximum(200);
    m_volumeSlider->setTickInterval(20);
    m_volumeSlider->setValue(100);
    m_volumeSlider->setOrientation(Qt::Vertical);
    m_volumeSlider->setTickPosition(QSlider::TicksBelow);
    m_volumeSlider->setWhatsThis(i18nc("@info:whatsthis","MIDI master volume control"));
    m_volumeSlider->setTracking(false);
    gridLayout->addWidget(m_volumeSlider, 1, 0);
    labelVol->setBuddy(m_volumeSlider);

    m_pitchSlider = new QSlider(this);
    m_pitchSlider->setMinimum(-12);
    m_pitchSlider->setMaximum(12);
    m_pitchSlider->setTickInterval(1);
    m_pitchSlider->setValue(0);
    m_pitchSlider->setOrientation(Qt::Vertical);
    m_pitchSlider->setTickPosition(QSlider::TicksBelow);
    m_pitchSlider->setWhatsThis(i18nc("@info:whatsthis","Note height transposition"));
    m_pitchSlider->setTracking(false);
    gridLayout->addWidget(m_pitchSlider, 1, 1);
    labelPitch->setBuddy(m_pitchSlider);

    m_volDock->setWidget(dw);
    addDockWidget(Qt::LeftDockWidgetArea, m_volDock);

    QAction* action = m_volDock->toggleViewAction();
    action->setText(i18nc("@title:window", "Volume and Pitch"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("view-media-equalizer")));
    action->setWhatsThis(i18nc("@action:inmenu","Show or hide the volume and pitch controls"));
    actionCollection()->addAction("show_volume_pitch", action);

    connect(m_volDock, SIGNAL(dockLocationChanged(Qt::DockWidgetArea)),
              SLOT(slotDockVolLocationChanged(Qt::DockWidgetArea)));

    /* Time Position dock */
    m_posDock = new QDockWidget(i18nc("@title:window", "Time Position"), this);
    m_posDock->setObjectName(QLatin1String("timeDock"));
    m_posDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    dw = new QWidget(m_posDock);
    QVBoxLayout* vLayout = new QVBoxLayout(dw);

    m_timeSlider = new QSlider(this);
    m_timeSlider->setRange(0, 10000);
    m_timeSlider->setTickInterval(10000 / 8);
    m_timeSlider->setOrientation(Qt::Horizontal);
    m_timeSlider->setTickPosition(QSlider::TicksBelow);
    m_timeSlider->setWhatsThis(i18nc("@info:whatsthis","Time position control"));
    m_timeSlider->setTracking(false);
    vLayout->addWidget(m_timeSlider);

    m_timeLabel = new TimeLabel(this);
    m_timeLabel->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_timeLabel->setTotalTime(10000);
    m_timeLabel->setMarkers(8);
    vLayout->addWidget(m_timeLabel);

    m_posDock->setWidget(dw);
    addDockWidget(Qt::TopDockWidgetArea, m_posDock);

    action = m_posDock->toggleViewAction();
    action->setText(i18nc("@title:window", "Time Position"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("player-time")));
    action->setWhatsThis(i18nc("@action:inmenu","Show or hide the time position controls"));
    actionCollection()->addAction("show_position", action);

    /* Codecs dock */
    m_encDock = new QDockWidget(i18nc("@title:window", "Text Encoding"), this);
    m_encDock->setObjectName(QLatin1String("codecsDock"));

    m_comboCodecs = new QComboBox(this);
    m_comboCodecs->setWhatsThis(i18nc("@info:whatsthis","Character encoding for lyrics and other text"));
    m_comboCodecs->addItem(i18nc("@item:inlistbox Default MIDI text encoding", "Default ( ASCII )"));
    // Add available text codecs
    const auto codecs = QTextCodec::availableCodecs();
    QStringList codecNames;
    for (const QByteArray &name : codecs)
        codecNames << QString::fromLatin1(name);
    codecNames.sort();
    m_comboCodecs->addItems(codecNames);
    int idx = m_comboCodecs->findText(m_settings->encoding(), Qt::MatchContains);
    if (idx >= 0) m_comboCodecs->setCurrentIndex(idx);
    m_encDock->setWidget(m_comboCodecs);
    addDockWidget(Qt::TopDockWidgetArea, m_encDock);

    action = m_encDock->toggleViewAction();
    action->setText(i18nc("@title:window", "Text Encoding"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("view-list-text")));
    action->setWhatsThis(i18nc("@action:inmenu","Show or hide the character encoding control"));
    actionCollection()->addAction("show_codecs", action);

    /* Rhythm dock */
    m_rthmDock = new QDockWidget(i18nc("@title:window", "Rhythm"), this);
    m_rthmDock->setObjectName(QLatin1String("rhythmDock"));
    m_rthmDock->setAllowedAreas(Qt::TopDockWidgetArea | Qt::BottomDockWidgetArea);
    dw = new QWidget(m_rthmDock);
    gridLayout = new QGridLayout(dw);

    QLabel* labelSpeed = new QLabel(this);
    labelSpeed->setText(i18nc("@label","Tempo:"));
    gridLayout->addWidget(labelSpeed, 0, 0);

    m_labelTempo = new QLabel(this);
    m_labelTempo->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_labelTempo->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    m_labelTempo->setTextInteractionFlags(Qt::NoTextInteraction);
    m_labelTempo->setText(i18nc("@label:slider bpm=beats per minute","120.0 bpm"));
    m_labelTempo->setWhatsThis(i18nc("@info:whatsthis","Tempo, or song speed in beats per minute"));
    gridLayout->addWidget(m_labelTempo, 0, 1);

    m_tempoSlider = new QSlider(this);
    m_tempoSlider->setMinimum(0);
    m_tempoSlider->setMaximum(200);
    m_tempoSlider->setTickInterval(20);
    m_tempoSlider->setValue(100);
    m_tempoSlider->setOrientation(Qt::Horizontal);
    m_tempoSlider->setTickPosition(QSlider::TicksAbove);
    m_tempoSlider->setWhatsThis(i18nc("@info:whatsthis","Tempo (song speed) control, between 50% and 200%"));
    m_tempoSlider->setTracking(false);
    gridLayout->addWidget(m_tempoSlider, 0, 2);

    m_btnResetTempo = new QPushButton(this);
    m_btnResetTempo->setText(i18nc("@action:button","100%"));
    m_btnResetTempo->setWhatsThis(i18nc("@info:whatsthis","Reset the song tempo (song speed) to 100%"));
    gridLayout->addWidget(m_btnResetTempo, 0, 3);

    QLabel* labelMetronome = new QLabel(this);
    labelMetronome->setText(i18nc("@label","Metronome:"));
    gridLayout->addWidget(labelMetronome, 1, 0);

    m_labelBeatCount = new QLabel(this);
    m_labelBeatCount->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_labelBeatCount->setText(QLatin1String("1:01"));
    m_labelBeatCount->setAlignment(Qt::AlignRight|Qt::AlignTrailing|Qt::AlignVCenter);
    m_labelBeatCount->setTextInteractionFlags(Qt::NoTextInteraction);
    m_labelBeatCount->setWhatsThis(i18nc("@info:whatsthis","Time position in measures:beats"));
    gridLayout->addWidget(m_labelBeatCount, 1, 1);

    m_rhythm = new RhythmView(this);
    m_rhythm->setFixedHeight(10);
    m_rhythm->setWhatsThis(i18nc("@info:whatsthis","Visual metronome"));
    gridLayout->addWidget(m_rhythm, 1, 2, 1, 1, Qt::AlignVCenter);

    m_rthmDock->setWidget(dw);
    addDockWidget(Qt::BottomDockWidgetArea, m_rthmDock);

    action = m_rthmDock->toggleViewAction();
    action->setText(i18nc("@title:window", "Rhythm"));
    action->setIcon(QIcon::fromTheme(QStringLiteral("chronometer")));
    action->setWhatsThis(i18nc("@action:inmenu","Show or hide the rhythm controls"));
    actionCollection()->addAction("show_rhythm", action);

    /* Central Widget */
    m_lyricsText = new QTextEdit(this);
    m_lyricsText->setReadOnly(true);
    m_lyricsText->setTextInteractionFlags(Qt::NoTextInteraction);
    m_lyricsText->setWhatsThis(i18nc("@info:whatsthis","Song lyrics, or some other text"));
    m_lyricsText->setAutoFillBackground(true);
    setCentralWidget(m_lyricsText);
}

void KMid2::setupPlaces()
{
    // In KF5, KFilePlacesModel is available but we skip auto-adding places
    // as it requires more complex setup. Users can add places manually.
}

void KMid2::slotURLSelected(const QUrl& url)
{
    QList<QUrl> urls;
    urls.append(url);
    setPlayList(urls);
}

void KMid2::slotSaveSongSettings()
{
    if (!m_songName.isEmpty()) {
        QString fileName = QString("songlib/%1.cfg").arg(m_songName);
        QString localPath = QStandardPaths::writableLocation(QStandardPaths::AppDataLocation)
                            + "/" + fileName;
        QDir().mkpath(QFileInfo(localPath).absolutePath());
        KConfig songSettings(localPath);
        KConfigGroup grp = songSettings.group("Global");
        grp.writeEntry("file", m_midiobj->currentSource());
        grp.writeEntry("encoding", m_comboCodecs->currentText());
        grp.writeEntry("volume", m_volumeSlider->value());
        grp.writeEntry("pitch", m_pitchSlider->value());
        grp.writeEntry("timeskew", m_tempoSlider->value());
        for(int i = 0; i < MIDI_CHANNELS; ++i ) {
            if ( m_midiobj->channelUsed(i) ) {
                grp = songSettings.group(QString("MIDI Channel %1").arg(i+1,2));
                grp.writeEntry("name", m_channels->channelName(i));
                grp.writeEntry("muted", m_channels->isChannelMuted(i));
                grp.writeEntry("solo", m_channels->isChannelSoloed(i));
                grp.writeEntry("locked", m_channels->isChannelLocked(i));
                grp.writeEntry("patch", m_channels->channelPatch(i));
            }
        }
        songSettings.sync();
    }
}

void KMid2::slotLoadSongSettings()
{
    int vol, pitch, skew, pgm;
    bool locked, muted, solo;
    if (!m_songName.isEmpty()) {
        QString fileName = QString("songlib/%1.cfg").arg(m_songName);
        QString localPath = QStandardPaths::locate(QStandardPaths::AppDataLocation, fileName);
        if (localPath.isEmpty()) return;
        KConfig songSettings(localPath);
        KConfigGroup grp = songSettings.group("Global");
        QString encoding = grp.readEntry("encoding", QString());
        if (!encoding.isEmpty())
            slotSelectEncoding(encoding);

        vol = grp.readEntry("volume", 100);
        m_volumeSlider->setValue(vol);
        m_volumeSlider->setToolTip(QString::number(vol*1.0, 'f', 0) + '%');

        pitch = grp.readEntry("pitch", 0);
        m_pitchSlider->setValue(pitch);
        m_pitchSlider->setToolTip(QString::number(pitch));

        skew = grp.readEntry("timeskew", 100);
        m_tempoSlider->setValue(skew);
        m_tempoSlider->setToolTip(QString::number(sliderToTempoFactor(skew),'f',0)+'%');

        for(int i = 0; i < MIDI_CHANNELS; ++i ) {
            QString grpName = QString("MIDI Channel %1").arg(i+1,2);
            if ( songSettings.hasGroup(grpName) ) {
                grp = songSettings.group(grpName);
                QString name = grp.readEntry("name", QString());
                if (name.isEmpty())
                    name = m_midiobj->channelLabel(i);
                if (!name.isEmpty()) {
                    m_channels->setChannelName(i, name);
                    m_pianola->slotLabel(i, name);
                }
                muted = grp.readEntry("muted", false);
                m_midiout->setMuted(i, muted);
                m_channels->setMuteChannel(i, muted);
                solo = grp.readEntry("solo", false);
                m_channels->setSoloChannel(i, solo);
                pgm = grp.readEntry("patch", -1);
                m_channels->setPatchChannel(i, pgm);
                if (pgm > -1)
                    m_midiout->sendProgram(i, pgm);
                locked = grp.readEntry("locked", false);
                m_channels->setLockChannel(i, locked);
                m_midiout->setLocked(i, locked);
            }
        }
        displayLyrics();
    }
}

void KMid2::slotSoftSynthErrors(const QString& pgm, const QStringList& messages)
{
    KMessageBox::error(this,
        i18ncp("@info", "Failed to run %2 with the provided arguments. Returned message: %3",
                "Failed to run %2 with the provided arguments. Returned messages: %3",
                messages.size(), pgm, messages.join("\n")),
        i18nc("@title:window", "%1 startup failed", pgm));
}

void KMid2::connectMidiOutput()
{
    m_midiout->outputDeviceList(!m_settings->advanced_ports());
    m_connected = m_midiout->setOutputDeviceName(m_settings->output_connection());
    slotCheckOutput();
    slotReadSettings();
    slotLoadSongSettings();
    if (KConfigDialog::exists("settings"))
        slotUpdateConfigDialogWidgets();
    if (!m_pendingList.isEmpty()) {
        setPlayList(m_pendingList);
        m_pendingList.clear();
    }
}

void KMid2::slotSoftSynthStarted(const QString& pgm, const QStringList& messages)
{
    if (!messages.isEmpty())
        KMessageBox::informationList(this,
            i18ncp("@info", "%2 has returned the following message when launched with the provided arguments.",
                    "%2 has returned the following messages when launched with the provided arguments.",
                    messages.size(), pgm),
            messages,
            i18ncp("@title:window", "%2 message", "%2 messages", messages.size(), pgm),
            "softsynth_warnings");
    connectMidiOutput();
}

void KMid2::readProperties(const KConfigGroup &cfg)
{
    QString currentSource = cfg.readEntry("CurrentSource", QString());
    QString playList = cfg.readEntry("PlayList", QString());
    bool wasPlaying = cfg.readEntry("Playing", false);
    if (!playList.isEmpty() && (m_midiobj != nullptr))
        loadPlaylist(playList);
    if (playList.isEmpty() && !currentSource.isEmpty() && (m_midiobj != nullptr))
        m_midiobj->setCurrentSource(currentSource);
    if (wasPlaying && !m_settings->player_autostart())
        play();
}

void KMid2::saveProperties(KConfigGroup &cfg)
{
    bool playing = (m_midiobj->state() == PlayingState);
    cfg.writeEntry("CurrentSource", m_midiobj->currentSource());
    cfg.writeEntry("PlayList", m_playList);
    cfg.writeEntry("Playing", playing);
}

void KMid2::showEvent(QShowEvent* event)
{
    Q_UNUSED(event);
    if ( !m_pendingList.isEmpty() &&
         !m_settings->exec_fluid() &&
         !m_settings->exec_timidity() ) {
        setPlayList(m_pendingList);
        m_pendingList.clear();
    }
}

void KMid2::slotDockVolLocationChanged(Qt::DockWidgetArea area)
{
    QDockWidget::DockWidgetFeatures flags = QDockWidget::DockWidgetClosable |
        QDockWidget::DockWidgetFloatable | QDockWidget::DockWidgetMovable;
    if (area == Qt::LeftDockWidgetArea || area == Qt::RightDockWidgetArea) {
        flags |= QDockWidget::DockWidgetVerticalTitleBar;
        m_volumeSlider->setOrientation(Qt::Vertical);
        m_volumeSlider->setTickPosition(QSlider::TicksBelow);
        m_pitchSlider->setOrientation(Qt::Vertical);
        m_pitchSlider->setTickPosition(QSlider::TicksBelow);
    } else {
        m_volumeSlider->setOrientation(Qt::Horizontal);
        m_volumeSlider->setTickPosition(QSlider::TicksBelow);
        m_pitchSlider->setOrientation(Qt::Horizontal);
        m_pitchSlider->setTickPosition(QSlider::TicksBelow);
    }
    m_volDock->setFeatures(flags);
}

// DBus property accessors
bool KMid2::autoStart() { return m_autostart->isChecked(); }
void KMid2::setAutoStart(bool enable) { m_autostart->setChecked(enable); }
QString KMid2::midiConnection() {
    if (m_midiout) return m_midiout->outputDeviceName();
    return QString();
}
void KMid2::setMidiConnection(const QString& conn) {
    if (m_midiout) {
        if (m_midiout->setOutputDeviceName(conn))
            m_settings->setOutput_connection(conn);
    }
}
double KMid2::tempoFactor() {
    if (m_midiobj) return m_midiobj->timeSkew();
    return 1.0;
}
void KMid2::setTempoFactor(double factor) {
    if (m_midiobj) m_midiobj->setTimeSkew(factor);
}
double KMid2::volumeFactor() { return m_volumeSlider->value() * 0.01; }
void KMid2::setVolumeFactor(double factor) { m_volumeSlider->setValue(factor * 100); }
int KMid2::transpose() {
    if (m_midiout) return m_midiout->pitchShift();
    return 0;
}
void KMid2::setTranspose(int amount) {
    if (m_midiout) m_midiout->setPitchShift(amount);
}
qlonglong KMid2::length() {
    if (m_midiobj) return m_midiobj->totalTime();
    return 0;
}
bool KMid2::isLooping() { return m_loop->isChecked(); }
void KMid2::setLooping(bool enable) { m_loop->setChecked(enable); }
qlonglong KMid2::position() {
    if (m_midiobj) return m_midiobj->currentTime();
    return 0;
}
int KMid2::state() {
    if (m_midiobj) return m_midiobj->state();
    return ErrorState;
}
QString KMid2::currentSource() const {
    if (m_midiobj) return m_midiobj->currentSource();
    return QString();
}
bool KMid2::openUrl(const QString& url) {
    QList<QUrl> urls;
    urls.append(QUrl::fromUserInput(url));
    setPlayList(urls);
    return true;
}
bool KMid2::isMuted(int channel) {
    if (m_midiout) return m_midiout->isMuted(channel);
    return false;
}
void KMid2::setMuted(int channel, bool muted) {
    if (m_midiout) m_midiout->setMuted(channel, muted);
}
QStringList KMid2::metaData(const QString& key) {
    if (m_midiobj) return m_midiobj->metaData(key);
    return QStringList();
}
QDBusVariant KMid2::songProperty(const QString& key) {
    if (m_midiobj) return QDBusVariant(m_midiobj->songProperty(key));
    return QDBusVariant();
}
QDBusVariant KMid2::channelProperty(int channel, const QString& key) {
    if (m_midiobj) return QDBusVariant(m_midiobj->channelProperty(channel, key));
    return QDBusVariant();
}
void KMid2::seek(qlonglong pos) {
    if (m_midiobj) m_midiobj->seek(pos);
}
void KMid2::reload() {
    if (m_midiobj) {
        QString src = m_midiobj->currentSource();
        if (!src.isEmpty()) {
            stop();
            m_midiobj->setCurrentSource(src);
        }
    }
}
