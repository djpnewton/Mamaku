#include "trackpad.h"

VOID
MamakuTrackpadInit(
    IN PMAMAKU_TRACKPAD tp,
    IN ULONG dragThreshold
    )
{
    tp->DragThreshold = dragThreshold;
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
                tp->Id = tp_data->id;
                tp->X = tp_data->x;
                tp->Y = tp_data->y;
            }
            break;

        case MAMAKU_TP_TOUCHDOWN:
            if (tp->Id != tp_data->id)
            {
                break;
            }
            if (tp_data->state)
            {
                tp->State = MAMAKU_TP_DRAG_OR_CLICK;

                // fall through case
            }
            else
            {
                tp->State = MAMAKU_TP_IDLE;
                break;
            }

        case MAMAKU_TP_DRAG_OR_CLICK:
            if (tp->Id != tp_data->id)
            {
                break;
            }
            if (tp_data->state & 0x50)
            {
                char dx = (tp_data->x - tp->X);
                char dy = (tp_data->y - tp->Y);
                if ((ULONG)abs(dx) > tp->DragThreshold || (ULONG)abs(dy) > tp->DragThreshold)
                {
                    tp->State = MAMAKU_TP_DRAG;

                    // fall through case
                }
            }
            else if (tp_data->state == 0x70)
            {
                tp->State = MAMAKU_TP_CLICK;

                // send mouse button down
                report->ReportID = REPORTID_MOUSE;
                report->Button = 1;
                report->XValue = 0;
                report->YValue = 0;
                report->WheelPosition = 0;

                return TRUE;
            }
            break;
        case MAMAKU_TP_DRAG:
            if (tp->Id != tp_data->id)
            {
                break;
            }
            if (tp_data->state)
            {
                char dx = (tp_data->x - tp->X);
                char dy = (tp_data->y - tp->Y);

                tp->X = tp_data->x;
                tp->Y = tp_data->y;

                // send mouse move
                report->ReportID = REPORTID_MOUSE;
                report->Button = 0;
                report->XValue = dx;
                report->YValue = dy;
                report->WheelPosition = 0;

                return TRUE;
            }
            break;
        case MAMAKU_TP_CLICK:
            tp->State = MAMAKU_TP_IDLE;

            // send mouse button up
            report->ReportID = REPORTID_MOUSE;
            report->Button = 0;
            report->XValue = 0;
            report->YValue = 0;
            report->WheelPosition = 0;

            return TRUE;
    }

    return FALSE;
}
