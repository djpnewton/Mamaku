#ifndef _TRACKPAD_H_
#define _TRACKPAD_H_

#include <wdm.h>
#include <wdf.h>

#include "../inc/katata_common.h"

typedef struct
{
    BYTE    State;

    USHORT  X;

    USHORT  Y;

} MAMAKU_TRACKPAD, *PMAMAKU_TRACKPAD;

#define MAMAKU_TP_IDLE      0
#define MAMAKU_TP_TOUCHDOWN 1
#define MAMAKU_TP_DRAG      2
#define MAMAKU_TP_TOUCHUP   2

VOID MamakuTrackpadInit(IN PMAMAKU_TRACKPAD tp);

BOOLEAN MamakuTrackpadProcessTouch(IN PMAMAKU_TRACKPAD tp, IN KatataTouch* touch, OUT KatataRelativeMouseReport* report);

#endif
