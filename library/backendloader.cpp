/*
    KMid2 MIDI/Karaoke Player
    Copyright (C) 2009-2010 Pedro Lopez-Cabanillas <plcl@users.sf.net>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License along
    with this program; if not, write to the Free Software Foundation, Inc.,
    51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
*/

#include "backendloader.h"
#include "backend.h"

#include <KPluginFactory>
#include <KPluginLoader>
#include <KPluginMetaData>
#include <QDebug>
#include <QVector>

namespace KMid {

    BackendLoader::BackendLoader(QObject * parent)
      : QObject(parent)
    { }

    BackendLoader::~BackendLoader()
    { }

    void BackendLoader::loadAllBackends()
    {
        // KF5: use KPluginLoader to find plugins by service type
        QVector<KPluginMetaData> offers = KPluginLoader::findPlugins(
            QString(), [](const KPluginMetaData &md) {
                return md.serviceTypes().contains(QLatin1String("KMid/backend"));
            });

        for (const KPluginMetaData &md : offers) {
            KPluginLoader loader(md.fileName());
            KPluginFactory *factory = loader.factory();
            if (!factory) {
                qWarning() << "KPluginFactory could not load the backend:" << md.fileName()
                           << loader.errorString();
                continue;
            }
            Backend *backend = factory->create<Backend>(this, QVariantList());
            if (backend != nullptr)
                emit loaded(backend, md.fileName(), md.name());
        }
    }
}
