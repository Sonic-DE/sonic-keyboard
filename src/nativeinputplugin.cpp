/*
 * SPDX-FileCopyrightText: 2026 SonicDE Community
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "inputplugin.h"
#include "nativeinputmethod.h"

#include <QInputMethodEvent>
#include <QtGui/private/qxkbcommon_p.h>

InputPlugin::InputPlugin(NativeInputMethod *inputMethod)
    : m_inputMethod(inputMethod)
{
    connect(inputMethod, &NativeInputMethod::contextChanged, this, [this] {
        m_deleteStart = m_deleteLength = 0;
        m_preedit.clear();
        Q_EMIT contextChanged();
        if (!hasContext()) {
            Q_EMIT deactivate();
        }
    });
    connect(inputMethod, &NativeInputMethod::surroundingTextChanged, this, &InputPlugin::surroundingTextChanged);
    connect(inputMethod, &NativeInputMethod::contentTypeChanged, this, &InputPlugin::contentTypeChanged);
    connect(inputMethod, &NativeInputMethod::keyPressed, this, [this](QKeyEvent *event) {
        if (m_grabbing) {
            Q_EMIT keyPressed(event);
        }
    });
    connect(inputMethod, &NativeInputMethod::keyReleased, this, [this](QKeyEvent *event) {
        if (m_grabbing) {
            Q_EMIT keyReleased(event);
        }
    });
}

InputPlugin::~InputPlugin() = default;

void InputPlugin::setGrabbing(bool grabbing)
{
    // Local Qt event filtering, not a server-side keyboard grab.
    m_grabbing = grabbing;
}

void InputPlugin::setPreEditString(const QString &text)
{
    m_preedit = text;
    QInputMethodEvent event(text, {QInputMethodEvent::Attribute(QInputMethodEvent::Cursor, m_preeditCursor, 1)});
    m_inputMethod->sendEvent(&event);
}

void InputPlugin::moveCursor(int cursorPosition, int anchorPosition)
{
    QInputMethodEvent event({}, {QInputMethodEvent::Attribute(QInputMethodEvent::Selection, cursorPosition, anchorPosition - cursorPosition)});
    m_inputMethod->sendEvent(&event);
}

void InputPlugin::setPreEditCursor(int cursorPosition)
{
    m_preeditCursor = cursorPosition;
}

void InputPlugin::setPreEditStyle(int, int, int)
{
    // Ordinary virtual-keyboard composition styling belongs to Qt's engine.
}

void InputPlugin::deleteSurroundingText(int start, int length)
{
    // Qt replacement ranges use UTF-16 positions, like QString and the local
    // overlay controller. Apply replacement atomically with the next commit.
    m_deleteStart = start;
    m_deleteLength = length;
}

void InputPlugin::commit(const QString &text)
{
    QInputMethodEvent event;
    event.setCommitString(text, m_deleteStart, m_deleteLength);
    m_deleteStart = m_deleteLength = 0;
    if (m_inputMethod->sendEvent(&event)) {
        m_inputMethod->refresh();
        Q_EMIT receivedCommit();
    }
}

void InputPlugin::keysym(uint, uint symbol, KeyState state, uint modifiers)
{
    const auto qtModifiers = Qt::KeyboardModifiers(modifiers);
    QKeyEvent event(state == Released ? QEvent::KeyRelease : QEvent::KeyPress, QXkbCommon::keysymToQtKey(symbol, qtModifiers), qtModifiers);
    m_inputMethod->sendEvent(&event);
}

void InputPlugin::key(KeyState state, quint32 scanCode)
{
    m_inputMethod->replayKey(scanCode, state != Released);
}

InputPlugin::ContentHint InputPlugin::contentHint() const
{
    const auto hints = m_inputMethod->hints();
    int result = content_hint_none;
    if (m_inputMethod->isSensitive()) {
        result |= content_hint_password;
    }
    if (!(hints & Qt::ImhNoAutoUppercase)) {
        result |= content_hint_auto_capitalization;
    }
    if (hints & Qt::ImhPreferLowercase) {
        result |= content_hint_lowercase;
    }
    if (hints & Qt::ImhPreferUppercase) {
        result |= content_hint_uppercase;
    }
    if (hints & Qt::ImhMultiLine) {
        result |= content_hint_multiline;
    }
    return static_cast<ContentHint>(result);
}

InputPlugin::ContentPurpose InputPlugin::contentPurpose() const
{
    if (m_inputMethod->isSensitive()) {
        return content_purpose_password;
    }
    const auto hints = m_inputMethod->hints();
    if (hints & Qt::ImhDigitsOnly) {
        return content_purpose_digits;
    }
    if (hints & Qt::ImhDialableCharactersOnly) {
        return content_purpose_phone;
    }
    if (hints & Qt::ImhEmailCharactersOnly) {
        return content_purpose_email;
    }
    if (hints & Qt::ImhUrlCharactersOnly) {
        return content_purpose_url;
    }
    return content_purpose_normal;
}

QString InputPlugin::surroundingText() const
{
    return m_inputMethod->query(Qt::ImSurroundingText).toString();
}

uint32_t InputPlugin::cursorPos() const
{
    return surroundingText().first(m_inputMethod->query(Qt::ImCursorPosition).toInt()).toUtf8().size();
}

uint32_t InputPlugin::anchorPos() const
{
    return surroundingText().first(m_inputMethod->query(Qt::ImAnchorPosition).toInt()).toUtf8().size();
}

bool InputPlugin::hasContext() const
{
    return m_inputMethod->hasContext();
}
