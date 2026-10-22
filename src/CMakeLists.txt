# SPDX-FileCopyrightText: 2024 Aleix Pol <aleixpol@kde.org>
# SPDX-FileCopyrightText: 2025 Devin Lin <devin@kde.org>
# SPDX-FileCopyrightText: 2025 Kristen McWilliam <kristen@kde.org>
#
# SPDX-License-Identifier: BSD-2-Clause

configure_file(config-plasma-keyboard.h.cmake ${CMAKE_CURRENT_BINARY_DIR}/config-plasma-keyboard.h)

add_library(sonickeyboardui SHARED)

target_sources(sonickeyboardui PRIVATE
    inputpanelintegration.cpp
    inputpanelintegration.h
    inputpanelrole.h
    nativeinputlisteneritem.cpp
    inputlisteneritem.h
    nativeinputmethod.cpp
    nativeinputmethod.h
    inputpanelwindow.cpp
    inputpanelwindow.h
    nativeinputplugin.cpp
    inputplugin.h
    overlay/overlaycontroller.cpp
    overlay/overlaycontroller.h
    overlay/candidatemodel.cpp
    overlay/candidatemodel.h
    overlay/overlaytrigger.cpp
    overlay/overlaytrigger.h
    overlay/longpresstrigger.cpp
    overlay/longpresstrigger.h
    overlay/diacriticsdataloader.cpp
    overlay/diacriticsdataloader.h
    overlay/diacritics.qrc
    overlay/prefixquerytrigger.cpp
    overlay/prefixquerytrigger.h
    overlay/textexpansiontrigger.cpp
    overlay/textexpansiontrigger.h
)

if(PLASMA_KEYBOARD_VIBRATION_ENABLED)
    set_source_files_properties(dbus/org.sigxcpu.Feedback.Haptic.xml PROPERTIES INCLUDE vibrationevent.h)
    qt_add_dbus_interfaces(dbusinterface_SRCS
        dbus/org.sigxcpu.Feedback.Haptic.xml)

    target_sources(sonickeyboardui PRIVATE
        vibration.cpp
        vibration.h
        vibrationevent.h
        ${dbusinterface_SRCS}
    )
endif()

if(PLASMA_KEYBOARD_SOUND_ENABLED)
    target_sources(sonickeyboardui PRIVATE
        soundresources.qrc
    )
endif()

ecm_qt_declare_logging_category(sonickeyboardui
    HEADER logging.h
    IDENTIFIER "PlasmaKeyboard"
    CATEGORY_NAME "org.kde.plasma.keyboard"
    DESCRIPTION "Plasma Keyboard"
    DEFAULT_SEVERITY Warning
)

ecm_add_qml_module(sonickeyboardui
    URI "org.kde.plasma.keyboard"
    GENERATE_PLUGIN_SOURCE
    DEPENDENCIES
    QtQuick
)

ecm_target_qml_sources(sonickeyboardui SOURCES
    qml/main.qml
    qml/LanguagePopup.qml
    qml/LanguagePopupDelegate.qml
    qml/DiacriticsOverlay.qml
    qml/OverlayWindow.qml
)
ecm_finalize_qml_module(sonickeyboardui DESTINATION ${KDE_INSTALL_QMLDIR})

kconfig_add_kcfg_files(sonickeyboardui GENERATE_MOC plasmakeyboardsettings.kcfgc)

target_include_directories(sonickeyboardui PRIVATE
    ${CMAKE_BINARY_DIR}
    # Add directories to include path for QML type registration
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/overlay
)

target_link_libraries(sonickeyboardui
    PRIVATE
    KF6::CoreAddons
    KF6::I18n
    KF6::I18nQml
    KF6::ConfigCore
    KF6::ConfigGui
    KF6::Crash
    PkgConfig::XKBCommon
    Plasma::PlasmaQuick
    Qt::DBus
    Qt::Gui
    Qt::GuiPrivate
    Qt::VirtualKeyboard
)

add_executable(plasma-keyboard main.cpp)
target_include_directories(plasma-keyboard PRIVATE ${CMAKE_BINARY_DIR} ${CMAKE_CURRENT_BINARY_DIR})
target_link_libraries(plasma-keyboard PRIVATE Qt::Gui Qt::Qml KF6::ConfigCore KF6::CoreAddons KF6::I18n KF6::I18nQml KF6::Crash)
install(TARGETS sonickeyboardui ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(TARGETS plasma-keyboard ${KDE_INSTALL_TARGETS_DEFAULT_ARGS})
install(PROGRAMS org.kde.plasma.keyboard.desktop DESTINATION ${KDE_INSTALL_APPDIR})

add_subdirectory(styles)
add_subdirectory(qmlplugin)

add_library(sonicvirtualkeyboardplugin MODULE qtkeyboardinputcontext.cpp)
target_link_libraries(sonicvirtualkeyboardplugin PRIVATE Qt::GuiPrivate Qt::Qml Qt::VirtualKeyboardPrivate KF6::I18nQml)
target_compile_options(sonicvirtualkeyboardplugin PRIVATE $<$<CXX_COMPILER_ID:GNU,Clang>:-UQT_NO_KEYWORDS>)
set_target_properties(sonicvirtualkeyboardplugin PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/plugins/platforminputcontexts")
add_dependencies(sonicvirtualkeyboardplugin sonickeyboarduiplugin breezestyle keyboardlib)
add_dependencies(plasma-keyboard sonicvirtualkeyboardplugin)
install(TARGETS sonicvirtualkeyboardplugin DESTINATION ${KDE_INSTALL_QTPLUGINDIR}/platforminputcontexts)

install(DIRECTORY layouts DESTINATION ${CMAKE_INSTALL_PREFIX}/share/plasma/keyboard)
install(DIRECTORY overlay/diacritics DESTINATION ${CMAKE_INSTALL_PREFIX}/share/plasma/keyboard)

install(FILES plasmakeyboardsettings.kcfg DESTINATION ${KDE_INSTALL_KCFGDIR})

install(
    FILES ${CMAKE_CURRENT_SOURCE_DIR}/org.kde.plasma.keyboard.metainfo.xml
    DESTINATION ${KDE_INSTALL_METAINFODIR}
)
