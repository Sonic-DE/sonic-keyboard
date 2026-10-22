/*
    SPDX-FileCopyrightText: 2024 Aleix Pol i Gonzalez <aleixpol@kde.org>
    SPDX-FileCopyrightText: 2025 Kristen McWilliam <kristen@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "config-plasma-keyboard.h"
#include "layoutpathhelper.h"
#include <plasma_keyboard_version.h>

#include <KAboutData>
#include <KConfigWatcher>
#include <KCoreConfigSkeleton>
#include <KCrash>
#include <KLocalizedQmlContext>
#include <KLocalizedString>

#include <QCommandLineParser>
#include <QDir>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QWindow>

#include <memory>

// signal handler for SIGINT & SIGTERM
#ifdef Q_OS_UNIX
#include <KSignalHandler>
#include <signal.h>
#include <unistd.h>
#endif

int main(int argc, char **argv)
{
    qputenv("QT_IM_MODULE", "sonicvirtualkeyboard");
    qputenv("QT_QPA_PLATFORM", "xcb");
    initLayoutsPath();

    QGuiApplication application(argc, argv);

    KLocalizedString::setApplicationDomain("plasma-keyboard");

    KAboutData aboutData(QStringLiteral("plasma-keyboard"),
                         i18n("Plasma Keyboard"),
                         QStringLiteral(PLASMA_KEYBOARD_VERSION_STRING),
                         i18n("An on-screen keyboard for Plasma"),
                         KAboutLicense::GPL,
                         i18n("Copyright 2024, Aleix Pol Gonzalez"));

    aboutData.addAuthor(i18n("Aleix Pol Gonzalez"), i18n("Author"), QStringLiteral("aleixpol@kde.org"));
    aboutData.setOrganizationDomain("kde.org");
    aboutData.setDesktopFileName(QStringLiteral("org.kde.plasma.keyboard"));
    application.setWindowIcon(QIcon::fromTheme(QStringLiteral("input-keyboard-virtual")));
    aboutData.setProgramLogo(application.windowIcon());

    KAboutData::setApplicationData(aboutData);

    KCrash::initialize();

    {
        QCommandLineParser parser;
        aboutData.setupCommandLine(&parser);
        parser.process(application);
        aboutData.processCommandLine(&parser);
    }

    QQmlApplicationEngine view;
    KLocalization::setupLocalizedContext(&view);
    QQmlComponent registration(&view);
    registration.setData("import QtQml\nimport org.kde.plasma.keyboard\nQtObject {}", QUrl());
    const std::unique_ptr<QObject> module(registration.create());
    if (!module) {
        qCritical() << registration.errors();
        return 1;
    }
    auto *settings = qobject_cast<KCoreConfigSkeleton *>(
        view.singletonInstance<QObject *>(QStringLiteral("org.kde.plasma.keyboard"), QStringLiteral("PlasmaKeyboardSettings")));
    if (!settings) {
        qCritical("Cannot load keyboard settings");
        return 1;
    }
    if (!PLASMA_KEYBOARD_SOUND_ENABLED) {
        settings->setProperty("soundEnabled", false);
    }
    if (!PLASMA_KEYBOARD_VIBRATION_ENABLED) {
        settings->setProperty("vibrationEnabled", false);
    }

    // Listen to config updates from kcm, and reparse
    auto watcher = KConfigWatcher::create(settings->sharedConfig());
    // clang-format off
    QObject::connect(watcher.get(),
        &KConfigWatcher::configChanged,
        &application,
        [settings](const KConfigGroup &, const QByteArrayList &) {
            settings->sharedConfig()->reparseConfiguration();
            settings->load();
        });
    // clang-format on

    QObject::connect(&view, &QQmlApplicationEngine::objectCreated, &application, [](QObject *object) {
        auto window = qobject_cast<QWindow *>(object);
        if (!window) {
            QCoreApplication::exit(1);
            return;
        }

        window->setVisible(true);
    });
    view.load(QUrl(QStringLiteral("qrc:/qt/qml/org/kde/plasma/keyboard/main.qml")));

#ifdef Q_OS_UNIX
    /**
     * Set up signal handler for SIGINT and SIGTERM
     */
    KSignalHandler::self()->watchSignal(SIGINT);
    KSignalHandler::self()->watchSignal(SIGTERM);
    QObject::connect(KSignalHandler::self(), &KSignalHandler::signalReceived, &application, [](int signal) {
        if (signal == SIGINT || signal == SIGTERM) {
            qDebug() << "Received signal" << signal << ", exiting now.";
            QCoreApplication::quit();
        }
    });
#endif

    qDebug() << "Starting Plasma Keyboard application";

    return application.exec();
}
