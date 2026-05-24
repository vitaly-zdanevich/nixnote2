message("Out path: $${OUT_PWD}")

lessThan(QT_MAJOR_VERSION, 6) {
    error("NixNote tests now require Qt 6; use qmake6 to build them.")
}

QT += core widgets printsupport sql network xml dbus qml testlib

INCLUDEPATH += ..

CONFIG += link_pkgconfig
PKGCONFIG += tidy

# -g flag needed for linker - https://stackoverflow.com/questions/5244509/no-debugging-symbols-found-when-using-gdb
LIBS += -g

win32:PKGCONFIG = ""
win32:LIBS += -L"../winlib" -ltidy
win32:INCLUDEPATH += "../winlib/includes"
win32:QMAKE_LFLAGS += -fstack-protector-strong

TARGET = tests
TEMPLATE = app

SOURCES += tests.cpp \
           ../src/html/enmlformatter.cpp \
           ../src/html/htmldom.cpp \
           ../src/logger/qslog.cpp \
           ../src/logger/qslogdest.cpp \
           ../src/logger/qsdebugoutput.cpp \
           ../src/utilities/NixnoteStringUtils.cpp \
           ../src/utilities/encrypt.cpp

HEADERS += tests.h \
           ../src/html/enmlformatter.h \
           ../src/html/htmldom.h \
           ../src/logger/qslog.h \
           ../src/logger/qslogdest.h \
           ../src/logger/qsdebugoutput.h \
           ../src/utilities/NixnoteStringUtils.h \
           ../src/utilities/encrypt.h

CONFIG(debug, debug|release) {
    DESTDIR = qmake-build-debug-t
    message($$TARGET: Debug build!)
} else {
    DESTDIR = qmake-build-release-t
    message($$TARGET: Release build!)
}
OBJECTS_DIR = $${DESTDIR}
MOC_DIR = $${DESTDIR}


# get g++ version
gcc {
    COMPILER_VERSION = $$system($$QMAKE_CXX " -dumpversion")
    COMPILER_MAJOR_VERSION1 = $$split(COMPILER_VERSION, ".")
    COMPILER_MAJOR_VERSION = $$first(COMPILER_MAJOR_VERSION1)
    message("$$TARGET: Compiler version $$COMPILER_MAJOR_VERSION")
    COMPILER_CONFIG = g++$$COMPILER_MAJOR_VERSION
    message("$$TARGET: Adding compiler config $$COMPILER_CONFIG")
    CONFIG += $$COMPILER_CONFIG
}

linux:QMAKE_CXXFLAGS += -std=c++17 -g -O2  -Wformat -Werror=format-security
linux:QMAKE_LFLAGS += -Wl,-Bsymbolic-functions -Wl,-z,relro

g++4 {
  # this is a guess, but "stack-protector-strong" may not be available yet
  QMAKE_CXXFLAGS += -fstack-protector
} else {
  QMAKE_CXXFLAGS += -fstack-protector-strong
}


isEmpty(PREFIX) {
 PREFIX = /usr
}


# install
target.path = $${PREFIX}/abcd
INSTALLS += target
