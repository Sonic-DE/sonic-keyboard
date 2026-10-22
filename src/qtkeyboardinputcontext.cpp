/*
 * SPDX-FileCopyrightText: 2026 SonicDE Community
 * SPDX-License-Identifier: GPL-3.0-or-later
 */

#include "layoutpathhelper.h"

#include <QGuiApplication>
#include <QPointer>
#include <QQmlApplicationEngine>
#include <QQmlComponent>
#include <QQmlContext>
#include <QScopedValueRollback>
#include <QWindow>
#include <QtVirtualKeyboard/private/platforminputcontext_p.h>
#include <qpa/qplatforminputcontextplugin_p.h>

#include <KLocalizedQmlContext>

#include <memory>

// The input engine, focus, composition, hints, key delivery and public API are
// Qt's actual implementation. This adapter replaces only the desktop view.
class SonicQtVirtualKeyboardInputContext final : public QtVirtualKeyboard::PlatformInputContext {
    Q_OBJECT
public:
    explicit SonicQtVirtualKeyboardInputContext(bool desktop)
        : m_desktop(desktop)
    {
        connect(qGuiApp, &QCoreApplication::aboutToQuit, this, [this] {
            m_stopping = true;
            m_engine.reset();
        });
    }

    void update(Qt::InputMethodQueries queries) override
    {
        if (inputMethodAccepted()) {
            ensureView();
        }
        QtVirtualKeyboard::PlatformInputContext::update(queries);
    }

    void showInputPanel() override
    {
        ensureView();
        QtVirtualKeyboard::PlatformInputContext::showInputPanel();
    }

private:
    void ensureView()
    {
        if (!m_desktop || m_engine || m_loading || m_stopping || !focusObject() || !qGuiApp || QGuiApplication::platformName() != QLatin1String("xcb")) {
            return;
        }
        const QScopedValueRollback loading(m_loading, true);
        auto engine = std::make_unique<QQmlApplicationEngine>();
        auto *translation = new KLocalizedQmlContext(engine.get());
        translation->setTranslationDomain(QStringLiteral("plasma-keyboard"));
        engine->rootContext()->setContextObject(translation);
        QQmlEngine::setContextForObject(translation, engine->rootContext());

        // Register this project's types/resources, then load its existing UI.
        // Do not replace main.qml with a demo or proxy the focused editor.
        QQmlComponent registration(engine.get());
        registration.setData("import QtQml\nimport org.kde.plasma.keyboard\nQtObject {}", QUrl());
        const std::unique_ptr<QObject> module(registration.create());
        if (!module) {
            qWarning() << "Cannot load Sonic Keyboard UI module:" << registration.errors();
            return;
        }
        engine->load(QUrl(QStringLiteral("qrc:/qt/qml/org/kde/plasma/keyboard/main.qml")));
        if (engine->rootObjects().isEmpty()) {
            return;
        }
        auto *window = qobject_cast<QWindow *>(engine->rootObjects().first());
        if (!window) {
            return;
        }
        window->setObjectName(QStringLiteral("sonicKeyboardWindow"));
        m_engine = std::move(engine);
    }

    const bool m_desktop;
    bool m_loading = false;
    bool m_stopping = false;
    std::unique_ptr<QQmlApplicationEngine> m_engine;
};

class SonicQtVirtualKeyboardPlugin final : public QPlatformInputContextPlugin {
    Q_OBJECT
    Q_PLUGIN_METADATA(IID QPlatformInputContextFactoryInterface_iid FILE "qtkeyboardplugin.json")
public:
    QPlatformInputContext *create(const QString &key, const QStringList &) override
    {
        if (key.compare(QLatin1String("sonicvirtualkeyboard"), Qt::CaseInsensitive) != 0
            && key.compare(QLatin1String("qtvirtualkeyboard"), Qt::CaseInsensitive) != 0) {
            return nullptr;
        }
        if (qEnvironmentVariableIsEmpty("QT_VIRTUALKEYBOARD_STYLE")) {
            qputenv("QT_VIRTUALKEYBOARD_STYLE", "Breeze");
        }
        initLayoutsPath();

        // Preserve Qt's public desktop/embedded switch. Its own engine uses
        // AppInputPanel while the original Sonic UI supplies the desktop view.
        const bool hadDisable = qEnvironmentVariableIsSet("QT_VIRTUALKEYBOARD_DESKTOP_DISABLE");
        const QByteArray disable = qgetenv("QT_VIRTUALKEYBOARD_DESKTOP_DISABLE");
        const bool desktop = disable.toInt() == 0;
        qputenv("QT_VIRTUALKEYBOARD_DESKTOP_DISABLE", "1");
        auto *context = new SonicQtVirtualKeyboardInputContext(desktop);
        if (hadDisable) {
            qputenv("QT_VIRTUALKEYBOARD_DESKTOP_DISABLE", disable);
        } else {
            qunsetenv("QT_VIRTUALKEYBOARD_DESKTOP_DISABLE");
        }
        return context;
    }
};

#include "qtkeyboardinputcontext.moc"
