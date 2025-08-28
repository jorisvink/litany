QT		+= widgets network

TEMPLATE	= app
TARGET		= litany
OBJECTS_DIR	= build/obj
MOC_DIR		= build/moc

INCLUDEPATH	+= include
KYRKA		= $$getenv("KYRKA")

isEmpty(KYRKA) {
	LIBS += -L /usr/local/lib -lkyrka
	INCLUDEPATH += /usr/local/include
} else {
	LIBS += $$(KYRKA)/lib/libkyrka.a
	INCLUDEPATH += $$(KYRKA)/include
	message("Using $(KYRKA) for libkyrka")
}

LIBS += -lsodium

HEADERS+=	include/litany.h \
		include/chat.h \
		include/tunnel.h \
		include/liturgy.h \
		include/group.h \
		include/peer.h \
		include/settings.h \
		include/window.h \

SOURCES +=	src/main.cc \
		src/chat.cc \
		src/tunnel.cc \
		src/litany.cc \
		src/liturgy.cc \
		src/group.cc \
		src/peer.cc \
		src/settings.cc \
		src/msg.c \
		src/utf8.c

QMAKE_CXXFLAGS	+=	-g

litany.path	= /usr/local/bin
litany.files	= litany
INSTALLS	+= litany

windows {
	QMAKE_CFLAGS += -DPLATFORM_WINDOWS
	QMAKE_CXXFLAGS += -DPLATFORM_WINDOWS
}

macos {
	LIBPATH += /opt/homebrew/Cellar/libsodium/1.0.20/lib
}

sanitize {
	CONFIG+= sanitizer sanitize_address sanitize_undefined
}

requires(qtConfig(udpsocket))
