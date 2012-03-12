#ifndef _TRACE_H_
#define _TRACE_H_

//
// Helper macros
//

#define DRIVERNAME                 "mamaku.sys: "

#define DEBUG_LEVEL_ERROR   1
#define DEBUG_LEVEL_BLARG   2
#define DEBUG_LEVEL_INFO    3
#define DEBUG_LEVEL_VERBOSE 4

#define DBG_INIT  1
#define DBG_PNP   2
#define DBG_IOCTL 4

#if DBG
#define MamakuPrint(dbglevel, dbgcatagory, fmt, ...) {          \
    if (MamakuDebugLevel >= dbglevel &&                         \
        (MamakuDebugCatagories && dbgcatagory))                 \
    {                                                           \
        DbgPrint(DRIVERNAME);                                   \
        DbgPrint(fmt, __VA_ARGS__);                             \
    }                                                           \
}
#else
#define MamakuPrint(dbglevel, fmt, ...) {                       \
}
#endif

//
// Globals
//

static ULONG MamakuDebugLevel      = 100;//DEBUG_LEVEL_BLARG;
static ULONG MamakuDebugCatagories = DBG_INIT || DBG_PNP || DBG_IOCTL;

#endif
