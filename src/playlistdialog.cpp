/*
    KMid2 MIDI/Karaoke Player
    Copyright (C) 2009-2010 Pedro Lopez-Cabanillas <plcl@users.sf.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.
*/

#include "playlistdialog.h"
#include <KLocalizedString>

PlayListDialog::PlayListDialog(QWidget *parent) : QDialog(parent)
{
    setWindowTitle(i18nc("@title:window","Manage Playlist"));

    QVBoxLayout *mainLayout = new QVBoxLayout(this);

    QLabel *label = new QLabel(i18nc("@label","Playlist items:"), this);
    mainLayout->addWidget(label);

    m_listWidget = new QListWidget(this);
    mainLayout->addWidget(m_listWidget);

    QHBoxLayout *inputLayout = new QHBoxLayout;
    m_lineEdit = new QLineEdit(this);
    m_lineEdit->setPlaceholderText(i18nc("@info:placeholder","File path or URL"));
    inputLayout->addWidget(m_lineEdit);
    m_addBtn = new QPushButton(i18nc("@action:button","Add"), this);
    inputLayout->addWidget(m_addBtn);
    m_removeBtn = new QPushButton(i18nc("@action:button","Remove"), this);
    inputLayout->addWidget(m_removeBtn);
    mainLayout->addLayout(inputLayout);

    QHBoxLayout *btnLayout = new QHBoxLayout;
    m_okBtn = new QPushButton(i18nc("@action:button","OK"), this);
    m_okBtn->setDefault(true);
    m_cancelBtn = new QPushButton(i18nc("@action:button","Cancel"), this);
    m_resetBtn = new QPushButton(i18nc("@action:button","Reset"), this);
    btnLayout->addWidget(m_resetBtn);
    btnLayout->addStretch();
    btnLayout->addWidget(m_okBtn);
    btnLayout->addWidget(m_cancelBtn);
    mainLayout->addLayout(btnLayout);

    connect(m_addBtn, SIGNAL(clicked()), SLOT(addItem()));
    connect(m_removeBtn, SIGNAL(clicked()), SLOT(removeItem()));
    connect(m_okBtn, SIGNAL(clicked()), SLOT(accept()));
    connect(m_cancelBtn, SIGNAL(clicked()), SLOT(reject()));
    connect(m_resetBtn, SIGNAL(clicked()), SLOT(reset()));
}

PlayListDialog::~PlayListDialog()
{ }

void PlayListDialog::addItem()
{
    QString text = m_lineEdit->text().trimmed();
    if (!text.isEmpty()) {
        m_listWidget->addItem(text);
        m_lineEdit->clear();
    }
}

void PlayListDialog::removeItem()
{
    QListWidgetItem *item = m_listWidget->currentItem();
    if (item) {
        delete m_listWidget->takeItem(m_listWidget->row(item));
    }
}

void PlayListDialog::reset()
{
    m_listWidget->clear();
}

void PlayListDialog::setItems(const QStringList& items)
{
    m_listWidget->clear();
    m_listWidget->addItems(items);
}

QStringList PlayListDialog::items() const
{
    QStringList result;
    for (int i = 0; i < m_listWidget->count(); ++i)
        result << m_listWidget->item(i)->text();
    return result;
}

void PlayListDialog::setCurrentItem(const QString& item)
{
    QList<QListWidgetItem*> found = m_listWidget->findItems(item, Qt::MatchExactly);
    if (!found.isEmpty())
        m_listWidget->setCurrentItem(found.first());
}

QString PlayListDialog::currentItem() const
{
    QListWidgetItem *item = m_listWidget->currentItem();
    if (item) return item->text();
    return QString();
}

int PlayListDialog::itemCount() const
{
    return m_listWidget->count();
}
