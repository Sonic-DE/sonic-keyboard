/*
 * SPDX-FileCopyrightText: 2026 SonicDE Community
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nativeinputmethod.h"

#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QInputMethod>
#include <QInputMethodEvent>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QQmlExpression>
#include <QQmlProperty>
#include <QQuickItem>
#include <QQuickWindow>
#include <QTemporaryDir>
#include <QTest>
#include <QVirtualKeyboardInputContext>
#include <QVirtualKeyboardInputEngine>
#include <private/qguiapplication_p.h>
#include <qpa/qplatforminputcontext.h>
#include <qpa/qplatformintegration.h>

#include <functional>

class QueryCounter : public QObject {
public:
    int textQueries = 0;
    bool eventFilter(QObject *, QEvent *event) override
    {
        if (event->type() == QEvent::InputMethodQuery) {
            const auto queries = static_cast<QInputMethodQueryEvent *>(event)->queries();
            if (queries & (Qt::ImSurroundingText | Qt::ImCursorPosition | Qt::ImAnchorPosition)) {
                ++textQueries;
            }
        }
        return false;
    }
};

class NativeKeyboardTest : public QObject {
    Q_OBJECT
    QQmlApplicationEngine m_engine;
    QQuickWindow *m_window = nullptr;
    QQuickWindow *m_keyboardWindow = nullptr;
    QQuickItem *m_panel = nullptr;
    QPointer<QVirtualKeyboardInputContext> m_context;
    QPointer<QVirtualKeyboardInputEngine> m_inputEngine;
    QObject *m_plain = nullptr;
    QObject *m_password = nullptr;

    void commit(const QString &text)
    {
        QVERIFY(m_context);
        m_context->commit(text);
    }

    void press(int key, const QString &text, Qt::KeyboardModifiers modifiers = Qt::NoModifier)
    {
        QVERIFY(m_inputEngine);
        QVERIFY(m_inputEngine->virtualKeyClick(static_cast<Qt::Key>(key), text, modifiers));
    }

private Q_SLOTS:
    void initTestCase()
    {
        if (qgetenv("SONIC_KEYBOARD_TEST_ISOLATED") != "1") {
            QSKIP("Native keyboard integration requires the private Xvfb CTest wrapper");
        }
        auto *context = QGuiApplicationPrivate::platformIntegration()->inputContext();
        QVERIFY(context);
        QVERIFY2(QByteArray(context->metaObject()->className()).contains("SonicQtVirtualKeyboard"), context->metaObject()->className());
        QVERIFY(QByteArray(context->metaObject()->superClass()->className()).contains("QtVirtualKeyboard::PlatformInputContext"));
        m_engine.addImportPath(QStringLiteral(KEYBOARD_TEST_IMPORT_PATH));
        const bool desktop = qEnvironmentVariableIntValue("SONIC_KEYBOARD_TEST_DESKTOP") != 0;
        m_engine.load(QUrl::fromLocalFile(desktop ? QStringLiteral(KEYBOARD_DESKTOP_TEST_QML) : QStringLiteral(KEYBOARD_TEST_QML)));
        QVERIFY(!m_engine.rootObjects().isEmpty());
        m_window = qobject_cast<QQuickWindow *>(m_engine.rootObjects().first());
        QVERIFY(m_window);
        m_plain = m_window->property("plain").value<QObject *>();
        m_password = m_window->property("password").value<QObject *>();
        QVERIFY(m_plain);
        QVERIFY(m_password);
        m_window->requestActivate();
        QVERIFY(QTest::qWaitForWindowExposed(m_window));
        QVERIFY(QMetaObject::invokeMethod(m_window, "focusPlain"));
        if (desktop) {
            auto findKeyboard = []() -> QQuickWindow * {
                for (auto *window : QGuiApplication::topLevelWindows()) {
                    if (window->objectName() == QLatin1String("sonicKeyboardWindow")) {
                        return qobject_cast<QQuickWindow *>(window);
                    }
                }
                return nullptr;
            };
            QTRY_VERIFY((m_keyboardWindow = findKeyboard()));
            QTRY_VERIFY(m_keyboardWindow->isVisible());
            m_panel = m_keyboardWindow->findChild<QQuickItem *>(QStringLiteral("sonicInputPanel"));
            QVERIFY(m_keyboardWindow->flags() & Qt::WindowDoesNotAcceptFocus);
        } else {
            m_keyboardWindow = m_window;
            m_panel = m_window->property("panel").value<QQuickItem *>();
        }
        QVERIFY(m_panel);
        const QString requestedStyle = qEnvironmentVariable("SONIC_KEYBOARD_TEST_STYLE");
        QTRY_COMPARE(m_window->property("styleName").toString(), requestedStyle.isEmpty() ? QStringLiteral("Breeze") : requestedStyle);
        QTRY_VERIFY(m_panel->property("keyboard").value<QObject *>());
        QQmlExpression engine(QQmlEngine::contextForObject(m_panel), m_panel, QStringLiteral("InputContext.inputEngine"));
        m_inputEngine = qobject_cast<QVirtualKeyboardInputEngine *>(engine.evaluate().value<QObject *>());
        QVERIFY2(!engine.hasError(), qPrintable(engine.error().toString()));
        QVERIFY(m_inputEngine);
        m_context = m_inputEngine->inputContext();
        QVERIFY(m_context);
    }

    void standardInputMethodApi()
    {
        auto *input = QGuiApplication::inputMethod();
        input->hide();
        QTRY_VERIFY(!input->isVisible());
        input->show();
        QTRY_VERIFY(input->isVisible());
        input->update(Qt::ImQueryAll);
        QTRY_VERIFY(!input->keyboardRectangle().isEmpty());
        QVERIFY(!input->locale().name().isEmpty());
        QCOMPARE(QGuiApplication::focusObject(), m_plain);
        QCOMPARE(QGuiApplication::focusWindow(), m_window);
    }

    void bundledLayoutsUseTheQtLayoutApi()
    {
        QCOMPARE(qgetenv("QT_VIRTUALKEYBOARD_LAYOUT_PATH"), QStringLiteral(KEYBOARD_TEST_LAYOUTS).toUtf8());
        QVERIFY(QDir(QStringLiteral(KEYBOARD_TEST_LAYOUTS)).exists());
        QVERIFY(!m_inputEngine->inputModes().isEmpty());
    }

    void init()
    {
        QVERIFY(QMetaObject::invokeMethod(m_window, "focusPlain"));
        QTRY_COMPARE(QGuiApplication::focusObject(), m_plain);
        QVERIFY(m_plain->setProperty("text", QString()));
        QVERIFY(m_password->setProperty("text", QString()));
        QTRY_VERIFY(m_inputEngine->inputMethod());
    }

    void nativeUnicodeAndKeysKeepEditorFocus()
    {
        commit(QStringLiteral("Qt "));
        commit(QString::fromUtf8("é中🙂"));
        QTRY_COMPARE(m_plain->property("text").toString(), QString::fromUtf8("Qt é中🙂"));
        press(Qt::Key_Backspace, QString());
        QTRY_COMPARE(m_plain->property("text").toString(), QString::fromUtf8("Qt é中"));
        QCOMPARE(QGuiApplication::focusObject(), m_plain);
        press(Qt::Key_A, QStringLiteral("A"), Qt::ShiftModifier);
        press(Qt::Key_A, QStringLiteral("a"));
        // Normal Qt input methods may hold letters as preedit until commit.
        QGuiApplication::inputMethod()->commit();
        QTRY_COMPARE(m_plain->property("text").toString(), QString::fromUtf8("Qt é中Aa"));
        // Physical shortcuts continue to reach the editor while the OSK is up.
        QTest::keyClick(m_window, Qt::Key_A, Qt::ControlModifier);
        QTRY_COMPARE(m_plain->property("selectedText").toString(), m_plain->property("text").toString());
        commit(QStringLiteral("replacement"));
        QTRY_COMPARE(m_plain->property("text").toString(), QStringLiteral("replacement"));
        QCOMPARE(QGuiApplication::focusObject(), m_plain);
    }

    void styledKeyClickUsesQtBackend()
    {
        std::function<QQuickItem *(QQuickItem *)> findKey = [&](QQuickItem *item) -> QQuickItem * {
            if (item->isVisible() && item->isEnabled() && item->width() > 0 && item->height() > 0 && item->property("key").isValid()
                && item->property("text").toString().toLower() == QLatin1String("a")) {
                return item;
            }
            for (auto *child : item->childItems()) {
                if (auto *key = findKey(child)) {
                    return key;
                }
            }
            return nullptr;
        };
        QQuickItem *key = nullptr;
        QTRY_VERIFY((key = findKey(m_panel)));
        const auto point = key->mapToScene(QPointF(key->width() / 2, key->height() / 2)).toPoint();
        QTest::mouseClick(m_keyboardWindow, Qt::LeftButton, Qt::NoModifier, point);
        QGuiApplication::inputMethod()->commit();
        QTRY_COMPARE(m_plain->property("text").toString().toLower(), QStringLiteral("a"));
        QCOMPARE(QGuiApplication::focusObject(), m_plain);
        QCOMPARE(QGuiApplication::focusWindow(), m_window);
    }

    void languageSwitcherOpensAndSelectsLayout()
    {
        if (m_keyboardWindow == m_window) {
            QSKIP("The custom language popup belongs to the desktop frontend");
        }
        std::function<QQuickItem *(QQuickItem *, const QString &)> findItem = [&](QQuickItem *item, const QString &name) -> QQuickItem * {
            if (item->objectName() == name && item->isVisible()) {
                return item;
            }
            for (auto *child : item->childItems()) {
                if (auto *found = findItem(child, name)) {
                    return found;
                }
            }
            return nullptr;
        };
        auto *key = findItem(m_panel, QStringLiteral("changeLanguageKey"));
        QVERIFY(key);
        // Empty project preferences must not disable the switcher by forcing
        // Qt's activeLocales down to a single locale.
        QTRY_VERIFY_WITH_TIMEOUT(key->isEnabled(), 2000);
        const QRegion closedMask = m_keyboardWindow->mask();
        auto *popup = m_keyboardWindow->findChild<QObject *>(QStringLiteral("sonicLanguagePopup"));
        QVERIFY(popup);
        auto clickItem = [&](QQuickItem *item) {
            const auto point = item->mapToScene(QPointF(item->width() / 2, item->height() / 2)).toPoint();
            QTest::mouseClick(m_keyboardWindow, Qt::LeftButton, Qt::NoModifier, point);
        };
        // The default full locale list is tall enough to extend above the key
        // panel. It must remain visible and accept an outside-click dismissal.
        clickItem(key);
        QTRY_VERIFY(popup->property("visible").toBool());
        QVERIFY(popup->property("localeCount").toInt() > 2);
        auto *content = popup->property("contentItem").value<QQuickItem *>();
        QVERIFY(content);
        QVERIFY(m_keyboardWindow->mask().contains(content->mapToScene(QPointF(1, 1)).toPoint()));
        QVERIFY(m_keyboardWindow->mask().contains(QPoint(20, 20)));
        QTest::mouseClick(m_keyboardWindow, Qt::LeftButton, Qt::NoModifier, QPoint(20, 20));
        QTRY_VERIFY(!popup->property("visible").toBool());
        QTRY_COMPARE(m_keyboardWindow->mask(), closedMask);

        QQmlExpression configure(QQmlEngine::contextForObject(m_panel),
                                 m_panel,
                                 QStringLiteral("VirtualKeyboardSettings.activeLocales = ['en_US', 'de_DE']; VirtualKeyboardSettings.locale = 'en_US'"));
        configure.evaluate();
        QVERIFY2(!configure.hasError(), qPrintable(configure.error().toString()));
        auto *keyboard = m_panel->property("keyboard").value<QObject *>();
        QVERIFY(keyboard);
        // The selected layout locale can differ from its fallback input-method
        // locale (for example the shipped en_US layout shares English input).
        QTRY_COMPARE(keyboard->property("locale").toString(), QStringLiteral("en_US"));
        QTRY_VERIFY(key->isEnabled());
        clickItem(key);
        QTRY_VERIFY(popup->property("visible").toBool());
        QTRY_COMPARE(popup->property("localeCount").toInt(), 2);
        QQuickItem *german = nullptr;
        QTRY_VERIFY((german = findItem(content, QStringLiteral("locale-de_DE"))));
        const auto point = german->mapToScene(QPointF(german->width() / 2, german->height() / 2)).toPoint();
        QVERIFY2(m_keyboardWindow->mask().contains(point), "Language popup is clipped by the keyboard's X11 mask");
        clickItem(german);
        QTRY_COMPARE(keyboard->property("locale").toString(), QStringLiteral("de_DE"));
        QTRY_VERIFY(keyboard->property("layout").toString().contains(QStringLiteral("/de_DE/")));
        QTRY_VERIFY(!popup->property("visible").toBool());
        QTRY_COMPARE(m_keyboardWindow->mask(), closedMask);
        std::function<QQuickItem *(QQuickItem *)> findUmlaut = [&](QQuickItem *item) -> QQuickItem * {
            if (item->isVisible() && item->isEnabled() && item->property("key").isValid()
                && item->property("text").toString().toLower() == QString::fromUtf8("ü")) {
                return item;
            }
            for (auto *child : item->childItems()) {
                if (auto *found = findUmlaut(child)) {
                    return found;
                }
            }
            return nullptr;
        };
        QQuickItem *umlaut = nullptr;
        QTRY_VERIFY((umlaut = findUmlaut(m_panel)));
        clickItem(umlaut);
        QGuiApplication::inputMethod()->commit();
        QTRY_COMPARE(m_plain->property("text").toString().toLower(), QString::fromUtf8("ü"));
        QCOMPARE(QGuiApplication::focusObject(), m_plain);
        QCOMPARE(QGuiApplication::focusWindow(), m_window);

        // Closing by clicking outside must restore the normal click-through
        // region, not leave an invisible full-screen input window behind.
        key = findItem(m_panel, QStringLiteral("changeLanguageKey"));
        QVERIFY(key);
        clickItem(key);
        QTRY_VERIFY(popup->property("visible").toBool());
        QTest::mouseClick(m_keyboardWindow, Qt::LeftButton, Qt::NoModifier, QPoint(20, 20));
        QTRY_VERIFY(!popup->property("visible").toBool());
        QTRY_COMPARE(m_keyboardWindow->mask(), closedMask);
    }

    void passwordHintsAndOverlayPrivacy()
    {
        QVERIFY(QMetaObject::invokeMethod(m_window, "focusPassword"));
        QTRY_COMPARE(QGuiApplication::focusObject(), m_password);
        // Qt keeps the editor's password echo mode separately from the hints
        // exposed to its keyboard engine; sensitive/no-prediction must survive.
        const auto required = int(Qt::ImhSensitiveData | Qt::ImhNoPredictiveText);
        QTRY_COMPARE(int(m_context->inputMethodHints()) & required, required);
        commit(QStringLiteral("private-"));
        commit(QString::fromUtf8("é中🙂"));
        QTRY_COMPARE(m_password->property("text").toString(), QString::fromUtf8("private-é中🙂"));
        QVERIFY(!m_password->property("displayText").toString().contains(QStringLiteral("private")));
        QueryCounter counter;
        m_password->installEventFilter(&counter);
        NativeInputMethod observer;
        observer.refresh();
        QVERIFY(observer.hasContext());
        QVERIFY(observer.isSensitive());
        QVERIFY(!observer.query(Qt::ImSurroundingText).isValid());
        QVERIFY(!observer.query(Qt::ImCursorPosition).isValid());
        QCOMPARE(counter.textQueries, 0);
        m_password->removeEventFilter(&counter);
    }

    void explicitPaletteBindings_data()
    {
        QTest::addColumn<QColor>("foreground");
        QTest::addColumn<QColor>("background");
        QTest::newRow("dark") << QColor(QStringLiteral("#beffe6")) << QColor(QStringLiteral("#202428"));
        QTest::newRow("light") << QColor(QStringLiteral("#183d56")) << QColor(QStringLiteral("#f5f5f5"));
    }

    void explicitPaletteBindings()
    {
        QFETCH(QColor, foreground);
        QFETCH(QColor, background);
        auto *keyboard = m_panel->property("keyboard").value<QObject *>();
        QVERIFY(keyboard);
        auto *style = keyboard->property("style").value<QObject *>();
        QVERIFY(style);
        auto *theme = style->property("theme").value<QObject *>();
        QVERIFY(theme);
        QQmlProperty textColor(theme, QStringLiteral("Kirigami.Theme.textColor"), QQmlEngine::contextForObject(theme));
        QQmlProperty backgroundColor(theme, QStringLiteral("Kirigami.Theme.backgroundColor"), QQmlEngine::contextForObject(theme));
        QVERIFY(textColor.isValid());
        QVERIFY(backgroundColor.isValid());
        QVERIFY(textColor.write(foreground));
        QVERIFY(backgroundColor.write(background));
        QTRY_COMPARE(theme->property("keyTextColor").value<QColor>(), foreground);
        QTRY_COMPARE(theme->property("keyboardBackgroundColor").value<QColor>(), background);
        QVERIFY(!theme->property("fontFamily").toString().isEmpty());
        QImage screenshot;
        QTRY_VERIFY(!(screenshot = m_keyboardWindow->grabWindow()).isNull());
        auto count = [&](const QColor &color) {
            const QImage image = m_keyboardWindow->grabWindow();
            int result = 0;
            const int top = int(m_panel->mapToScene(QPointF()).y());
            for (int y = qMax(0, top); y < image.height(); ++y) {
                for (int x = 0; x < image.width(); ++x) {
                    if (image.pixelColor(x, y) == color) {
                        ++result;
                    }
                }
            }
            return result;
        };
        QTRY_VERIFY_WITH_TIMEOUT(count(background) > 100, 5000);
        QTRY_VERIFY_WITH_TIMEOUT(count(foreground) > 50, 5000);
    }
};

int main(int argc, char **argv)
{
    QTemporaryDir home;
    if (!home.isValid()) {
        return 1;
    }
    qputenv("HOME", home.path().toUtf8());
    qputenv("XDG_CONFIG_HOME", (home.path() + QStringLiteral("/config")).toUtf8());
    qputenv("XDG_CONFIG_DIRS", (home.path() + QStringLiteral("/empty-config")).toUtf8());
    qputenv("XDG_DATA_HOME", (home.path() + QStringLiteral("/data")).toUtf8());
    qputenv("XDG_CACHE_HOME", (home.path() + QStringLiteral("/cache")).toUtf8());
    qputenv("QT_QPA_PLATFORMTHEME", "generic");
    qputenv("QT_NO_XDG_DESKTOP_PORTAL", "1");
    qputenv("DBUS_SESSION_BUS_ADDRESS", "unix:path=/nonexistent/sonic-keyboard-test-bus");
    qputenv("DBUS_SYSTEM_BUS_ADDRESS", "unix:path=/nonexistent/sonic-keyboard-test-system-bus");
    QDir().mkpath(home.path() + QStringLiteral("/runtime"));
    QFile::setPermissions(home.path() + QStringLiteral("/runtime"), QFile::ReadOwner | QFile::WriteOwner | QFile::ExeOwner);
    qputenv("XDG_RUNTIME_DIR", (home.path() + QStringLiteral("/runtime")).toUtf8());
    qputenv("QT_SCALE_FACTOR", "1");
    qunsetenv("QT_SCREEN_SCALE_FACTORS");
    const QByteArray module = qgetenv("SONIC_KEYBOARD_TEST_MODULE");
    qputenv("QT_IM_MODULE", module.isEmpty() ? QByteArray("qtvirtualkeyboard") : module);
    qputenv("QT_VIRTUALKEYBOARD_LAYOUT_PATH", QStringLiteral(KEYBOARD_TEST_LAYOUTS).toUtf8());
    if (qEnvironmentVariableIntValue("SONIC_KEYBOARD_TEST_DESKTOP")) {
        qunsetenv("QT_VIRTUALKEYBOARD_DESKTOP_DISABLE");
    } else {
        qputenv("QT_VIRTUALKEYBOARD_DESKTOP_DISABLE", "1");
    }
    const QByteArray requestedStyle = qgetenv("SONIC_KEYBOARD_TEST_STYLE");
    if (requestedStyle.isEmpty()) {
        qunsetenv("QT_VIRTUALKEYBOARD_STYLE");
    } else {
        qputenv("QT_VIRTUALKEYBOARD_STYLE", requestedStyle);
    }
    QCoreApplication::addLibraryPath(QStringLiteral(KEYBOARD_TEST_PLUGIN_PATH));
    QGuiApplication app(argc, argv);
    NativeKeyboardTest test;
    int result = 1;
    QTimer::singleShot(0, &app, [&] {
        result = QTest::qExec(&test, argc, argv);
        app.exit(result);
    });
    app.exec();
    return result;
}

#include "nativekeyboardtest.moc"
