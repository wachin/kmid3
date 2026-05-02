/*
    KMid2 MIDI/Karaoke Player
    Copyright (C) 2009-2010 Pedro Lopez-Cabanillas <plcl@users.sf.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#ifndef PLAYLISTDIALOG_H
#define PLAYLISTDIALOG_H

#include <QDialog>
#include <QListWidget>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>

class PlayListDialog : public QDialog {
    Q_OBJECT
public:
    PlayListDialog(QWidget *parent = nullptr);
    virtual ~PlayListDialog();

    void setItems(const QStringList& items);
    QStringList items() const;
    void setCurrentItem(const QString& item);
    QString currentItem() const;
    int itemCount() const;

public slots:
    void reset();

private slots:
    void addItem();
    void removeItem();

private:
    QListWidget *m_listWidget;
    QLineEdit *m_lineEdit;
    QPushButton *m_addBtn;
    QPushButton *m_removeBtn;
    QPushButton *m_okBtn;
    QPushButton *m_cancelBtn;
    QPushButton *m_resetBtn;
};

#endif /* PLAYLISTDIALOG_H */
