/*
    SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
    SPDX-FileCopyrightText: 2026 Kristen McWilliam <kristen@kde.org>

    SPDX-License-Identifier: GPL-2.0-only OR GPL-3.0-only OR LicenseRef-KDE-Accepted-GPL
*/

#include "inputpanelwindow.h"

#include "inputpanelintegration.h"

#include <KSandbox>
#include <QDesktopServices>
#include <QGuiApplication>
#include <QProcess>
#include <QScreen>
#include <qnamespace.h>

InputPanelWindow::InputPanelWindow(QWindow *parent)
    : InputPanelWindow(true, parent)
{
}

InputPanelWindow::InputPanelWindow(bool settingsAvailable, QWindow *parent)
    : QQuickWindow{parent}
    , m_settingsAvailable(settingsAvailable)
{
    initInputPanelIntegration(this, InputPanelRole::Keyboard);
    connect(qGuiApp, &QGuiApplication::focusWindowChanged, this, [this](QWindow *focus) {
        if (focus && focus != this) {
            setScreen(focus->screen());
            setPosition(focus->screen()->geometry().topLeft());
        }
    });
    if (auto *focus = QGuiApplication::focusWindow(); focus && focus->screen()) {
        setScreen(focus->screen());
        setPosition(focus->screen()->geometry().topLeft());
    }
}

QRect InputPanelWindow::interactiveRegion() const
{
    return m_interactiveRegion;
}

void InputPanelWindow::setInteractiveRegion(QRect interactiveRegion)
{
    if (interactiveRegion == m_interactiveRegion) {
        return;
    }
    m_interactiveRegion = interactiveRegion;
    Q_EMIT interactiveRegionChanged();

    // Set only a part of the window to be interactive
    setMask(QRegion(m_interactiveRegion));
}

bool InputPanelWindow::settingsAvailable() const
{
    return m_settingsAvailable;
}

void InputPanelWindow::setSettingsAvailable(bool available)
{
    if (m_settingsAvailable == available) {
        return;
    }
    m_settingsAvailable = available;
    Q_EMIT settingsAvailableChanged();
}

void InputPanelWindow::showSettings()
{
    if (!settingsAvailable()) {
        return;
    }
    if (KSandbox::isInside()) {
        QProcess::startDetached(QStringLiteral("kcmshell6"), {QStringLiteral("kcm_plasmakeyboard")});
    } else {
        QDesktopServices::openUrl(QUrl(QStringLiteral("systemsettings:kcm_plasmakeyboard")));
    }
}

bool InputPanelWindow::initInputPanel(InputPanelRole::Role role)
{
    return initInputPanelIntegration(this, role);
}

#include "moc_inputpanelwindow.cpp"
