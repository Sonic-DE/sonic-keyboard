/*
    SPDX-FileCopyrightText: 2026 Kristen McWilliam <kristen@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "inputpanelintegration.h"

#include <QDebug>
#include <QGuiApplication>
#include <QInputMethod>
#include <QWindow>

bool initInputPanelIntegration(QWindow *window, InputPanelRole::Role role)
{
    if (!window || QGuiApplication::platformName() != QLatin1String("xcb")) {
        return false;
    }

    // The same non-focus-stealing X11 flags used by Qt's DesktopInputPanel.
    // Qt's input context retains the application's focused editor throughout.
    window->setFlags(Qt::Window | Qt::FramelessWindowHint | Qt::WindowStaysOnTopHint | Qt::WindowDoesNotAcceptFocus | Qt::BypassWindowManagerHint);
    if (role == InputPanelRole::OverlayPanel) {
        auto position = [window] {
            auto *focus = QGuiApplication::focusWindow();
            if (focus && focus != window) {
                window->setPosition(focus->mapToGlobal(QGuiApplication::inputMethod()->cursorRectangle().bottomLeft().toPoint()));
            }
        };
        QObject::connect(QGuiApplication::inputMethod(), &QInputMethod::cursorRectangleChanged, window, position);
        position();
    }
    return true;
}
