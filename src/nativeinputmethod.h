/*
 * SPDX-FileCopyrightText: 2026 SonicDE Community
 * SPDX-License-Identifier: GPL-2.0-or-later
 */

#pragma once

#include <QHash>
#include <QKeyEvent>
#include <QObject>
#include <QPointer>
#include <QTimer>

#include <memory>

/**
 * Application-local adapter for the optional physical-key overlays.
 *
 * Ordinary virtual-key input is delivered by Qt Virtual Keyboard itself. This
 * adapter never reads another process, grabs X11 input, or injects global keys.
 * Sensitive fields expose hints only, never surrounding text, to the overlays.
 */
class NativeInputMethod : public QObject {
    Q_OBJECT
public:
    explicit NativeInputMethod(QObject *parent = nullptr);
    ~NativeInputMethod() override;

    QObject *target() const;
    bool hasContext() const;
    bool isSensitive() const;
    Qt::InputMethodHints hints() const;
    QVariant query(Qt::InputMethodQuery query) const;
    bool sendEvent(QEvent *event);
    void replayKey(quint32 scanCode, bool pressed);

public Q_SLOTS:
    void refresh();

Q_SIGNALS:
    void contextChanged();
    void surroundingTextChanged();
    void contentTypeChanged();
    void keyPressed(QKeyEvent *event);
    void keyReleased(QKeyEvent *event);

protected:
    bool eventFilter(QObject *object, QEvent *event) override;

private:
    QPointer<QObject> m_target;
    Qt::InputMethodHints m_hints;
    QString m_text;
    int m_cursor = 0;
    int m_anchor = 0;
    QTimer m_poll;
    QHash<quint32, std::shared_ptr<QKeyEvent>> m_keys;
    bool m_dispatching = false;
    bool m_refreshing = false;
};
