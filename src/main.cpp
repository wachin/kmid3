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

#include <config.h>
#include "kmid2.h"

#include <KAboutData>
#include <KLocalizedString>
#include <QApplication>
#include <QCommandLineParser>
#include <QUrl>

static const char description[] = I18N_NOOP("A KDE MIDI/Karaoke player");
static const char version[] = VERSION;

int main(int argc, char **argv)
{
    QApplication app(argc, argv);

    KAboutData about(
        QStringLiteral("kmid"),
        i18n("KMid"),
        QLatin1String(version),
        i18n(description),
        KAboutLicense::GPL,
        i18n("(C) 2009-2010 Pedro Lopez-Cabanillas")
    );
    about.addAuthor(i18n("Pedro Lopez-Cabanillas"), i18n("Maintainer"),
                    QStringLiteral("plcl@users.sf.net"));
    about.addAuthor(i18n("Antonio Larrosa Jimenez"), i18n("Author"),
                    QStringLiteral("larrosa@kde.org"));

    KAboutData::setApplicationData(about);

    QCommandLineParser parser;
    about.setupCommandLine(&parser);
    parser.addPositionalArgument(QStringLiteral("URL"), i18n("Song(s) to open"), QStringLiteral("[URL...]"));
    parser.process(app);
    about.processCommandLine(&parser);

    KMid2 *widget = new KMid2;
    widget->show();

    QStringList args = parser.positionalArguments();
    if (!args.isEmpty()) {
        QList<QUrl> urls;
        for (const QString &arg : args)
            urls.append(QUrl::fromUserInput(arg));
        widget->setUrlsLater(urls);
    }

    return app.exec();
}
