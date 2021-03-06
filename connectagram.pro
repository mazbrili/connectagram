lessThan(QT_MAJOR_VERSION, 5) {
	error("Connectagram requires Qt 5.9 or greater")
}
equals(QT_MAJOR_VERSION, 5):lessThan(QT_MINOR_VERSION, 9) {
	error("Connectagram requires Qt 5.9 or greater")
}

TEMPLATE = app
QT += network widgets
CONFIG += c++11

CONFIG(debug, debug|release) {
	CONFIG += warn_on
	DEFINES += QT_DEPRECATED_WARNINGS
	DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x051400
	DEFINES += QT_NO_NARROWING_CONVERSIONS_IN_CONNECT
}

# Allow in-tree builds
MOC_DIR = build
OBJECTS_DIR = build
RCC_DIR = build

# Set program version
VERSION = 1.2.11
DEFINES += VERSIONSTR=\\\"$${VERSION}\\\"

# Set program name
unix: !macx {
	TARGET = connectagram
} else {
	TARGET = Connectagram
}

# Specify program sources
HEADERS = src/board.h \
	src/cell.h \
	src/clock.h \
	src/definitions.h \
	src/dictionary.h \
	src/letter.h \
	src/locale_dialog.h \
	src/new_game_dialog.h \
	src/pattern.h \
	src/score_board.h \
	src/view.h \
	src/window.h \
	src/word.h \
	src/wordlist.h

SOURCES = src/board.cpp \
	src/cell.cpp \
	src/clock.cpp \
	src/definitions.cpp \
	src/dictionary.cpp \
	src/letter.cpp \
	src/locale_dialog.cpp \
	src/main.cpp \
	src/new_game_dialog.cpp \
	src/pattern.cpp \
	src/score_board.cpp \
	src/view.cpp \
	src/window.cpp \
	src/word.cpp \
	src/wordlist.cpp

# Generate translations
TRANSLATIONS = $$files(translations/connectagram_*.ts)
qtPrepareTool(LRELEASE, lrelease)
updateqm.input = TRANSLATIONS
updateqm.output = ${QMAKE_FILE_PATH}/${QMAKE_FILE_BASE}.qm
updateqm.commands = $$LRELEASE -silent ${QMAKE_FILE_IN} -qm ${QMAKE_FILE_OUT}
updateqm.CONFIG += no_link target_predeps
QMAKE_EXTRA_COMPILERS += updateqm

# Install program data
RESOURCES = icons/icons.qrc
macx {
	ICON = icons/connectagram.icns

	GAME_DATA.files = data
	GAME_DATA.path = Contents/Resources

	QMAKE_BUNDLE_DATA += GAME_DATA
} else:win32 {
	RC_ICONS = icons/connectagram.ico
	QMAKE_TARGET_DESCRIPTION = "Anagram game"
	QMAKE_TARGET_COPYRIGHT = "Copyright (C) 2019 Graeme Gott"
} else:unix {
	RESOURCES += icons/icon.qrc

	isEmpty(PREFIX) {
		PREFIX = /usr/local
	}
	isEmpty(BINDIR) {
		BINDIR = bin
	}

	target.path = $$PREFIX/$$BINDIR/

	data.files = data
	data.path = $$PREFIX/share/connectagram/

	pixmap.files = icons/connectagram.xpm
	pixmap.path = $$PREFIX/share/pixmaps/

	icon.files = icons/hicolor/*
	icon.path = $$PREFIX/share/icons/hicolor/

	desktop.files = icons/connectagram.desktop
	desktop.path = $$PREFIX/share/applications/

	appdata.files = icons/connectagram.appdata.xml
	appdata.path = $$PREFIX/share/metainfo/

	qm.files = $$replace(TRANSLATIONS, .ts, .qm)
	qm.path = $$PREFIX/share/connectagram/translations
	qm.CONFIG += no_check_exist

	man.files = doc/connectagram.6
	man.path = $$PREFIX/share/man/man6

	INSTALLS += target pixmap icon desktop appdata data qm man
}
