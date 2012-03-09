#include "trackpad.h"

VOID
MamakuTrackpadInit(
    IN PMAMAKU_TRACKPAD tp
    )
{
    tp->State = MAMAKU_TP_IDLE;
}


BOOLEAN
MamakuTrackpadProcessTouch(
    IN PMAMAKU_TRACKPAD tp,
    IN KatataTouch* touch,
    OUT KatataRelativeMouseReport* report
    )
{
    switch (tp->State)
    {
        case MAMAKU_TP_IDLE:
            if ((touch->Status & MULTI_TIPSWITCH_BIT) == MULTI_TIPSWITCH_BIT)
            {
                tp->State = MAMAKU_TP_TOUCHDOWN;
                tp->X = touch->XValue;
                tp->Y = touch->YValue;
            }
            break;

        case MAMAKU_TP_TOUCHDOWN:
            if ((touch->Status & MULTI_TIPSWITCH_BIT) == MULTI_TIPSWITCH_BIT)
            {
                tp->State = MAMAKU_TP_DRAG;

                // fall through case
            }
            else
            {
                tp->State = MAMAKU_TP_IDLE;
                break;
            }

        case MAMAKU_TP_DRAG:
            if ((touch->Status & MULTI_TIPSWITCH_BIT) == MULTI_TIPSWITCH_BIT)
            {
                char dx = touch->XValue - tp->X;
                char dy = touch->YValue - tp->Y;
                tp->X = touch->XValue;
                tp->Y = touch->YValue;

                report->ReportID = REPORTID_MOUSE;
                report->Button = 0; //TODO: for now
                report->XValue = dx;
                report->YValue = dy;
                report->WheelPosition = 0; //TODO: for now

                return TRUE;
            }
            else
            {
                tp->State = MAMAKU_TP_IDLE;
            }
            break;
    }

    return FALSE;
}
