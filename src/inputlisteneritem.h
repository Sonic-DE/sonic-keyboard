/*
    SPDX-FileCopyrightText: 2024 Aleix Pol i Gonzalez <aleixpol@kde.org>
    SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
    SPDX-FileCopyrightText: 2025 Kristen McWilliam <kristen@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QObject>
#include <qqmlintegration.h>

#include <xkbcommon/xkbcommon.h>

#include "inputplugin.h"
#include "overlay/overlaycontroller.h"

class InputListenerItem : public QObject {
    Q_OBJECT
    QML_ELEMENT

    Q_PROPERTY(bool keyboardNavigationActive MEMBER m_keyboardNavigationActive)

    /**
     * Controller for overlay popups (diacritics, emoji, text expansion).
     *
     * Exposed to QML for connecting overlay windows.
     */
    Q_PROPERTY(OverlayController *overlayController READ overlayController CONSTANT)

public:
    InputListenerItem();

    /**
     * Get the overlay controller.
     */
    OverlayController *overlayController() const;

Q_SIGNALS:
    void keyNavigationPressed(int key);
    void keyNavigationReleased(int key);

private:
    bool isNavigationKey(int key) const;
    InputPlugin m_input;
    OverlayController *m_overlayController = nullptr;
    bool m_keyboardNavigationActive = false;
};
