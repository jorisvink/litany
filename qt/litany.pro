QT		+= widgets network

TEMPLATE	= app
TARGET		= litany
OBJECTS_DIR	= build/obj
MOC_DIR		= build/moc

INCLUDEPATH	+=	include /usr/local/include
LIBPATH		+=	/usr/local/lib /opt/homebrew/Cellar/libsodium/1.0.20/lib
LIBS		+=	-lkyrka -lsodium

HEADERS		+=	include/litany.h \
			include/tunnel.h \
			include/liturgy.h \
			include/group_chat.h \
			include/peer.h \
			include/peer_chat.h \
			include/window.h \

SOURCES		+=	src/main.cc \
			src/tunnel.cc \
			src/liturgy.cc \
			src/group_chat.cc \
			src/peer_chat.cc \
			src/litany.cc \
			src/msg.c

QMAKE_CXXFLAGS	+=	-g

litany.path	= /usr/local/bin
litany.files	= litany

INSTALLS	+= litany

sanitize {
	CONFIG+= sanitizer sanitize_address sanitize_undefined
}

requires(qtConfig(udpsocket))
