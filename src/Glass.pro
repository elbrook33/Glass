TEMPLATE     = app
#QMAKE_CXX    = clang++
#QMAKE_LINK   = clang++
CONFIG      += console
CONFIG      -= app_bundle
CONFIG      -= qt
CONFIG      += c++11
CONFIG      += link_pkgconfig
PKGCONFIG   += x11
PKGCONFIG   += xcomposite
PKGCONFIG   += xfixes
PKGCONFIG   += xdamage
PKGCONFIG   += xext
PKGCONFIG   += gl
PKGCONFIG   += glew
LIBS        += -pthread
LIBS        += -lSOIL

SOURCES     += main.cpp \
               xpp.cpp \
               draw.cpp \
               UVRect.cpp

HEADERS     += \
               xpp.h \
               draw.h \
               shaderProgram.h \
               shaders.h \
               blur.h \
               UVRect.h

OTHER_FILES += \
               Progress.txt \
             ../rsrc/*.png

final.path   = /opt/Glass
final.files  = Glass \
             ../rsrc/*.png

INSTALLS    += final
