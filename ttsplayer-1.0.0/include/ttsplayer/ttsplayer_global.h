#ifndef TTSPLAYER_GLOBAL_H
#define TTSPLAYER_GLOBAL_H

#include <QtCore/qglobal.h>

#if defined(TTSPLAYER_LIBRARY)
#  define TTSPLAYER_EXPORT Q_DECL_EXPORT
#else
#  define TTSPLAYER_EXPORT Q_DECL_IMPORT
#endif

#endif
