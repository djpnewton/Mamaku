#if !defined(_KATATA_H_)
#define _KATATA_H_

#pragma warning(disable:4200)  // suppress nameless struct/union warning
#pragma warning(disable:4201)  // suppress nameless struct/union warning
#pragma warning(disable:4214)  // suppress bit field types other than int warning
#include <initguid.h>
#include <wdm.h>

#pragma warning(default:4200)
#pragma warning(default:4201)
#pragma warning(default:4214)
#include <wdf.h>

#pragma warning(disable:4201)  // suppress nameless struct/union warning
#pragma warning(disable:4214)  // suppress bit field types other than int warning
#include <hidport.h>

#include "../inc/katata_common.h"

//
// String definitions
//

#define DRIVERNAME                 "katata.sys: "

#define VMULTI_POOL_TAG            (ULONG) 'atak'

//
//These are the device attributes returned by vmulti in response
// to IOCTL_HID_GET_DEVICE_ATTRIBUTES.
//

#define KATATA_PID              0xBACC
#define KATATA_VID              0x20FF
#define KATATA_VERSION          0x0001


#define MT_TOUCH_COLLECTION                                                    \
    0xa1, 0x02,                         /*     COLLECTION (Logical)         */ \
    0x09, 0x42,                         /*       USAGE (Tip Switch)         */ \
    0x15, 0x00,                         /*       LOGICAL_MINIMUM (0)        */ \
    0x25, 0x01,                         /*       LOGICAL_MAXIMUM (1)        */ \
    0x75, 0x01,                         /*       REPORT_SIZE (1)            */ \
    0x95, 0x01,                         /*       REPORT_COUNT (1)           */ \
    0x81, 0x02,                         /*       INPUT (Data,Var,Abs)       */ \
    0x09, 0x32,                         /*       USAGE (In Range)           */ \
    0x81, 0x02,                         /*       INPUT (Data,Var,Abs)       */ \
    0x09, 0x47,                         /*       USAGE (Confidence)         */ \
    0x81, 0x02,                         /*       INPUT (Data,Var,Abs)       */ \
    0x95, 0x05,                         /*       REPORT_COUNT (5)           */ \
    0x81, 0x03,                         /*       INPUT (Cnst,Ary,Abs)       */ \
    0x75, 0x08,                         /*       REPORT_SIZE (8)            */ \
    0x09, 0x51,                         /*       USAGE (Contact Identifier) */ \
    0x95, 0x01,                         /*       REPORT_COUNT (1)           */ \
    0x81, 0x02,                         /*       INPUT (Data,Var,Abs)       */ \
    0x05, 0x01,                         /*       USAGE_PAGE (Generic Desk.. */ \
    0x26, 0xff, 0x7f,                   /*       LOGICAL_MAXIMUM (32767)    */ \
    0x75, 0x10,                         /*       REPORT_SIZE (16)           */ \
    0x55, 0x00,                         /*       UNIT_EXPONENT (0)          */ \
    0x65, 0x00,                         /*       UNIT (None)                */ \
    0x35, 0x00,                         /*       PHYSICAL_MINIMUM (0)       */ \
    0x46, 0x00, 0x00,                   /*       PHYSICAL_MAXIMUM (0)       */ \
    0x09, 0x30,                         /*       USAGE (X)                  */ \
    0x81, 0x02,                         /*       INPUT (Data,Var,Abs)       */ \
    0x09, 0x31,                         /*       USAGE (Y)                  */ \
    0x81, 0x02,                         /*       INPUT (Data,Var,Abs)       */ \
    0x05, 0x0d,                         /*       USAGE PAGE (Digitizers)    */ \
    0x09, 0x48,                         /*       USAGE (Width)              */ \
    0x81, 0x02,                         /*       INPUT (Data,Var,Abs)       */ \
    0x09, 0x49,                         /*       USAGE (Height)             */ \
    0x81, 0x02,                         /*       INPUT (Data,Var,Abs)       */ \
    0xc0,                               /*    END_COLLECTION                */

//
// This is the default report descriptor for the Hid device provided
// by the mini driver in response to IOCTL_HID_GET_REPORT_DESCRIPTOR.
// 

typedef UCHAR HID_REPORT_DESCRIPTOR, *PHID_REPORT_DESCRIPTOR;

HID_REPORT_DESCRIPTOR DefaultReportDescriptor[] = {
//
// Multitouch report starts here
//
    0x05, 0x0d,                         // USAGE_PAGE (Digitizers)
    0x09, 0x04,                         // USAGE (Touch Screen)
    0xa1, 0x01,                         // COLLECTION (Application)
    0x85, REPORTID_MTOUCH,              //   REPORT_ID (Touch)
    0x09, 0x22,                         //   USAGE (Finger)
    MT_TOUCH_COLLECTION
    MT_TOUCH_COLLECTION
    0x05, 0x0d,                         //    USAGE_PAGE (Digitizers)
    0x09, 0x54,                         //    USAGE (Contact Count)
    0x95, 0x01,                         //    REPORT_COUNT (1)
    0x75, 0x08,                         //    REPORT_SIZE (8)
    0x15, 0x00,                         //    LOGICAL_MINIMUM (0)
    0x25, 0x08,                         //    LOGICAL_MAXIMUM (8)
    0x81, 0x02,                         //    INPUT (Data,Var,Abs)
    0x09, 0x55,                         //    USAGE(Contact Count Maximum)
    0xb1, 0x02,                         //    FEATURE (Data,Var,Abs)
    0xc0,                               // END_COLLECTION

//
// Feature report starts here
//
    0x09, 0x0E,                         // USAGE (Device Configuration)
    0xa1, 0x01,                         // COLLECTION (Application)
    0x85, REPORTID_FEATURE,             //   REPORT_ID (Configuration)              
    0x09, 0x23,                         //   USAGE (Device Settings)              
    0xa1, 0x02,                         //   COLLECTION (logical)    
    0x09, 0x52,                         //    USAGE (Device Mode)         
    0x09, 0x53,                         //    USAGE (Device Identifier)
    0x15, 0x00,                         //    LOGICAL_MINIMUM (0)      
    0x25, 0x0a,                         //    LOGICAL_MAXIMUM (10)
    0x75, 0x08,                         //    REPORT_SIZE (8)         
    0x95, 0x02,                         //    REPORT_COUNT (2)         
    0xb1, 0x02,                         //   FEATURE (Data,Var,Abs)    
    0xc0,                               //   END_COLLECTION
    0xc0,                               // END_COLLECTION
};

//
// This is the default HID descriptor returned by the mini driver
// in response to IOCTL_HID_GET_DEVICE_DESCRIPTOR. The size
// of report descriptor is currently the size of DefaultReportDescriptor.
//

CONST HID_DESCRIPTOR DefaultHidDescriptor = {
    0x09,   // length of HID descriptor
    0x21,   // descriptor type == HID  0x21
    0x0100, // hid spec release
    0x00,   // country code == Not Specified
    0x01,   // number of HID class descriptors
    { 0x22,   // descriptor type 
    sizeof(DefaultReportDescriptor) }  // total length of report descriptor
};


typedef struct _KATATA_CONTEXT 
{
    WDFIOTARGET IoTarget;

    BYTE DeviceMode;

} KATATA_CONTEXT, *PKATATA_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(KATATA_CONTEXT, KatataGetDeviceContext)


//
// Feature report infomation
//

#define DEVICE_MODE_MOUSE        0x00
#define DEVICE_MODE_SINGLE_INPUT 0x01
#define DEVICE_MODE_MULTI_INPUT  0x02

#pragma pack(1)
typedef struct _VMULTI_FEATURE_REPORT
{

    BYTE      ReportID;

    BYTE      DeviceMode;

    BYTE      DeviceIdentifier;

} KatataFeatureReport;

typedef struct _VMULTI_MAXCOUNT_REPORT
{

    BYTE         ReportID;

    BYTE         MaximumCount;

} KatataMaxCountReport;
#pragma pack()

//
// Function definitions
//

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_UNLOAD KatataDriverUnload;

EVT_WDF_DRIVER_DEVICE_ADD KatataEvtDeviceAdd;

EVT_WDF_DEVICE_D0_ENTRY KatataD0Entry;

EVT_WDF_DEVICE_D0_EXIT KatataD0Exit;

EVT_WDF_DPC KatataReadDpc;

EVT_WDF_REQUEST_COMPLETION_ROUTINE KatataIoctlComplete;

EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL KatataEvtInternalDeviceControl;

NTSTATUS
KatataGetHidDescriptor(
    IN WDFDEVICE Device,
    IN WDFREQUEST Request
    );

NTSTATUS
KatataGetReportDescriptor(
    IN WDFDEVICE Device,
    IN WDFREQUEST Request
    );

NTSTATUS
KatataGetDeviceAttributes(
    IN WDFREQUEST Request
    );

NTSTATUS
KatataGetString(
    IN WDFREQUEST Request
    );

NTSTATUS
KatataWriteReport(
    IN PKATATA_CONTEXT DevContext,
    IN WDFREQUEST Request
    );

NTSTATUS
KatataProcessVendorReport(
    IN PKATATA_CONTEXT DevContext,
    IN PVOID ReportBuffer,
    IN ULONG ReportBufferLen,
    OUT size_t* BytesWritten
    );

NTSTATUS
KatataReadReport(
    IN PKATATA_CONTEXT DevContext,
    IN WDFREQUEST Request,
    OUT BOOLEAN* CompleteRequest
    );

NTSTATUS
KatataSetFeature(
    IN PKATATA_CONTEXT DevContext,
    IN WDFREQUEST Request,
    OUT BOOLEAN* CompleteRequest
    );

NTSTATUS
KatataGetFeature(
    IN PKATATA_CONTEXT DevContext,
    IN WDFREQUEST Request,
    OUT BOOLEAN* CompleteRequest
    );

PCHAR
DbgHidInternalIoctlString(
    IN ULONG        IoControlCode
    );

//
// Helper macros
//

#define DEBUG_LEVEL_ERROR   1
#define DEBUG_LEVEL_INFO    2
#define DEBUG_LEVEL_VERBOSE 3

#define DBG_INIT  1
#define DBG_PNP   2
#define DBG_READ  4
#define DBG_IOCTL 8

#if DBG
#define KatataPrint(dbglevel, dbgcatagory, fmt, ...) {          \
    if (KatataDebugLevel >= dbglevel &&                         \
        (KatataDebugCatagories && dbgcatagory))                 \
    {                                                           \
        DbgPrint(DRIVERNAME);                                   \
        DbgPrint(fmt, __VA_ARGS__);                             \
    }                                                           \
}
#else
#define KatataPrint(dbglevel, fmt, ...) {                       \
}
#endif

#endif
