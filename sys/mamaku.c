#include "mamaku.h"
#include "../inc/katata_common.h"

//
// Globals
//

ULONG MamakuDebugLevel      = 100;//DEBUG_LEVEL_BLARG;
ULONG MamakuDebugCatagories = DBG_INIT || DBG_PNP || DBG_IOCTL;

//
// Protocol defines
//

#define REPORT_HEADER    0xA1
#define REPORT_ID_TOUCH  0x28
#define REPORT_ID_TOUCH2 0xF7


NTSTATUS
DriverEntry (
    __in PDRIVER_OBJECT  DriverObject,
    __in PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS               status = STATUS_SUCCESS;
    WDF_DRIVER_CONFIG      config;
    WDF_OBJECT_ATTRIBUTES  attributes;

    MamakuPrint(DEBUG_LEVEL_INFO, DBG_INIT,
        "Driver Entry: Built %s %s\n", __DATE__, __TIME__);

    WDF_DRIVER_CONFIG_INIT(&config, MamakuEvtDeviceAdd);

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);

    //
    // Create a framework driver object to represent our driver.
    //

    status = WdfDriverCreate(DriverObject,
                             RegistryPath,
                             &attributes,      
                             &config,         
                             WDF_NO_HANDLE
                             );

    if (!NT_SUCCESS(status)) 
    {
        MamakuPrint(DEBUG_LEVEL_ERROR, DBG_INIT,
            "WdfDriverCreate failed with status 0x%x\n", status);
    }

    return status;
}

VOID
MamakuDeviceCleanup(
    IN WDFDEVICE Device
    )
{
    NTSTATUS status;
    PMAMAKU_CONTEXT devContext;

    MamakuPrint(DEBUG_LEVEL_INFO, DBG_PNP,
        "MamakuDeviceCleanup entry\n");

    devContext = MamakuGetDeviceContext(Device);
}

NTSTATUS
MamakuEvtDeviceAdd(
    IN WDFDRIVER       Driver,
    IN PWDFDEVICE_INIT DeviceInit
    )
{
    NTSTATUS                      status = STATUS_SUCCESS;
    WDF_PNPPOWER_EVENT_CALLBACKS  pnpPowerCallbacks;
    UCHAR                         minorFunction;
    WDF_IO_QUEUE_CONFIG           queueConfig;
    WDF_OBJECT_ATTRIBUTES         attributes;
    WDFDEVICE                     device;
    WDFDEVICE                     childDevice;
    WDFQUEUE                      queue;
    PMAMAKU_CONTEXT               devContext;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    MamakuPrint(DEBUG_LEVEL_INFO, DBG_PNP,
        "MamakuEvtDeviceAdd called\n");

    //
    // Tell framework this is a filter driver. Filter drivers by default are  
    // not power policy owners. This works well for this driver because
    // HIDclass driver is the power policy owner for HID minidrivers.
    //
    
    WdfFdoInitSetFilter(DeviceInit);

    //
    // Initialize the pnp power callbacks
    // 

    WDF_PNPPOWER_EVENT_CALLBACKS_INIT(&pnpPowerCallbacks);

    pnpPowerCallbacks.EvtDeviceD0Entry = MamakuD0Entry; 
    pnpPowerCallbacks.EvtDeviceD0Exit = MamakuD0Exit; 

    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    //
    // Query Interface preprocess callback
    //

    minorFunction = IRP_MN_QUERY_INTERFACE;
    status = WdfDeviceInitAssignWdmIrpPreprocessCallback(DeviceInit, MamakuIrpPreprocessQueryInterface, IRP_MJ_PNP, &minorFunction, 1);

    if (!NT_SUCCESS(status)) 
    {
        MamakuPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
            "WdfDeviceInitAssignWdmIrpPreprocessCallback failed with status code 0x%x\n", status);

        return status;
    }

    //
    // Setup the device context
    //

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, MAMAKU_CONTEXT);

    //
    // Add a device cleanup callback
    //

    attributes.EvtCleanupCallback = MamakuDeviceCleanup;

    //
    // Create a framework device object.This call will in turn create
    // a WDM device object, attach to the lower stack, and set the
    // appropriate flags and attributes.
    //
    
    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);

    if (!NT_SUCCESS(status)) 
    {
        MamakuPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
            "WdfDeviceCreate failed with status code 0x%x\n", status);

        return status;
    }

    devContext = MamakuGetDeviceContext(device);
    devContext->Device = device;
    devContext->IoTarget = WdfDeviceGetIoTarget(device);
    devContext->BthInterfaceRetrieved = FALSE;
    devContext->BthAddressAndChannelRetrieved = FALSE;
    devContext->InMultitouchMode = FALSE;

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);

    queueConfig.EvtIoRead = MamakuIoRead;
    queueConfig.EvtIoWrite = MamakuIoWrite;
    queueConfig.EvtIoDeviceControl = MamakuIoDeviceControl;
    queueConfig.EvtIoInternalDeviceControl = MamakuIoInternalDeviceControl;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = device;

    status = WdfIoQueueCreate(device,
                              &queueConfig,
                              &attributes,
                              &queue
                              );

    if (!NT_SUCCESS (status)) 
    {
        MamakuPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
            "WdfIoQueueCreate #1 failed 0x%x\n", status);

        return status;
    }

    //
    // Create manual queue to stuff l2ca acl read transfers from hidclass above us in
    //

    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);
    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = device;

    status = WdfIoQueueCreate(device, &queueConfig, &attributes, &devContext->DeadAclTransferQueue);

    if (!NT_SUCCESS (status))
    {
        MamakuPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
            "WdfIoQueueCreate #2 failed 0x%x\n", status);

        return status;
    }

    //
    // Create BRB read request
    //

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = device;
    status = WdfRequestCreate(&attributes, devContext->IoTarget, &devContext->ReadRequest);

    if (!NT_SUCCESS (status))
    {
        MamakuPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
            "WdfRequestCreate failed 0x%x\n", status);

        return status;
    }

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = device;
    status = WdfMemoryCreatePreallocated(&attributes, &devContext->ReadBRB, sizeof(BRB), &devContext->ReadMemory);

    if (!NT_SUCCESS (status))
    {
        MamakuPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
            "WdfMemoryCreatePreallocated failed 0x%x\n", status);

        return status;
    }

    //
    // Create child pdo for multitouch testing
    //

    status = MamakuCreatePdo(device, &childDevice);

    if (!NT_SUCCESS (status))
    {
        MamakuPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
            "MamakuCreatePdo failed 0x%x\n", status);

        return status;
    }

    // default queue for child pdo

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);

    queueConfig.EvtIoInternalDeviceControl = MamakuChildPdoIoInternalDeviceControl;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = childDevice;

    status = WdfIoQueueCreate(childDevice,
                              &queueConfig,
                              &attributes,
                              &queue
                              );

    if (!NT_SUCCESS (status)) 
    {
        MamakuPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
            "WdfIoQueueCreate #3 failed 0x%x\n", status);

        return status;
    }

    //
    // Create manual I/O queue to take care of child hid report read requests
    //

    WDF_IO_QUEUE_CONFIG_INIT(&queueConfig, WdfIoQueueDispatchManual);

    queueConfig.PowerManaged = WdfFalse;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    attributes.ParentObject = device;

    status = WdfIoQueueCreate(device,
            &queueConfig,
            &attributes,
            &devContext->ChildReportQueue
            );

    if (!NT_SUCCESS(status)) 
    {
        MamakuPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
            "WdfIoQueueCreate #4 failed 0x%x\n", status);

        return status;
    }

    return status;
}

NTSTATUS
MamakuD0Entry(
    IN WDFDEVICE Device,
    IN WDF_POWER_DEVICE_STATE PreviousState
    )
{
    MamakuPrint(DEBUG_LEVEL_INFO, DBG_PNP,
        "MamakuD0Entry called\n");

    return STATUS_SUCCESS;
}

NTSTATUS
MamakuD0Exit(
    IN WDFDEVICE Device,
    IN WDF_POWER_DEVICE_STATE TargetState
    )
{
    PMAMAKU_CONTEXT devContext;

    MamakuPrint(DEBUG_LEVEL_INFO, DBG_PNP,
        "MamakuD0Exit called\n");

    devContext = MamakuGetDeviceContext(Device);
    
    devContext->InMultitouchMode = FALSE;

    WdfIoQueuePurge(devContext->DeadAclTransferQueue, NULL, NULL);

    return STATUS_SUCCESS;
}

NTSTATUS
MamakuQueryInterfaceCompletion(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    IN PVOID Context
)
{
    PIO_STACK_LOCATION irpSp;

    irpSp = IoGetCurrentIrpStackLocation(Irp);

    MamakuPrint(DEBUG_LEVEL_INFO, DBG_PNP,
        "MamakuQueryInterfaceCompletion called\n")

    if (IsEqualGUID(irpSp->Parameters.QueryInterface.InterfaceType, &GUID_BTHDDI_PROFILE_DRIVER_INTERFACE))
    {
        MamakuPrint(DEBUG_LEVEL_INFO, DBG_PNP,
            "    BTH_PROFILE_DRIVER_INTERFACE retrieved\n");

        if (irpSp->Parameters.QueryInterface.Size == sizeof(BTH_PROFILE_DRIVER_INTERFACE))
        {
            PMAMAKU_CONTEXT devContext = Context;

            devContext->BthInterface = *((PBTH_PROFILE_DRIVER_INTERFACE)irpSp->Parameters.QueryInterface.Interface);
            devContext->BthInterfaceRetrieved = TRUE;
        }
        else
        {
            MamakuPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
                "    irpSp->Parameters.QueryInterface.Size wrong\n");
        }
    }

    // 
    // Because the dispatch routine is returning the status of lower driver
    // as is, you must do the following:
    // 
    if (Irp->PendingReturned)
    {
        IoMarkIrpPending(Irp);
    }
    
    return STATUS_CONTINUE_COMPLETION ; // Make sure of same synchronicity 
    
}

NTSTATUS
MamakuIrpPreprocessQueryInterface(
    IN WDFDEVICE Device,
    IN OUT PIRP Irp)
{
    PMAMAKU_CONTEXT devContext;

    MamakuPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
        "MamakuIrpPreprocessQueryInterface called\n")

    devContext = MamakuGetDeviceContext(Device);

    //
    // Set a completion routine and deliver the IRP back to
    // the framework. 
    //
    
    IoCopyCurrentIrpStackLocationToNext(Irp);
    IoSetCompletionRoutine(Irp, MamakuQueryInterfaceCompletion, devContext, TRUE, FALSE, FALSE);
    return WdfDeviceWdmDispatchPreprocessedIrp(Device, Irp);
}

VOID
MamakuIoRead(
    IN WDFQUEUE Queue,
    IN WDFREQUEST Request,
    IN size_t Length
    )
{
    PMAMAKU_CONTEXT devContext;
    WDF_REQUEST_SEND_OPTIONS options;

    MamakuPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
        "MamakuIoRead called\n")

    devContext = MamakuGetDeviceContext(WdfIoQueueGetDevice(Queue));

    WDF_REQUEST_SEND_OPTIONS_INIT(
                        &options,
                        WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);
        
    if (!WdfRequestSend(Request,
                        devContext->IoTarget,
                        &options))
    {
        NTSTATUS status = WdfRequestGetStatus(Request);

        MamakuPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
            "WdfRequestSend failed - 0x%x\n", status)

        WdfRequestComplete(Request, status);
    }
}

VOID
MamakuIoWrite(
    IN WDFQUEUE Queue,
    IN WDFREQUEST Request,
    IN size_t Length
    )
{
    PMAMAKU_CONTEXT devContext;
    WDF_REQUEST_SEND_OPTIONS options;

    MamakuPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
        "MamakuIoWrite called\n")

    devContext = MamakuGetDeviceContext(WdfIoQueueGetDevice(Queue));

    WDF_REQUEST_SEND_OPTIONS_INIT(
                        &options,
                        WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);
        
    if (!WdfRequestSend(Request,
                        devContext->IoTarget,
                        &options))
    {
        NTSTATUS status = WdfRequestGetStatus(Request);

        MamakuPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
            "WdfRequestSend failed - 0x%x\n", status)

        WdfRequestComplete(Request, status);
    }
}

VOID
MamakuIoDeviceControl(
    IN WDFQUEUE Queue,
    IN WDFREQUEST Request,
    IN size_t OutputBufferLength,
    IN size_t InputBufferLength,
    IN ULONG IoControlCode
    )
{
    PMAMAKU_CONTEXT devContext;
    WDF_REQUEST_SEND_OPTIONS options;

    MamakuPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
        "MamakuIoDeviceControl called (0x%x)\n", IoControlCode)

    devContext = MamakuGetDeviceContext(WdfIoQueueGetDevice(Queue));

    WDF_REQUEST_SEND_OPTIONS_INIT(
                        &options,
                        WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);
        
    if (!WdfRequestSend(Request,
                        devContext->IoTarget,
                        &options))
    {
        NTSTATUS status = WdfRequestGetStatus(Request);

        MamakuPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
            "WdfRequestSend failed - 0x%x\n", status)

        WdfRequestComplete(Request, status);
    }
}

NTSTATUS
MamakuSetMultitouchMode(
    IN PMAMAKU_CONTEXT devContext,
    IN WDFIOTARGET Target,
    IN BTH_ADDR BtAddress,
    IN L2CAP_CHANNEL_HANDLE ChannelHandle
    )
{
    NTSTATUS status;
    WDFREQUEST request;
    PBRB brbTransfer = NULL;
    WDF_MEMORY_DESCRIPTOR memoryDesc;

    //
    // Code to switch magic trackpad into multitouch mode
    //
    BYTE feature[] = { REPORT_HEADER, 0xD7, 0x01 };
    
    MamakuPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
        "MamakuSetMultitouchMode called\n");


    status = WdfRequestCreate(WDF_NO_OBJECT_ATTRIBUTES, Target, &request);

    if (!NT_SUCCESS (status))
    {
        MamakuPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
            "WdfRequestCreate failed 0x%x\n", status);

        return status;
    }

    //
    // Create Bluetooth Request Block
    //

    brbTransfer = devContext->BthInterface.BthAllocateBrb(BRB_L2CA_ACL_TRANSFER, MAMAKU_POOL_TAG);

    if (brbTransfer == NULL)
    {
        MamakuPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
            "BthAllocateBrb failed\n");

        status = STATUS_INSUFFICIENT_RESOURCES;
        goto cleanup;
    }

    brbTransfer->BrbL2caAclTransfer.BtAddress = BtAddress;
    brbTransfer->BrbL2caAclTransfer.ChannelHandle = ChannelHandle;
    brbTransfer->BrbL2caAclTransfer.TransferFlags = ACL_TRANSFER_DIRECTION_OUT;
    brbTransfer->BrbL2caAclTransfer.BufferSize = sizeof(feature);
    brbTransfer->BrbL2caAclTransfer.Buffer = feature;
    brbTransfer->BrbL2caAclTransfer.BufferMDL = NULL;
    brbTransfer->BrbL2caAclTransfer.Timeout = 0;
    brbTransfer->BrbL2caAclTransfer.RemainingBufferSize = 0;

    //
    // Send BRB IOCTL
    //

    WDF_MEMORY_DESCRIPTOR_INIT_BUFFER(&memoryDesc, brbTransfer, sizeof(brbTransfer->BrbHeader.Length));

    status = WdfIoTargetSendInternalIoctlOthersSynchronously(Target, request, IOCTL_INTERNAL_BTH_SUBMIT_BRB,
                    &memoryDesc, NULL, NULL, NULL, NULL); 

    if (!NT_SUCCESS (status))
    {
        MamakuPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
            "WdfIoTargetSendInternalIoctlSynchronously failed 0x%x\n", status);

        goto cleanup;
    }

    //
    // Set multitouch mode flag
    //

    devContext->InMultitouchMode = TRUE;

    //
    // Start our read bt workitem chain
    //

    MamakuCreateWorkItem(devContext, MamakuReadBtWorkItem);

cleanup:

    if (brbTransfer != NULL)
    {
        devContext->BthInterface.BthFreeBrb(brbTransfer);
    }

    WdfObjectDelete(request);

    return status;
}

VOID
MamakuSetMtWorkItem(
    IN WDFWORKITEM WorkItem
    )
{
    NTSTATUS status;
    WDFDEVICE device;
    PMAMAKU_CONTEXT devContext;
    PMAMAKU_WORK_ITEM_CONTEXT workitemContext;

    MamakuPrint(DEBUG_LEVEL_INFO, DBG_PNP,
        "MamakuSetMtWorkItem called\n");

    workitemContext = MamakuGetWorkItemContext(WorkItem);
    devContext = MamakuGetDeviceContext(workitemContext->Device);

    MamakuSetMultitouchMode(devContext, devContext->IoTarget, devContext->BtAddress, devContext->ChannelHandle);
}

VOID
MamakuParseTouch(
    IN PMAMAKU_CONTEXT devContext,
    IN BYTE* data,
    IN int touch_index)
{
    int x, y, touch_major, touch_minor, size, id, orientation, state;

    x = (data[1] << 27 | data[0] << 19) >> 19;
    y = -((data[3] << 30 | data[2] << 22 | data[1] << 14) >> 19);
    touch_major = data[4];
    touch_minor = data[5];
    size = data[6] & 0x3f;
    id = (data[7] << 2 | data[6] >> 6) & 0xf;
    orientation = (data[7] >> 2) - 32;
    state = data[8] & 0xf0;

    MamakuPrint(DEBUG_LEVEL_BLARG, DBG_IOCTL,
                "    touch_index: %d, id: %d, state: %x, x: %d, y: %d\n", touch_index, id, state, x, y);

    {
        NTSTATUS status;
        WDFREQUEST reqRead;
        KatataMultiTouchReport* report;
        size_t bytesReturned;

        status = WdfIoQueueRetrieveNextRequest(devContext->ChildReportQueue, &reqRead);

        if (NT_SUCCESS(status)) 
        {
            status = WdfRequestRetrieveOutputBuffer(reqRead,
                                                    sizeof(KatataMultiTouchReport),
                                                    &report,
                                                    &bytesReturned);

            if (NT_SUCCESS(status)) 
            {
                //
                // Copy data into request
                //

                if (bytesReturned > sizeof(KatataMultiTouchReport))
                {
                    bytesReturned = sizeof(KatataMultiTouchReport);
                }

#define TP_MIN_X -2909
#define TP_MAX_X 3167
#define TP_MIN_Y -2456
#define TP_MAX_Y 2565

                report->ReportID = REPORTID_MTOUCH;
                report->Touch[0].Status = MULTI_IN_RANGE_BIT | MULTI_CONFIDENCE_BIT;
                if (state)
                    report->Touch[0].Status |= MULTI_TIPSWITCH_BIT;
                report->Touch[0].ContactID = 0;
                report->Touch[0].XValue = (USHORT)(((x - TP_MIN_X) / (float)(TP_MAX_X - TP_MIN_X)) * (float)(MULTI_MAX_COORDINATE - MULTI_MIN_COORDINATE));
                report->Touch[0].YValue = (USHORT)(((y - TP_MIN_Y) / (float)(TP_MAX_Y - TP_MIN_Y)) * (float)(MULTI_MAX_COORDINATE - MULTI_MIN_COORDINATE));
                report->Touch[0].Width = 0;
                report->Touch[0].Height = 0;
                report->ActualCount = 1;

                //
                // Complete read with the number of bytes returned as info
                //
                
                WdfRequestCompleteWithInformation(reqRead, 
                        status, 
                        bytesReturned);

                MamakuPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
                        "MamakuParseTouch %d bytes returned\n", bytesReturned);

            }
            else
            {
                MamakuPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
                    "WdfRequestRetrieveOutputBuffer failed Status 0x%x\n", status);
            }
        }
        else
        {
            MamakuPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
                    "WdfIoQueueRetrieveNextRequest failed Status 0x%x\n", status);
        }
        
    }
}

VOID
MamakuParseTouchBuffer(
    IN PMAMAKU_CONTEXT devContext,
    IN BYTE* buf,
    IN int size)
{
    if (*buf == REPORT_ID_TOUCH)
    {
        BYTE* data;
        int i, touch_count = (size - 4) / 9;
        int timestamp;

        MamakuPrint(DEBUG_LEVEL_BLARG, DBG_IOCTL,
            "    Touch Report\n");

        data = buf + 1;
        timestamp = data[0] >> 6 | data[1] << 2 | data[2] << 10;

        for (i = 0; i < touch_count; i++)
        {
            data = buf + 4 + i * 9;
            MamakuParseTouch(devContext, data, i);
        }
    }
    if (*buf == REPORT_ID_TOUCH2)
    {
        MamakuPrint(DEBUG_LEVEL_BLARG, DBG_IOCTL,
            "    Touch Report (2)\n");

        MamakuParseTouchBuffer(devContext, buf + 2, buf[1]);
        MamakuParseTouchBuffer(devContext, buf + 2 + buf[1], size - 2 - buf[1]);
    }
}

VOID
MamakuReadBtComplete(
    IN WDFREQUEST Request,
    IN WDFIOTARGET Target,
    IN PWDF_REQUEST_COMPLETION_PARAMS Params,
    IN WDFCONTEXT Context
    )
{
    NTSTATUS status;
    PMAMAKU_CONTEXT devContext = Context;

    MamakuPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
        "MamakuReadBtComplete called (reqType: 0x%x, status: 0x%x, info: 0x%x)\n", Params->Type, Params->IoStatus.Status, Params->IoStatus.Information);

    if (Params->IoStatus.Status == STATUS_SUCCESS)
    {
        WDF_REQUEST_REUSE_PARAMS params;
        PBRB brb = &devContext->ReadBRB;
        struct _BRB_L2CA_ACL_TRANSFER* aclTransfer = &devContext->ReadBRB.BrbL2caAclTransfer;
        BYTE* buf = aclTransfer->Buffer;

        MamakuPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
            "    _BRB_L2CA_ACL_TRANSFER (BtAddress: 0x%x, ChannelHandle: 0x%x)\n",
            aclTransfer->BtAddress, aclTransfer->ChannelHandle);
        MamakuPrint(DEBUG_LEVEL_BLARG, DBG_IOCTL,
            "    _BRB_L2CA_ACL_TRANSFER (TransferFlags: 0x%x, BufferSize: 0x%x, RemainingBufferSize: 0x%x)\n",
            aclTransfer->TransferFlags, aclTransfer->BufferSize, aclTransfer->RemainingBufferSize);

        //
        // Parse buffer
        //

        if (buf != NULL && aclTransfer->BufferSize > 0 && *buf == REPORT_HEADER)
        {
            MamakuParseTouchBuffer(devContext, buf + 1, aclTransfer->BufferSize - 1);
        }

        //
        // Reuse request
        //

        WDF_REQUEST_REUSE_PARAMS_INIT(&params, WDF_REQUEST_REUSE_NO_FLAGS, STATUS_SUCCESS);
        status = WdfRequestReuse(devContext->ReadRequest, &params);

        if (!NT_SUCCESS (status))
        {
            MamakuPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
                "WdfRequestReuse failed 0x%x\n", status);

            return;
        }

        //
        // Send new request
        //

        MamakuCreateWorkItem(devContext, MamakuReadBtWorkItem);
    }
}

NTSTATUS
MamakuReadBt(
    IN PMAMAKU_CONTEXT devContext,
    IN WDFIOTARGET Target,
    IN BTH_ADDR BtAddress,
    IN L2CAP_CHANNEL_HANDLE ChannelHandle
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PBRB brb;

    MamakuPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
        "MamakuReadBt called\n");

    //
    // Initialize Bluetooth Request Block
    //

    brb = &devContext->ReadBRB;
    devContext->BthInterface.BthInitializeBrb(brb, BRB_L2CA_ACL_TRANSFER);
    brb->BrbL2caAclTransfer.BtAddress = BtAddress;
    brb->BrbL2caAclTransfer.ChannelHandle = ChannelHandle;
    brb->BrbL2caAclTransfer.TransferFlags = ACL_TRANSFER_DIRECTION_IN | ACL_SHORT_TRANSFER_OK;
    brb->BrbL2caAclTransfer.BufferSize = sizeof(devContext->ReadBuffer);
    brb->BrbL2caAclTransfer.Buffer = devContext->ReadBuffer;
    brb->BrbL2caAclTransfer.BufferMDL = NULL;
    brb->BrbL2caAclTransfer.Timeout = 0;
    brb->BrbL2caAclTransfer.RemainingBufferSize = 0;

    //
    // Send BRB IOCTL
    //

    status = WdfIoTargetFormatRequestForInternalIoctlOthers(Target, devContext->ReadRequest, IOCTL_INTERNAL_BTH_SUBMIT_BRB,
                    devContext->ReadMemory, NULL, NULL, NULL, NULL, NULL); 

    if (!NT_SUCCESS (status))
    {
        MamakuPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
            "WdfIoTargetFormatRequestForInternalIoctlOthers failed 0x%x\n", status);

        return status;
    }

    WdfRequestSetCompletionRoutine(devContext->ReadRequest, MamakuReadBtComplete, devContext);
    
    if (WdfRequestSend(devContext->ReadRequest, devContext->IoTarget, WDF_NO_SEND_OPTIONS) == FALSE)
    {
        status = WdfRequestGetStatus(devContext->ReadRequest);

        MamakuPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
            "WdfRequestSend failed, WdfRequestGetStatus = 0x%x\n", status);
    }
    else
    {
        status = STATUS_SUCCESS;
    }

    return status;
}

VOID
MamakuReadBtWorkItem(
    IN WDFWORKITEM WorkItem
    )
{
    NTSTATUS status;
    WDFDEVICE device;
    PMAMAKU_CONTEXT devContext;
    PMAMAKU_WORK_ITEM_CONTEXT workitemContext;

    MamakuPrint(DEBUG_LEVEL_INFO, DBG_PNP,
        "MamakuReadBtWorkItem called\n");

    workitemContext = MamakuGetWorkItemContext(WorkItem);
    devContext = MamakuGetDeviceContext(workitemContext->Device);

    MamakuReadBt(devContext, devContext->IoTarget, devContext->BtAddress, devContext->ChannelHandle);
}

VOID
MamakuCreateWorkItem(
    IN PMAMAKU_CONTEXT devContext,
    IN PFN_WDF_WORKITEM workItemFunction
    )
{
    NTSTATUS status;
    PMAMAKU_WORK_ITEM_CONTEXT workitemContext;
    WDF_OBJECT_ATTRIBUTES attributes;
    WDF_WORKITEM_CONFIG workitemConfig;
    WDFWORKITEM workItem;

    WDF_OBJECT_ATTRIBUTES_INIT(&attributes);
    WDF_OBJECT_ATTRIBUTES_SET_CONTEXT_TYPE(&attributes, MAMAKU_WORK_ITEM_CONTEXT);
    attributes.ParentObject = devContext->Device;

    WDF_WORKITEM_CONFIG_INIT(&workitemConfig, workItemFunction);

    status = WdfWorkItemCreate(&workitemConfig, &attributes, &workItem);

    if (!NT_SUCCESS(status))
    {
        MamakuPrint(DEBUG_LEVEL_INFO, DBG_PNP,
            "WdfWorkItemCreate failed 0x%x\n", status)

        return;
    }

    workitemContext = MamakuGetWorkItemContext(workItem);
    workitemContext->Device = devContext->Device;

    WdfWorkItemEnqueue(workItem);
}

VOID
MamakuIoInternalDeviceControlComplete(
    IN WDFREQUEST Request,
    IN WDFIOTARGET Target,
    IN PWDF_REQUEST_COMPLETION_PARAMS Params,
    IN WDFCONTEXT Context
    )
{
    PMAMAKU_CONTEXT devContext = Context;

    MamakuPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
        "MamakuIoInternalDeviceControlComplete called (ioctl: 0x%x, status: 0x%x, info: 0x%x)\n", Params->Parameters.Ioctl.IoControlCode, Params->IoStatus.Status, Params->IoStatus.Information);

    if (Params->IoStatus.Status == STATUS_SUCCESS &&
        Params->Parameters.Ioctl.IoControlCode == IOCTL_INTERNAL_BTH_SUBMIT_BRB)
    {
        PIRP irp = WdfRequestWdmGetIrp(Request);
        PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
        BRB_HEADER* brbHeader = irpSp->Parameters.Others.Argument1;

        MamakuPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
            "  IOCTL_INTERNAL_BTH_SUBMIT_BRB (Type: 0x%x)\n", brbHeader->Type);

        switch (brbHeader->Type)
        {
            case BRB_L2CA_ACL_TRANSFER:
            { 
                struct _BRB_L2CA_ACL_TRANSFER* aclTransfer = irpSp->Parameters.Others.Argument1;

                MamakuPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
                    "  _BRB_L2CA_ACL_TRANSFER (BtAddress: 0x%x, ChannelHandle: 0x%x)\n",
                    aclTransfer->BtAddress, aclTransfer->ChannelHandle);
                MamakuPrint(DEBUG_LEVEL_BLARG, DBG_IOCTL,
                    "  _BRB_L2CA_ACL_TRANSFER (TransferFlags: 0x%x, BufferSize: 0x%x, RemainingBufferSize: 0x%x)\n",
                    aclTransfer->TransferFlags, aclTransfer->BufferSize, aclTransfer->RemainingBufferSize);

                if ((aclTransfer->TransferFlags & ACL_TRANSFER_DIRECTION_IN) == ACL_TRANSFER_DIRECTION_IN ||
                    (aclTransfer->TransferFlags & ACL_TRANSFER_DIRECTION_OUT) == ACL_TRANSFER_DIRECTION_OUT)
                {
                    BYTE* buf = aclTransfer->Buffer;

                    if (buf != NULL)
                    {
                        // I dont know what this report is but I guess it is hidclass sending down a feature report or something
                        // when we detect this being send we will also put the device into multitouch mode
                        BYTE some_report[] = {REPORT_HEADER, 0x60, 0x2};

                        /*
                        if (aclTransfer->BufferSize >= 5)
                        {
                            MamakuPrint(DEBUG_LEVEL_BLARG, DBG_IOCTL,
                                "    %2.2x %2.2x %2.2x %2.2x %2.2x\n", *buf, *(buf+1), *(buf+2), *(buf+3), *(buf+4));
                        }
                        else if (aclTransfer->BufferSize >= 4)
                        {
                            MamakuPrint(DEBUG_LEVEL_BLARG, DBG_IOCTL,
                                "    %2.2x %2.2x %2.2x %2.2x\n", *buf, *(buf+1), *(buf+2), *(buf+3));
                        }
                        else if (aclTransfer->BufferSize >= 3)
                        {
                            MamakuPrint(DEBUG_LEVEL_BLARG, DBG_IOCTL,
                                "    %2.2x %2.2x %2.2x\n", *buf, *(buf+1), *(buf+2));
                        }
                        else if (aclTransfer->BufferSize >= 2)
                        {
                            MamakuPrint(DEBUG_LEVEL_BLARG, DBG_IOCTL,
                                "    %2.2x %2.2x\n", *buf, *(buf+1));
                        }
                        else if (aclTransfer->BufferSize >= 1)
                        {
                            MamakuPrint(DEBUG_LEVEL_BLARG, DBG_IOCTL,
                                "    %2.2x\n", *buf);
                        }
                        */

                        //
                        // Create workitem to set multitouch mode
                        //

                        if (aclTransfer->BufferSize == sizeof(some_report) && memcmp(buf, some_report, sizeof(some_report)) == 0 &&
                            devContext->BthInterfaceRetrieved && devContext->BthAddressAndChannelRetrieved)
                        {
                            MamakuCreateWorkItem(devContext, MamakuSetMtWorkItem);
                        }

                    }
                }
                break;
            }

            case BRB_L2CA_OPEN_CHANNEL_RESPONSE:
            {
                struct _BRB_L2CA_OPEN_CHANNEL* openChannel = irpSp->Parameters.Others.Argument1;

                MamakuPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
                    "  _BRB_L2CA_OPEN_CHANNEL (BtAddress: 0x%x, ChannelHandle: 0x%x)\n",
                    openChannel->BtAddress, openChannel->ChannelHandle);

                //
                // Save bluetooth address and channel
                //

                devContext->BtAddress = openChannel->BtAddress;
                devContext->ChannelHandle = openChannel->ChannelHandle;
                devContext->BthAddressAndChannelRetrieved = TRUE;

                break;
            }
        }
    }

    WdfRequestComplete(Request, Params->IoStatus.Status);
}

VOID
MamakuIoInternalDeviceControl(
    IN WDFQUEUE Queue,
    IN WDFREQUEST Request,
    IN size_t OutputBufferLength,
    IN size_t InputBufferLength,
    IN ULONG IoControlCode
    )
{
    NTSTATUS status;
    PMAMAKU_CONTEXT devContext;
    WDFMEMORY inMemory;
    WDFMEMORY outMemory;

    MamakuPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
        "MamakuIoInternalDeviceControl called (0x%x)\n", IoControlCode);

    devContext = MamakuGetDeviceContext(WdfIoQueueGetDevice(Queue));

    //
    // If in multitouch mode we want to stuff upstream read requests into a manual queue
    //

    if (devContext->InMultitouchMode)
    {
        WDF_REQUEST_PARAMETERS Params;

        MamakuPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
            "  InMultitouchMode == TRUE\n", IoControlCode);

        WDF_REQUEST_PARAMETERS_INIT(&Params);
        WdfRequestGetParameters(Request, &Params);

        if (Params.Parameters.DeviceIoControl.IoControlCode == IOCTL_INTERNAL_BTH_SUBMIT_BRB)
        {
            PIRP irp = WdfRequestWdmGetIrp(Request);
            PIO_STACK_LOCATION irpSp = IoGetCurrentIrpStackLocation(irp);
            BRB_HEADER* brbHeader = irpSp->Parameters.Others.Argument1;

            MamakuPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
                "  IOCTL_INTERNAL_BTH_SUBMIT_BRB (Type: 0x%x)\n", brbHeader->Type);

            switch (brbHeader->Type)
            {
                case BRB_L2CA_ACL_TRANSFER:            

                    status = WdfRequestForwardToIoQueue(Request, devContext->DeadAclTransferQueue);

                    if (!NT_SUCCESS (status))
                    {
                        MamakuPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
                            "WdfRequestForwardToIoQueue failed 0x%x\n", status);

                        goto complete_failure;
                    }   

                    return;
            }
        }
    }

    //
    // Setup completion routine and send request down the stack
    //

    status = WdfRequestRetrieveInputMemory(Request, &inMemory);

    if (!NT_SUCCESS (status))
    {
        if (status == STATUS_BUFFER_TOO_SMALL)
        {
            // The input buffer's length is zero.
            inMemory = WDF_NO_HANDLE;
        }
        else
        {
            MamakuPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
                "WdfRequestRetrieveInputMemory failed 0x%x\n", status);

            goto complete_failure;
        }
    }

    status = WdfRequestRetrieveOutputMemory(Request, &outMemory);

    if (!NT_SUCCESS (status))
    {
        if (status == STATUS_BUFFER_TOO_SMALL)
        {
            // The output buffer's length is zero.
            outMemory = WDF_NO_HANDLE;
        }
        else
        {
            MamakuPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
                "WdfRequestRetrieveOutputMemory failed 0x%x\n", status);

            goto complete_failure;
        }
    }    

    status = WdfIoTargetFormatRequestForInternalIoctl(devContext->IoTarget,
                                                      Request,
                                                      IoControlCode,
                                                      inMemory,
                                                      NULL,
                                                      outMemory,
                                                      NULL);

    if (!NT_SUCCESS (status))
    {
        MamakuPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
            "WdfIoTargetFormatRequestForInternalIoctl failed 0x%x\n", status);

        goto complete_failure;
    }   

    WdfRequestSetCompletionRoutine(Request,
                                   MamakuIoInternalDeviceControlComplete,
                                   devContext);
    
    if (!WdfRequestSend(Request,
                        devContext->IoTarget,
                        NULL))
    {
        NTSTATUS status = WdfRequestGetStatus(Request);

        MamakuPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
            "WdfRequestSend failed - 0x%x\n", status)

        goto complete_failure;
    }

    return;
    
complete_failure:

    WdfRequestComplete(Request, status);
}

VOID
MamakuChildPdoIoInternalDeviceControl(
    IN WDFQUEUE Queue,
    IN WDFREQUEST Request,
    IN size_t OutputBufferLength,
    IN size_t InputBufferLength,
    IN ULONG IoControlCode
    )
{
    NTSTATUS status;
    PMAMAKU_CONTEXT devContext;
    WDFMEMORY inMemory;
    WDFMEMORY outMemory;

    MamakuPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
        "MamakuChildPdoIoInternalDeviceControl called (0x%x)\n", IoControlCode);

    devContext = MamakuGetDeviceContext(WdfPdoGetParent(WdfIoQueueGetDevice(Queue)));

    //
    // Forward this hid read request to our manual queue
    // (in other words, we are going to defer this request
    // until we have a corresponding bluetooth read completion
    // to match it will)
    //
    
    status = WdfRequestForwardToIoQueue(Request, devContext->ChildReportQueue);

    if(!NT_SUCCESS(status))
    {
        MamakuPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
                "WdfRequestForwardToIoQueue failed Status 0x%x\n", status);

        WdfRequestComplete(Request, status);
    }
}

