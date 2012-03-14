#ifndef _TRACKPAD_H_
#define _TRACKPAD_H_

#include <wdm.h>
#include <wdf.h>

#include "mamaku_common.h"
#include "../inc/katata_common.h"

typedef struct
{
    ULONG   DragThreshold;

    BYTE    State;

    int     Id;

    int     X;

    int     Y;

} MAMAKU_TRACKPAD, *PMAMAKU_TRACKPAD;

#define MAMAKU_TP_IDLE              0
#define MAMAKU_TP_TOUCHDOWN         1
#define MAMAKU_TP_TOUCHUP           2
#define MAMAKU_TP_DRAG_OR_CLICK     3
#define MAMAKU_TP_DRAG              4
#define MAMAKU_TP_CLICK             5

VOID MamakuTrackpadInit(IN PMAMAKU_TRACKPAD tp, IN ULONG dragThreshold);

BOOLEAN MamakuTrackpadProcessTouch(IN PMAMAKU_TRACKPAD tp, IN PMamakuTrackpadData tp_data, OUT KatataRelativeMouseReport* report);

#endif
