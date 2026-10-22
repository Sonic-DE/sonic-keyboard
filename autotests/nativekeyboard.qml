/*
 * SPDX-FileCopyrightText: 2026 SonicDE Community
 * SPDX-License-Identifier: GPL-2.0-or-later
 */
import QtQuick
import QtQuick.Window
import QtQuick.VirtualKeyboard
import QtQuick.VirtualKeyboard.Settings
import org.kde.kirigami as Kirigami
import org.kde.plasma.keyboard.lib as Keyboard

Window {
    id: root
    visible: true
    width: 1000
    height: 700
    color: "#606060"
    property alias plain: plain
    property alias password: password
    property alias panel: panel
    readonly property string styleName: VirtualKeyboardSettings.styleName
    readonly property var style: panel.keyboard.style
    readonly property var theme: Keyboard.BreezeConstants
    readonly property int nativeHints: panel.InputContext.inputMethodHints

    function focusPlain() { plain.forceActiveFocus(); Qt.inputMethod.show(); }
    function focusPassword() { password.forceActiveFocus(); Qt.inputMethod.show(); }
    function commit(value) { panel.InputContext.commit(value); }
    function press(key, text, modifiers) { panel.InputContext.inputEngine.virtualKeyClick(key, text, modifiers); }
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
    InputPanel {
        id: panel
        objectName: "nativeKeyboard"
        width: parent.width
        anchors.bottom: parent.bottom
    }
}
