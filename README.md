<!--
  - SPDX-FileCopyrightText: None
  - SPDX-License-Identifier: CC0-1.0
-->

# Sonic Keyboard

Sonic Keyboard combines this project's existing keyboard UI, theme support, and layouts with [Qt Virtual Keyboard](https://doc.qt.io/qt-6/qtvirtualkeyboard-overview.html).

The input engine is Qt's actual `QtVirtualKeyboard::PlatformInputContext`. The X11 frontend adapter supplies the original `src/qml/main.qml` desktop window. Composition, Unicode, focus, input hints, and text delivery remain handled by Qt. Wayland protocols and Qt Wayland development libraries are not required.

## Application integration

Select the replacement through the standard Qt input-method setting, before constructing the application:

```cpp
qputenv("QT_IM_MODULE", "sonicvirtualkeyboard");
QGuiApplication app(argc, argv);
```

Applications continue using the existing Qt API, including `QGuiApplication::inputMethod()->show()`, `hide()`, visibility notifications, and `QtQuick.VirtualKeyboard.InputPanel`. No Sonic-specific initialization calls or embedding headers are required. The plugin also accepts the `qtvirtualkeyboard` key when its plugin directory is preferred in Qt's plugin search path; the explicit `sonicvirtualkeyboard` key avoids ambiguity when the distribution's Qt plugin is also installed.

Leave `QT_VIRTUALKEYBOARD_DESKTOP_DISABLE` unset for the original desktop keyboard window. Set it to `1` when the application supplies its own `InputPanel`, exactly as with Qt Virtual Keyboard. Explicit `QT_VIRTUALKEYBOARD_STYLE` and `QT_VIRTUALKEYBOARD_LAYOUT_PATH` settings are preserved; otherwise the Breeze frontend and installed bundled layouts are selected.

Colors and fonts come from the application's applied Kirigami theme. Login-manager therefore uses the theme copied to the greeter account, while screenlocker uses the logged-in user's theme. The existing shared footer API is unchanged.

## Development

Build against matching Qt 6.11 or newer public/private development packages and the SonicDE providers:

```sh
cmake -S . -B build -G Ninja -DBUILD_TESTING=ON
cmake --build build --parallel 8
ctest --test-dir build --output-on-failure
```

For build-tree testing, add `build/plugins` to `QT_PLUGIN_PATH` and `build/bin` to `QML_IMPORT_PATH`, alongside the paths for the staged SonicDE dependencies. X11 integration tests use a private Xvfb display and require `xvfb-run` and `dbus-run-session`. The adapter uses a Qt private frontend hook, so rebuild it when updating Qt's private development ABI.

## Layouts

The keyboard layouts are located in the [src/layouts](/src/layouts) folder.

They are forked from Qt's [layouts](https://github.com/qt/qtvirtualkeyboard/tree/dev/src/layouts), with modifications that we want for Plasma. Please view the official [Qt documentation](https://doc.qt.io/qt-6/qtvirtualkeyboard-overview.html#adding-new-keyboard-layouts) for a guide on how to create and modify keyboard layouts.

To use Qt's built-in keyboard layouts rather than the bundled ones, set `PLASMA_KEYBOARD_USE_QT_LAYOUTS=1` when starting the Qt application.

## Troubleshooting

If the input context is unavailable, check that the matching Qt Virtual Keyboard runtime and the replacement plugin are in Qt's plugin search path. The login/lock-screen footer controls activation through Qt's usual input-method API.
