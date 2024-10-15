QT += core
QT += gui
QT += widgets
QT += network
QT += charts
QT += printsupport
QT += concurrent
QT += serialbus
QT += serialport
QT += bluetooth
QT += location
QT += dbus
QT += xml

###
TEMPLATE = app

###
CONFIG += c++17
CONFIG += sdk_no_version_check
CONFIG += nostrip
CONFIG += debug
#CONFIG += lrelease
CONFIG += embed_translations
CONFIG += create_prl
CONFIG += app_bundle

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
	cschargerdatamodel.cpp \
	csvedirectacdccharger.cpp \
	main.cpp \
	csvedirect.cpp \
	mainwindow.cpp

HEADERS += \
	cschargerdatamodel.h \
	csvedirect.h \
	csvedirectacdccharger.h \
	mainwindow.h

FORMS += \
	mainwindow.ui

# Default rules for deployment.
# $${TARGET}
target.path = /usr/local/bin
INSTALLS += target
