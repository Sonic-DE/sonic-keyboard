/*
    SPDX-FileCopyrightText: 2026 Kristen McWilliam <kristen@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#pragma once

#include <QObject>
#include <qqmlintegration.h>

/**
 * @file
 *
 * QML-visible roles for the keyboard and local X11 overlay windows.
 */

/**
 * Exposes input panel role values to both C++ and QML.
 */
class InputPanelRole : public QObject
{
    Q_OBJECT
    QML_ELEMENT
    QML_UNCREATABLE("InputPanelRole is an enum container and cannot be instantiated.")

public:
    /**
     * Declares how the local panel window should be positioned.
     *
     * InputPanelIntegration uses the role for placement without taking focus
     * away from the application's editor.
     */
    enum Role {
        /** Main on-screen keyboard window. */
        Keyboard = 0,
        /** Overlay panel associated with and in proximity to a text input field, such as a diacritics/emoji popup. */
        OverlayPanel = 1,
    };
    Q_ENUM(Role)
};
