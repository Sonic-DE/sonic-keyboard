/*
 * SPDX-FileCopyrightText: 2026 SonicDE Community
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "nativeinputmethod.h"

#include <QCoreApplication>
#include <QGuiApplication>
#include <QInputMethodEvent>
#include <QScopedValueRollback>

namespace {
QVariant queryObject(QObject *object, Qt::InputMethodQuery query)
{
    if (!object) {
        return {};
    }
    QInputMethodQueryEvent event(query);
    QCoreApplication::sendEvent(object, &event);
    return event.value(query);
}
}

NativeInputMethod::NativeInputMethod(QObject *parent)
    : QObject(parent)
{
    qGuiApp->installEventFilter(this);
    connect(qGuiApp, &QGuiApplication::focusObjectChanged, this, &NativeInputMethod::refresh);
    // Some Qt editors change their cursor/text without a key event. Observe
    // only this application's focused editor and keep overlay state current.
    m_poll.setInterval(50);
    connect(&m_poll, &QTimer::timeout, this, &NativeInputMethod::refresh);
    m_poll.start();
    refresh();
}

NativeInputMethod::~NativeInputMethod()
{
    if (qGuiApp) {
        qGuiApp->removeEventFilter(this);
    }
}

QObject *NativeInputMethod::target() const
{
    return m_target;
}

bool NativeInputMethod::hasContext() const
{
    return m_target && m_target == QGuiApplication::focusObject();
}

bool NativeInputMethod::isSensitive() const
{
    return m_hints.testFlag(Qt::ImhSensitiveData) || m_hints.testFlag(Qt::ImhHiddenText);
}

Qt::InputMethodHints NativeInputMethod::hints() const
{
    return m_hints;
}

QVariant NativeInputMethod::query(Qt::InputMethodQuery query) const
{
    if (!hasContext()) {
        return {};
    }
    if (isSensitive() && query != Qt::ImHints && query != Qt::ImEnabled) {
        return {};
    }
    return queryObject(m_target, query);
}

bool NativeInputMethod::sendEvent(QEvent *event)
{
    if (!hasContext() || !event) {
        return false;
    }
    const QScopedValueRollback dispatching(m_dispatching, true);
    return QCoreApplication::sendEvent(m_target, event);
}

void NativeInputMethod::replayKey(quint32 scanCode, bool pressed)
{
    const auto original = m_keys.value(scanCode);
    if (!original || !hasContext()) {
        return;
    }
    QKeyEvent event(pressed ? QEvent::KeyPress : QEvent::KeyRelease,
                    original->key(),
                    original->modifiers(),
                    original->nativeScanCode(),
                    original->nativeVirtualKey(),
                    original->nativeModifiers(),
                    original->text());
    sendEvent(&event);
}

void NativeInputMethod::refresh()
{
    if (m_refreshing) {
        return;
    }
    const QScopedValueRollback refreshing(m_refreshing, true);
    QObject *object = QGuiApplication::focusObject();
    if (!queryObject(object, Qt::ImEnabled).toBool()) {
        object = nullptr;
    }
    const auto hints = Qt::InputMethodHints(queryObject(object, Qt::ImHints).toInt());
    const bool changed = m_target != object;
    const bool hintsChanged = m_hints != hints;
    m_target = object;
    m_hints = hints;
    if (changed) {
        m_keys.clear();
        Q_EMIT contextChanged();
    }
    if (changed || hintsChanged) {
        Q_EMIT contentTypeChanged();
    }
    const QString text = query(Qt::ImSurroundingText).toString();
    const int cursor = query(Qt::ImCursorPosition).toInt();
    const int anchor = query(Qt::ImAnchorPosition).toInt();
    if (text != m_text || cursor != m_cursor || anchor != m_anchor) {
        m_text = text;
        m_cursor = cursor;
        m_anchor = anchor;
        Q_EMIT surroundingTextChanged();
    }
}

bool NativeInputMethod::eventFilter(QObject *object, QEvent *event)
{
    if (m_dispatching || object != QGuiApplication::focusObject() || (event->type() != QEvent::KeyPress && event->type() != QEvent::KeyRelease)) {
        return false;
    }
    refresh();
    // Password/PIN fields must never enter the physical-key overlay pipeline.
    if (!hasContext() || isSensitive()) {
        return false;
    }
    auto *key = static_cast<QKeyEvent *>(event);
    // Qt's virtual-keyboard backend delivers synthetic Qt keys with no native
    // scan code. Never feed those back into physical-key overlay processing.
    if (key->nativeScanCode() == 0) {
        return false;
    }
    m_keys.insert(key->nativeScanCode(), std::shared_ptr<QKeyEvent>(key->clone()));
    const bool originallyAccepted = key->isAccepted();
    key->setAccepted(false);
    if (event->type() == QEvent::KeyPress) {
        Q_EMIT keyPressed(key);
    } else {
        Q_EMIT keyReleased(key);
    }
    if (key->isAccepted()) {
        return true;
    }
    key->setAccepted(originallyAccepted);
    return false;
}
