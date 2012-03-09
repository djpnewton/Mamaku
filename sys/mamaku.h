#if !defined(_MAMAKU_H_)
#define _MAMAKU_H_

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

#include <wdmguid.h>
#include <bthdef.h>
#include <bthioctl.h>
#include <bthddi.h>
#include <bthguid.h>

#include "trackpad.h"

//
// String definitions
//

#define DRIVERNAME                 "mamaku.sys: "

#define MAMAKU_POOL_TAG            (ULONG) 'amam'

#define NTDEVICE_NAME_STRING       L"\\Device\\Mamaku"
#define SYMBOLIC_NAME_STRING       L"\\DosDevices\\Mamaku"


typedef struct _MAMAKU_CONTEXT 
{
    WDFDEVICE Device;

    WDFIOTARGET IoTarget;

    WDFQUEUE ChildReportQueue;

    BOOLEAN BthInterfaceRetrieved;

    BTH_PROFILE_DRIVER_INTERFACE BthInterface;

    BOOLEAN BthAddressAndChannelRetrieved;

    BTH_ADDR BtAddress;

    L2CAP_CHANNEL_HANDLE ChannelHandle;

    WDFQUEUE DeadAclTransferQueue;

    WDFREQUEST ReadRequest;

    BYTE ReadBuffer[0x10000];

    BRB ReadBRB;

    WDFMEMORY ReadMemory;

    BOOLEAN InMultitouchMode;

    BOOLEAN UseMultitouchDebug;

    MAMAKU_TRACKPAD Trackpad;

} MAMAKU_CONTEXT, *PMAMAKU_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(MAMAKU_CONTEXT, MamakuGetDeviceContext)

typedef struct _MAMAKU_WORK_ITEM_CONTEXT 
{
    WDFDEVICE Device;

} MAMAKU_WORK_ITEM_CONTEXT, *PMAMAKU_WORK_ITEM_CONTEXT;

WDF_DECLARE_CONTEXT_TYPE_WITH_NAME(MAMAKU_WORK_ITEM_CONTEXT, MamakuGetWorkItemContext)

//
// Function definitions
//

DRIVER_INITIALIZE DriverEntry;

EVT_WDF_DRIVER_UNLOAD MamakuDriverUnload;

EVT_WDF_DRIVER_DEVICE_ADD MamakuEvtDeviceAdd;

NTSTATUS MamakuCreatePdo(WDFDEVICE Device, OUT WDFDEVICE* Child);

EVT_WDF_DEVICE_D0_ENTRY MamakuD0Entry;

EVT_WDF_DEVICE_D0_EXIT MamakuD0Exit;

EVT_WDFDEVICE_WDM_IRP_PREPROCESS MamakuIrpPreprocessQueryInterface;

EVT_WDF_IO_QUEUE_IO_READ MamakuIoRead;

EVT_WDF_IO_QUEUE_IO_WRITE MamakuIoWrite;

EVT_WDF_IO_QUEUE_IO_DEVICE_CONTROL MamakuIoDeviceControl;

VOID MamakuReadBtWorkItem(IN WDFWORKITEM WorkItem);
VOID MamakuCreateWorkItem(IN PMAMAKU_CONTEXT devContext, IN PFN_WDF_WORKITEM workItemFunction);

EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL MamakuIoInternalDeviceControl;

EVT_WDF_IO_QUEUE_IO_INTERNAL_DEVICE_CONTROL MamakuChildPdoIoInternalDeviceControl;

//
// Helper macros
//

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

#endif
