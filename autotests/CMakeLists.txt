# SPDX-FileCopyrightText: 2026 Aleix Pol <aleixpol@kde.org>
# SPDX-License-Identifier: BSD-2-Clause

find_package(Qt6 ${QT_MIN_VERSION} REQUIRED COMPONENTS Test)
include(ECMAddTests)

add_executable(nativekeyboardtest nativekeyboardtest.cpp ../src/nativeinputmethod.cpp ../src/nativeinputmethod.h)
target_include_directories(nativekeyboardtest PRIVATE ${CMAKE_SOURCE_DIR}/src)
target_link_libraries(nativekeyboardtest PRIVATE Qt::Test Qt::Quick Qt::GuiPrivate Qt::VirtualKeyboard)
target_compile_definitions(nativekeyboardtest PRIVATE
    KEYBOARD_TEST_QML="${CMAKE_CURRENT_SOURCE_DIR}/nativekeyboard.qml"
    KEYBOARD_DESKTOP_TEST_QML="${CMAKE_CURRENT_SOURCE_DIR}/desktopkeyboard.qml"
    KEYBOARD_TEST_IMPORT_PATH="${CMAKE_BINARY_DIR}/bin"
    KEYBOARD_TEST_PLUGIN_PATH="${CMAKE_BINARY_DIR}/plugins")
target_compile_definitions(nativekeyboardtest PRIVATE KEYBOARD_TEST_LAYOUTS="${CMAKE_SOURCE_DIR}/src/layouts")
add_dependencies(nativekeyboardtest sonicvirtualkeyboardplugin sonickeyboarduiplugin breezestyle keyboardlib)
find_program(XVFB_RUN_EXECUTABLE xvfb-run)
find_program(DBUS_RUN_SESSION_EXECUTABLE dbus-run-session)
if(XVFB_RUN_EXECUTABLE AND DBUS_RUN_SESSION_EXECUTABLE)
    add_test(NAME nativekeyboardtest COMMAND ${XVFB_RUN_EXECUTABLE} -a -s "-screen 0 1280x900x24"
        ${DBUS_RUN_SESSION_EXECUTABLE} -- ${CMAKE_COMMAND} -E env
        QT_QPA_PLATFORM=xcb SONIC_KEYBOARD_TEST_ISOLATED=1 $<TARGET_FILE:nativekeyboardtest>)
    add_test(NAME desktopkeyboardapi COMMAND ${XVFB_RUN_EXECUTABLE} -a -s "-screen 0 1280x900x24"
        ${DBUS_RUN_SESSION_EXECUTABLE} -- ${CMAKE_COMMAND} -E env
        QT_QPA_PLATFORM=xcb SONIC_KEYBOARD_TEST_ISOLATED=1 SONIC_KEYBOARD_TEST_DESKTOP=1
        SONIC_KEYBOARD_TEST_MODULE=sonicvirtualkeyboard $<TARGET_FILE:nativekeyboardtest>)
    set_tests_properties(desktopkeyboardapi PROPERTIES TIMEOUT 90)
    add_test(NAME qtkeyboardstyleoverride COMMAND ${XVFB_RUN_EXECUTABLE} -a -s "-screen 0 1280x900x24"
        ${DBUS_RUN_SESSION_EXECUTABLE} -- ${CMAKE_COMMAND} -E env
        QT_QPA_PLATFORM=xcb SONIC_KEYBOARD_TEST_ISOLATED=1 SONIC_KEYBOARD_TEST_DESKTOP=1
        SONIC_KEYBOARD_TEST_MODULE=sonicvirtualkeyboard SONIC_KEYBOARD_TEST_STYLE=default
        $<TARGET_FILE:nativekeyboardtest> standardInputMethodApi nativeUnicodeAndKeysKeepEditorFocus
        styledKeyClickUsesQtBackend passwordHintsAndOverlayPrivacy)
    set_tests_properties(qtkeyboardstyleoverride PROPERTIES TIMEOUT 90)
else()
    add_test(NAME nativekeyboardtest COMMAND nativekeyboardtest)
    set_tests_properties(nativekeyboardtest PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
set_tests_properties(nativekeyboardtest PROPERTIES TIMEOUT 60)
