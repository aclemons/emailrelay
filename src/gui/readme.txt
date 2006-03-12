Windows build:
* edit "mingw-common.mak" to set "mk_bin" and "mk_qt"
* "make -f mingw.mak"

Unix/Linux build:
* eg: "./configure e_qtdir=/usr/local/qt4 e_x11dir=/usr/X11R6 --enable-gui"

Mac OSX build:
* eg: "make -f mac.mak e_qtdir=/usr/local/Trolltech/Qt-4.1.0"
