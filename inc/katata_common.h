#if !defined(_KATATA_COMMON_H_)
#define _KATATA_COMMON_H_

#define REPORTID_MTOUCH            1
#define REPORTID_FEATURE           2
#define REPORTID_MOUSE             3

//
// Multitouch specific report information
//

#define MULTI_TIPSWITCH_BIT    1
#define MULTI_IN_RANGE_BIT     2
#define MULTI_CONFIDENCE_BIT   4

#define MULTI_MIN_COORDINATE   0x0000
#define MULTI_MAX_COORDINATE   0x7FFF

#define MULTI_MAX_COUNT        20

#pragma pack(1)
typedef struct
{

    BYTE      Status;

    BYTE      ContactID;

    USHORT    XValue;

    USHORT    YValue;

    USHORT    Width;

    USHORT    Height;

}
KatataTouch, *PKatataTouch;

typedef struct _KATATA_MULTITOUCH_REPORT
{

    BYTE        ReportID;

    KatataTouch Touch[2];

    BYTE        ActualCount;

} KatataMultiTouchReport;
#pragma pack()

//
// Relative mouse specific report information
//

#define MOUSE_BUTTON_1     0x01
#define MOUSE_BUTTON_2     0x02
#define MOUSE_BUTTON_3     0x04

#define MOUSE_MIN_COORDINATE   -127
#define MOUSE_MAX_COORDINATE   127

#define MIN_WHEEL_POS   -127
#define MAX_WHEEL_POS    127

#pragma pack(1)
typedef struct _KATATA_RELATIVE_MOUSE_REPORT
{

    BYTE        ReportID;

    BYTE        Button;

    BYTE        XValue;

    BYTE        YValue;

    BYTE        WheelPosition;

} KatataRelativeMouseReport;
#pragma pack()

#endif
