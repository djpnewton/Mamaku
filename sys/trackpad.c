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
    IN PMamakuTrackpadData tp_data,
    OUT KatataRelativeMouseReport* report
    )
{
    switch (tp->State)
    {
        case MAMAKU_TP_IDLE:
            if (tp_data->state)
            {
                tp->State = MAMAKU_TP_TOUCHDOWN;
                tp->X = tp_data->x;
                tp->Y = tp_data->y;
            }
            break;

        case MAMAKU_TP_TOUCHDOWN:
            if (tp_data->state)
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
            if (tp_data->state)
            {
                char dx = (tp_data->x - tp->X);
                char dy = (tp_data->y - tp->Y);
                tp->X = tp_data->x;
                tp->Y = tp_data->y;

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
