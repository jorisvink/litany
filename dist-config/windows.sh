#!/bin/sh
#
# Configure litany for windows on my build machines.

KYRKA=/home/joris/src/libkyrka/x86_64-w64-mingw32.static-gcc \
    /home/joris/src/mxe/usr/x86_64-w64-mingw32.static/qt6/bin/qmake6 \
    qt/litany.pro CONFIG+=windows
