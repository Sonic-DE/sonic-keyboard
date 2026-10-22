/*
 * SPDX-FileCopyrightText: 2026 SonicDE Community
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
import QtQuick
import QtQuick.Window
import QtQuick.VirtualKeyboard.Settings

// A normal Qt client. It has no custom keyboard component or embedding API.
Window {
    visible: true
    width: 1000
    height: 700
    color: "#606060"
    property alias plain: plain
    property alias password: password
    readonly property string styleName: VirtualKeyboardSettings.styleName

    function focusPlain() { plain.forceActiveFocus(); Qt.inputMethod.show(); }
    function focusPassword() { password.forceActiveFocus(); Qt.inputMethod.show(); }

    TextInput {
        id: plain
        objectName: "plain"
        x: 20; y: 20; width: 900; height: 40
        color: "white"
        focus: true
    }
    TextInput {
        id: password
        objectName: "password"
        x: 20; y: 80; width: 900; height: 40
        color: "white"
        echoMode: TextInput.Password
        inputMethodHints: Qt.ImhHiddenText | Qt.ImhSensitiveData | Qt.ImhNoPredictiveText
    }
}
