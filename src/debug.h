#ifndef XPD_DEBUG_H
#define XPD_DEBUG_H

extern int xpdDebugLevel;

#define debug(...) { if(xpdDebugLevel > 0) printf("[XPD] "__VA_ARGS__); }

#define FUNC_START debug("CALL %s\n", __PRETTY_FUNCTION__);

#endif
