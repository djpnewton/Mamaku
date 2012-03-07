#include "katata.h"

#include <wdmguid.h>

//
// Globals
//

ULONG KatataDebugLevel      = 100;
ULONG KatataDebugCatagories = DBG_INIT || DBG_PNP || DBG_READ || DBG_IOCTL;


NTSTATUS
DriverEntry (
    __in PDRIVER_OBJECT  DriverObject,
    __in PUNICODE_STRING RegistryPath
    )
{
    NTSTATUS               status = STATUS_SUCCESS;
    WDF_DRIVER_CONFIG      config;
    WDF_OBJECT_ATTRIBUTES  attributes;

    KatataPrint(DEBUG_LEVEL_INFO, DBG_INIT,
        "Driver Entry: Built %s %s\n", __DATE__, __TIME__);

    WDF_DRIVER_CONFIG_INIT(&config, KatataEvtDeviceAdd);

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
        KatataPrint(DEBUG_LEVEL_ERROR, DBG_INIT,
            "WdfDriverCreate failed with status 0x%x\n", status);
    }

    return status;
}

VOID
KatataDeviceCleanup(
    IN WDFDEVICE Device
    )
{
    NTSTATUS status;
    PKATATA_CONTEXT devContext;

    KatataPrint(DEBUG_LEVEL_INFO, DBG_PNP,
        "KatataDeviceCleanup entry\n");

    devContext = KatataGetDeviceContext(Device);
}

NTSTATUS
KatataEvtDeviceAdd(
    IN WDFDRIVER       Driver,
    IN PWDFDEVICE_INIT DeviceInit
    )
{
    NTSTATUS                      status = STATUS_SUCCESS;
    WDF_PNPPOWER_EVENT_CALLBACKS  pnpPowerCallbacks;
    WDF_IO_QUEUE_CONFIG           queueConfig;
    WDF_OBJECT_ATTRIBUTES         attributes;
    WDFDEVICE                     device;
    WDFQUEUE                      queue;
    PKATATA_CONTEXT               devContext;

    UNREFERENCED_PARAMETER(Driver);

    PAGED_CODE();

    KatataPrint(DEBUG_LEVEL_INFO, DBG_PNP,
        "KatataEvtDeviceAdd called\n");

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

    pnpPowerCallbacks.EvtDeviceD0Entry = KatataD0Entry; 
    pnpPowerCallbacks.EvtDeviceD0Exit = KatataD0Exit; 

    WdfDeviceInitSetPnpPowerEventCallbacks(DeviceInit, &pnpPowerCallbacks);

    //
    // Setup the device context
    //

    WDF_OBJECT_ATTRIBUTES_INIT_CONTEXT_TYPE(&attributes, KATATA_CONTEXT);

    //
    // Add a device cleanup callback
    //

    attributes.EvtCleanupCallback = KatataDeviceCleanup;

    //
    // Create a framework device object.This call will in turn create
    // a WDM device object, attach to the lower stack, and set the
    // appropriate flags and attributes.
    //
    
    status = WdfDeviceCreate(&DeviceInit, &attributes, &device);

    if (!NT_SUCCESS(status)) 
    {
        KatataPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
            "WdfDeviceCreate failed with status code 0x%x\n", status);

        return status;
    }

    devContext = KatataGetDeviceContext(device);

    WDF_IO_QUEUE_CONFIG_INIT_DEFAULT_QUEUE(&queueConfig, WdfIoQueueDispatchParallel);

    queueConfig.EvtIoInternalDeviceControl = KatataEvtInternalDeviceControl;

    status = WdfIoQueueCreate(device,
                              &queueConfig,
                              WDF_NO_OBJECT_ATTRIBUTES,
                              &queue
                              );

    if (!NT_SUCCESS (status)) 
    {
        KatataPrint(DEBUG_LEVEL_ERROR, DBG_PNP,
            "WdfIoQueueCreate failed 0x%x\n", status);

        return status;
    }

    //
    // Get I/O target
    //

    devContext->IoTarget = WdfDeviceGetIoTarget(device);

    //
    // Initialize DeviceMode
    //

    devContext->DeviceMode = DEVICE_MODE_MOUSE;

    return status;
}

NTSTATUS
KatataD0Entry(
    IN WDFDEVICE Device,
    IN WDF_POWER_DEVICE_STATE PreviousState
    )
{
    KatataPrint(DEBUG_LEVEL_INFO, DBG_PNP,
        "KatataD0Entry called\n")

    return STATUS_SUCCESS;
}

NTSTATUS
KatataD0Exit(
    IN WDFDEVICE Device,
    IN WDF_POWER_DEVICE_STATE TargetState
    )
{
    KatataPrint(DEBUG_LEVEL_INFO, DBG_PNP,
        "KatataD0Exit called\n")

    return STATUS_SUCCESS;
}

VOID
KatataEvtInternalDeviceControl(
    IN WDFQUEUE     Queue,
    IN WDFREQUEST   Request,
    IN size_t       OutputBufferLength,
    IN size_t       InputBufferLength,
    IN ULONG        IoControlCode
    )
{
    NTSTATUS            status = STATUS_SUCCESS;
    WDFDEVICE           device;
    PKATATA_CONTEXT     devContext;
    BOOLEAN             completeRequest = TRUE;

    UNREFERENCED_PARAMETER(OutputBufferLength);
    UNREFERENCED_PARAMETER(InputBufferLength);

    device = WdfIoQueueGetDevice(Queue);
    devContext = KatataGetDeviceContext(device);

    KatataPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
        "%s, Queue:0x%p, Request:0x%p\n",
        DbgHidInternalIoctlString(IoControlCode),
        Queue, 
        Request
        );

    //
    // Please note that HIDCLASS provides the buffer in the Irp->UserBuffer
    // field irrespective of the ioctl buffer type. However, framework is very
    // strict about type checking. You cannot get Irp->UserBuffer by using
    // WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
    // internal ioctl. So depending on the ioctl code, we will either
    // use retreive function or escape to WDM to get the UserBuffer.
    //

    switch (IoControlCode) 
    {

    case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
        //
        // Retrieves the device's HID descriptor.
        //
        status = KatataGetHidDescriptor(device, Request);
        break;

    case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
        //
        //Retrieves a device's attributes in a HID_DEVICE_ATTRIBUTES structure.
        //
        status = KatataGetDeviceAttributes(Request);
        break;

    case IOCTL_HID_GET_REPORT_DESCRIPTOR:
        //
        //Obtains the report descriptor for the HID device.
        //
        status = KatataGetReportDescriptor(device, Request);
        break;

    case IOCTL_HID_GET_STRING:
        //
        // Requests that the HID minidriver retrieve a human-readable string
        // for either the manufacturer ID, the product ID, or the serial number
        // from the string descriptor of the device. The minidriver must send
        // a Get String Descriptor request to the device, in order to retrieve
        // the string descriptor, then it must extract the string at the
        // appropriate index from the string descriptor and return it in the
        // output buffer indicated by the IRP. Before sending the Get String
        // Descriptor request, the minidriver must retrieve the appropriate
        // index for the manufacturer ID, the product ID or the serial number
        // from the device extension of a top level collection associated with
        // the device.
        //
        status = KatataGetString(Request);
        break;

    case IOCTL_HID_WRITE_REPORT:
    case IOCTL_HID_SET_OUTPUT_REPORT:
        //
        //Transmits a class driver-supplied report to the device.
        //
        status = KatataWriteReport(devContext, Request);
        break;
 
    case IOCTL_HID_READ_REPORT:
    case IOCTL_HID_GET_INPUT_REPORT:
        //
        // Returns a report from the device into a class driver-supplied buffer.
        // 
        status = KatataReadReport(devContext, Request, &completeRequest);
        break;

    case IOCTL_HID_SET_FEATURE:
        //
        // This sends a HID class feature report to a top-level collection of
        // a HID class device.
        //
        status = KatataSetFeature(devContext, Request, &completeRequest);
        break;

    case IOCTL_HID_GET_FEATURE:
        //
        // returns a feature report associated with a top-level collection
        //
        status = KatataGetFeature(devContext, Request, &completeRequest);
        break;

    case IOCTL_HID_ACTIVATE_DEVICE:
        //
        // Makes the device ready for I/O operations.
        //
    case IOCTL_HID_DEACTIVATE_DEVICE:
        //
        // Causes the device to cease operations and terminate all outstanding
        // I/O requests.
        //
    default:
        status = STATUS_NOT_SUPPORTED;
        break;
    }

    if (completeRequest)
    {
        WdfRequestComplete(Request, status);

        KatataPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
                "%s completed, Queue:0x%p, Request:0x%p\n",
                DbgHidInternalIoctlString(IoControlCode),
                Queue, 
                Request
        );
    }
    else
    {
        KatataPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
                "%s deferred, Queue:0x%p, Request:0x%p\n",
                DbgHidInternalIoctlString(IoControlCode),
                Queue, 
                Request
        );
    }

    return;
}

NTSTATUS
KatataGetHidDescriptor(
    IN WDFDEVICE Device,
    IN WDFREQUEST Request
    )
{
    NTSTATUS            status = STATUS_SUCCESS;
    size_t              bytesToCopy = 0;
    WDFMEMORY           memory;

    UNREFERENCED_PARAMETER(Device);

    KatataPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
        "KatataGetHidDescriptor Entry\n");

    //
    // This IOCTL is METHOD_NEITHER so WdfRequestRetrieveOutputMemory
    // will correctly retrieve buffer from Irp->UserBuffer. 
    // Remember that HIDCLASS provides the buffer in the Irp->UserBuffer
    // field irrespective of the ioctl buffer type. However, framework is very
    // strict about type checking. You cannot get Irp->UserBuffer by using
    // WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
    // internal ioctl.
    //
    status = WdfRequestRetrieveOutputMemory(Request, &memory);

    if (!NT_SUCCESS(status)) 
    {
        KatataPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
            "WdfRequestRetrieveOutputMemory failed 0x%x\n", status);

        return status;
    }

    //
    // Use hardcoded "HID Descriptor" 
    //
    bytesToCopy = DefaultHidDescriptor.bLength;

    if (bytesToCopy == 0) 
    {
        status = STATUS_INVALID_DEVICE_STATE;

        KatataPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
            "DefaultHidDescriptor is zero, 0x%x\n", status);

        return status;        
    }
    
    status = WdfMemoryCopyFromBuffer(memory,
                            0, // Offset
                            (PVOID) &DefaultHidDescriptor,
                            bytesToCopy);

    if (!NT_SUCCESS(status)) 
    {
        KatataPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
            "WdfMemoryCopyFromBuffer failed 0x%x\n", status);

        return status;
    }

    //
    // Report how many bytes were copied
    //
    WdfRequestSetInformation(Request, bytesToCopy);

    KatataPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
        "KatataGetHidDescriptor Exit = 0x%x\n", status);

    return status;
}

NTSTATUS
KatataGetReportDescriptor(
    IN WDFDEVICE Device,
    IN WDFREQUEST Request
    )
{
    NTSTATUS            status = STATUS_SUCCESS;
    ULONG_PTR           bytesToCopy;
    WDFMEMORY           memory;

    UNREFERENCED_PARAMETER(Device);

    KatataPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
        "KatataGetReportDescriptor Entry\n");

    //
    // This IOCTL is METHOD_NEITHER so WdfRequestRetrieveOutputMemory
    // will correctly retrieve buffer from Irp->UserBuffer. 
    // Remember that HIDCLASS provides the buffer in the Irp->UserBuffer
    // field irrespective of the ioctl buffer type. However, framework is very
    // strict about type checking. You cannot get Irp->UserBuffer by using
    // WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
    // internal ioctl.
    //
    status = WdfRequestRetrieveOutputMemory(Request, &memory);
    if (!NT_SUCCESS(status)) 
    {
        KatataPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
            "WdfRequestRetrieveOutputMemory failed 0x%x\n", status);

        return status;
    }

    //
    // Use hardcoded Report descriptor
    //
    bytesToCopy = DefaultHidDescriptor.DescriptorList[0].wReportLength;

    if (bytesToCopy == 0) 
    {
        status = STATUS_INVALID_DEVICE_STATE;

        KatataPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
            "DefaultHidDescriptor's reportLength is zero, 0x%x\n", status);

        return status;        
    }
    
    status = WdfMemoryCopyFromBuffer(memory,
                            0,
                            (PVOID) DefaultReportDescriptor,
                            bytesToCopy);
    if (!NT_SUCCESS(status)) 
    {
        KatataPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
            "WdfMemoryCopyFromBuffer failed 0x%x\n", status);

        return status;
    }

    //
    // Report how many bytes were copied
    //
    WdfRequestSetInformation(Request, bytesToCopy);

    KatataPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
        "KatataGetReportDescriptor Exit = 0x%x\n", status);

    return status;
}


NTSTATUS
KatataGetDeviceAttributes(
    IN WDFREQUEST Request
    )
{
    NTSTATUS                 status = STATUS_SUCCESS;
    PHID_DEVICE_ATTRIBUTES   deviceAttributes = NULL;

    KatataPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
        "KatataGetDeviceAttributes Entry\n");

    //
    // This IOCTL is METHOD_NEITHER so WdfRequestRetrieveOutputMemory
    // will correctly retrieve buffer from Irp->UserBuffer. 
    // Remember that HIDCLASS provides the buffer in the Irp->UserBuffer
    // field irrespective of the ioctl buffer type. However, framework is very
    // strict about type checking. You cannot get Irp->UserBuffer by using
    // WdfRequestRetrieveOutputMemory if the ioctl is not a METHOD_NEITHER
    // internal ioctl.
    //
    status = WdfRequestRetrieveOutputBuffer(Request,
                                            sizeof (HID_DEVICE_ATTRIBUTES),
                                            &deviceAttributes,
                                            NULL);
    if (!NT_SUCCESS(status)) 
    {
        KatataPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
            "WdfRequestRetrieveOutputBuffer failed 0x%x\n", status);

        return status;
    }

    //
    // Set USB device descriptor
    //

    deviceAttributes->Size = sizeof (HID_DEVICE_ATTRIBUTES);
    deviceAttributes->VendorID = KATATA_VID;
    deviceAttributes->ProductID = KATATA_PID;
    deviceAttributes->VersionNumber = KATATA_VERSION;

    //
    // Report how many bytes were copied
    //
    WdfRequestSetInformation(Request, sizeof (HID_DEVICE_ATTRIBUTES));

    KatataPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
        "KatataGetDeviceAttributes Exit = 0x%x\n", status);

    return status;
}

NTSTATUS
KatataGetString(
    IN WDFREQUEST Request
    )
{
 
    NTSTATUS status = STATUS_SUCCESS;
    PWSTR pwstrID;
    size_t lenID;
    WDF_REQUEST_PARAMETERS params;
    void *pStringBuffer = NULL;

    KatataPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
        "KatataGetString Entry\n");
 
    WDF_REQUEST_PARAMETERS_INIT(&params);
    WdfRequestGetParameters(Request, &params);

    switch ((ULONG_PTR)params.Parameters.DeviceIoControl.Type3InputBuffer & 0xFFFF)
    {
        case HID_STRING_ID_IMANUFACTURER:
            pwstrID = L"DJP Inc.\0";
            break;

        case HID_STRING_ID_IPRODUCT:
            pwstrID = L"Virtual Multitouch Device\0";
            break;

        case HID_STRING_ID_ISERIALNUMBER:
            pwstrID = L"123123123\0";
            break;

        default:
            pwstrID = NULL;
            break;
    }

    lenID = pwstrID ? wcslen(pwstrID)*sizeof(WCHAR) + sizeof(UNICODE_NULL): 0;

    if(pwstrID == NULL)
    {

        KatataPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
            "KatataGetString Invalid request type\n");

        status = STATUS_INVALID_PARAMETER;

        return status;
    }

    status = WdfRequestRetrieveOutputBuffer(Request,
                                            lenID,
                                            &pStringBuffer,
                                            &lenID);

    if(!NT_SUCCESS( status)) 
    {

        KatataPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
            "KatataGetString WdfRequestRetrieveOutputBuffer failed Status 0x%x\n", status);

        return status;
    }

    RtlCopyMemory(pStringBuffer, pwstrID, lenID);

    WdfRequestSetInformation(Request, lenID);

    KatataPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
        "KatataGetString Exit = 0x%x\n", status);
 
    return status;
}

NTSTATUS
KatataWriteReport(
    IN PKATATA_CONTEXT DevContext,
    IN WDFREQUEST Request
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_REQUEST_PARAMETERS params;
    PHID_XFER_PACKET transferPacket = NULL;
    size_t bytesWritten = 0;

    KatataPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
        "KatataWriteReport Entry\n");

    WDF_REQUEST_PARAMETERS_INIT(&params);
    WdfRequestGetParameters(Request, &params);

    if (params.Parameters.DeviceIoControl.InputBufferLength < sizeof(HID_XFER_PACKET)) 
    {
        KatataPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
                "KatataWriteReport Xfer packet too small\n");

        status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {

        transferPacket = (PHID_XFER_PACKET) WdfRequestWdmGetIrp(Request)->UserBuffer;
        
        if (transferPacket == NULL) 
        {
            KatataPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
                "KatataWriteReport No xfer packet\n");

            status = STATUS_INVALID_DEVICE_REQUEST;
        }
        else
        {
            //
            // switch on the report id
            //

            switch (transferPacket->reportId)
            {
                default:

                    KatataPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
                            "KatataWriteReport Unhandled report type %d\n", transferPacket->reportId);

                    status = STATUS_INVALID_PARAMETER;

                    break;
            }
        }
    }

    KatataPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
        "KatataWriteReport Exit = 0x%x\n", status);

    return status;

}

NTSTATUS
KatataReadReport(
    IN PKATATA_CONTEXT DevContext,
    IN WDFREQUEST Request,
    OUT BOOLEAN* CompleteRequest
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_REQUEST_SEND_OPTIONS options;

    KatataPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
        "KatataReadReport Entry\n");

    //
    // Forward request to lower driver
    //

    WdfRequestFormatRequestUsingCurrentType(Request);

    WDF_REQUEST_SEND_OPTIONS_INIT(&options, WDF_REQUEST_SEND_OPTION_SEND_AND_FORGET);

    if (!WdfRequestSend(Request, DevContext->IoTarget, &options))
    {
        status = WdfRequestGetStatus(Request);

        KatataPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
                "WdfRequestSend failed Status 0x%x\n", status);
    }
    else
    {
        *CompleteRequest = FALSE;
    }

    KatataPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
        "KatataReadReport Exit = 0x%x\n", status);

    return status;
}

NTSTATUS
KatataSetFeature(
    IN PKATATA_CONTEXT DevContext,
    IN WDFREQUEST Request,
    OUT BOOLEAN* CompleteRequest
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_REQUEST_PARAMETERS params;
    PHID_XFER_PACKET transferPacket = NULL;
    KatataFeatureReport* pReport = NULL;

    KatataPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
        "KatataSetFeature Entry\n");

    WDF_REQUEST_PARAMETERS_INIT(&params);
    WdfRequestGetParameters(Request, &params);

    if (params.Parameters.DeviceIoControl.InputBufferLength < sizeof(HID_XFER_PACKET)) 
    {
        KatataPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
                "KatataSetFeature Xfer packet too small\n");

        status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {

        transferPacket = (PHID_XFER_PACKET) WdfRequestWdmGetIrp(Request)->UserBuffer;
        
        if (transferPacket == NULL) 
        {
            KatataPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
                "KatataWriteReport No xfer packet\n");

            status = STATUS_INVALID_DEVICE_REQUEST;
        }
        else
        {
            //
            // switch on the report id
            //

            switch (transferPacket->reportId)
            {
                case REPORTID_FEATURE:

                    if (transferPacket->reportBufferLen == sizeof(KatataFeatureReport))
                    {
                        pReport = (KatataFeatureReport*) transferPacket->reportBuffer;

                        DevContext->DeviceMode = pReport->DeviceMode;

                        KatataPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
                            "KatataSetFeature DeviceMode = 0x%x\n", DevContext->DeviceMode);
                    }
                    else
                    {
                        status = STATUS_INVALID_PARAMETER;

                        KatataPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
                                "KatataSetFeature Error transferPacket->reportBufferLen (%d) is different from sizeof(KatataFeatureReport) (%d)\n", 
                                transferPacket->reportBufferLen,
                                sizeof(KatataFeatureReport));
                    }

                    break;

                default:

                    KatataPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
                            "KatataSetFeature Unhandled report type %d\n", transferPacket->reportId);

                    status = STATUS_INVALID_PARAMETER;

                    break;
            }
        }
    }

    KatataPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
        "KatataSetFeature Exit = 0x%x\n", status);

    return status;
}

NTSTATUS
KatataGetFeature(
    IN PKATATA_CONTEXT DevContext,
    IN WDFREQUEST Request,
    OUT BOOLEAN* CompleteRequest
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    WDF_REQUEST_PARAMETERS params;
    PHID_XFER_PACKET transferPacket = NULL;

    KatataPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
        "KatataGetFeature Entry\n");

    WDF_REQUEST_PARAMETERS_INIT(&params);
    WdfRequestGetParameters(Request, &params);

    if (params.Parameters.DeviceIoControl.OutputBufferLength < sizeof(HID_XFER_PACKET)) 
    {
        KatataPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
                "KatataGetFeature Xfer packet too small\n");

        status = STATUS_BUFFER_TOO_SMALL;
    }
    else
    {

        transferPacket = (PHID_XFER_PACKET) WdfRequestWdmGetIrp(Request)->UserBuffer;
        
        if (transferPacket == NULL) 
        {
            KatataPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
                "KatataGetFeature No xfer packet\n");

            status = STATUS_INVALID_DEVICE_REQUEST;
        }
        else
        {
            //
            // switch on the report id
            //

            switch (transferPacket->reportId)
            {
                case REPORTID_MTOUCH:
                {

                    KatataMaxCountReport* pReport = NULL;

                    if (transferPacket->reportBufferLen == sizeof(KatataMaxCountReport))
                    {
                        pReport = (KatataMaxCountReport*) transferPacket->reportBuffer;

                        pReport->MaximumCount = MULTI_MAX_COUNT;

                        KatataPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
                            "KatataGetFeature MaximumCount = 0x%x\n", MULTI_MAX_COUNT);
                    }
                    else
                    {
                        status = STATUS_INVALID_PARAMETER;

                        KatataPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
                                "KatataGetFeature Error transferPacket->reportBufferLen (%d) is different from sizeof(KatataMaxCountReport) (%d)\n", 
                                transferPacket->reportBufferLen,
                                sizeof(KatataMaxCountReport));
                    }

                    break;
                }

                case REPORTID_FEATURE:
                {

                    KatataFeatureReport* pReport = NULL;

                    if (transferPacket->reportBufferLen == sizeof(KatataFeatureReport))
                    {
                        pReport = (KatataFeatureReport*) transferPacket->reportBuffer;

                        pReport->DeviceMode = DevContext->DeviceMode;

                        pReport->DeviceIdentifier = 0;

                        KatataPrint(DEBUG_LEVEL_INFO, DBG_IOCTL,
                            "KatataGetFeature DeviceMode = 0x%x\n", DevContext->DeviceMode);
                    }
                    else
                    {
                        status = STATUS_INVALID_PARAMETER;

                        KatataPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
                                "KatataGetFeature Error transferPacket->reportBufferLen (%d) is different from sizeof(KatataFeatureReport) (%d)\n", 
                                transferPacket->reportBufferLen,
                                sizeof(KatataFeatureReport));
                    }

                    break;
                }

                default:

                    KatataPrint(DEBUG_LEVEL_ERROR, DBG_IOCTL,
                            "KatataGetFeature Unhandled report type %d\n", transferPacket->reportId);

                    status = STATUS_INVALID_PARAMETER;

                    break;
            }
        }
    }

    KatataPrint(DEBUG_LEVEL_VERBOSE, DBG_IOCTL,
        "KatataGetFeature Exit = 0x%x\n", status);

    return status;
}

PCHAR
DbgHidInternalIoctlString(
    IN ULONG IoControlCode
    )
{
    switch (IoControlCode)
    {
    case IOCTL_HID_GET_DEVICE_DESCRIPTOR:
        return "IOCTL_HID_GET_DEVICE_DESCRIPTOR";
    case IOCTL_HID_GET_REPORT_DESCRIPTOR:
        return "IOCTL_HID_GET_REPORT_DESCRIPTOR";
    case IOCTL_HID_READ_REPORT:
        return "IOCTL_HID_READ_REPORT";
    case IOCTL_HID_GET_DEVICE_ATTRIBUTES:
        return "IOCTL_HID_GET_DEVICE_ATTRIBUTES";
    case IOCTL_HID_WRITE_REPORT:
        return "IOCTL_HID_WRITE_REPORT";
    case IOCTL_HID_SET_FEATURE:
        return "IOCTL_HID_SET_FEATURE";
    case IOCTL_HID_GET_FEATURE:
        return "IOCTL_HID_GET_FEATURE";
    case IOCTL_HID_GET_STRING:
        return "IOCTL_HID_GET_STRING";
    case IOCTL_HID_ACTIVATE_DEVICE:
        return "IOCTL_HID_ACTIVATE_DEVICE";
    case IOCTL_HID_DEACTIVATE_DEVICE:
        return "IOCTL_HID_DEACTIVATE_DEVICE";
    case IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST:
        return "IOCTL_HID_SEND_IDLE_NOTIFICATION_REQUEST";
    case IOCTL_HID_SET_OUTPUT_REPORT:
        return "IOCTL_HID_SET_OUTPUT_REPORT";
    case IOCTL_HID_GET_INPUT_REPORT:
        return "IOCTL_HID_GET_INPUT_REPORT";
    default:
        return "Unknown IOCTL";
    }
}

