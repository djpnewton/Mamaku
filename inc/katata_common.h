#if !defined(_KATATA_COMMON_H_)
#define _KATATA_COMMON_H_

#define REPORTID_MTOUCH            1
#define REPORTID_FEATURE           2

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

typedef struct _VMULTI_MULTITOUCH_REPORT
{

    BYTE        ReportID;

    KatataTouch Touch[2];

    BYTE        ActualCount;

} KatataMultiTouchReport;
#pragma pack()

#endif
