/*
 * SPDX-FileCopyrightText: 2026 SonicDE Community
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#include "inputlisteneritem.h"
#include "nativeinputmethod.h"
#include "plasmakeyboardsettings.h"

#include "overlay/longpresstrigger.h"
#include "overlay/overlaycontroller.h"
#include "overlay/prefixquerytrigger.h"
#include "overlay/textexpansiontrigger.h"

#include <QGuiApplication>
#include <QInputMethod>

Q_GLOBAL_STATIC(NativeInputMethod, s_inputMethod)

InputListenerItem::InputListenerItem()
    : m_input(s_inputMethod)
    , m_overlayController(new OverlayController(&m_input, this))
{
    m_input.setGrabbing(true);
    m_overlayController->registerTrigger(new LongPressTrigger(m_overlayController));
    m_overlayController->registerTrigger(new PrefixQueryTrigger(m_overlayController));
    m_overlayController->registerTrigger(new TextExpansionTrigger(m_overlayController));

    connect(&m_input, &InputPlugin::contextChanged, this, [this] {
        m_overlayController->cancelOverlay();
        // Qt queries and edits the actual application field, not this observer.
        if (m_input.hasContext()) {
            QGuiApplication::inputMethod()->update(Qt::ImQueryAll);
            QGuiApplication::inputMethod()->show();
        } else {
            QGuiApplication::inputMethod()->hide();
        }
    });
    connect(&m_input, &InputPlugin::contentTypeChanged, this, [this] {
        if (m_input.contentHint() & InputPlugin::content_hint_sensitive_data) {
            m_overlayController->cancelOverlay();
        }
    });
    connect(&m_input, &InputPlugin::surroundingTextChanged, m_overlayController, &OverlayController::handleSurroundingTextChanged);
    connect(&m_input, &InputPlugin::keyPressed, this, [this](QKeyEvent *event) {
        if (m_overlayController->processKeyPress(event)) {
            event->accept();
            return;
        }
        if (!QGuiApplication::inputMethod()->isVisible() || !PlasmaKeyboardSettings::self()->keyboardNavigationEnabled()) {
            return;
        }
        if (isNavigationKey(event->key())) {
            event->accept();
            Q_EMIT keyNavigationPressed(event->key());
        }
    });
    connect(&m_input, &InputPlugin::keyReleased, this, [this](QKeyEvent *event) {
        if (m_overlayController->processKeyRelease(event)) {
            event->accept();
            return;
        }
        if (!QGuiApplication::inputMethod()->isVisible() || !PlasmaKeyboardSettings::self()->keyboardNavigationEnabled()) {
            return;
        }
        if (isNavigationKey(event->key())) {
            event->accept();
            Q_EMIT keyNavigationReleased(event->key());
        }
    });
}

bool InputListenerItem::isNavigationKey(int key) const
{
    return key == Qt::Key_Left || key == Qt::Key_Right || key == Qt::Key_Up || key == Qt::Key_Down || (m_keyboardNavigationActive && key == Qt::Key_Return);
}

OverlayController *InputListenerItem::overlayController() const
{
    return m_overlayController;
}
