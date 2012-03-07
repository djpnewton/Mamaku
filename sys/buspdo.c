#include <ntddk.h>
#include <wdf.h>
#include <Hidport.h>

#include "mamaku.h"

#define BUS_HARDWARE_IDS L"DJPSOFT\\MamakuTest\0"
#define BUS_HARDWARE_IDS_LENGTH sizeof(BUS_HARDWARE_IDS)
#define BUS_COMPATIBLE_IDS L"DJPSOFT\\MamakuCompatibleTest\0"
#define BUS_COMPATIBLE_IDS_LENGTH sizeof(BUS_COMPATIBLE_IDS)
#define BUS_DEVICE_LOCATION L"MamakuTest Bus 0"
#define BUS_DEVICE_INSTANCEID L"0x7337"
#define BUS_DEVICE_TEXT L"MamakuTest "BUS_DEVICE_INSTANCEID
#define BUS_DEVICE_SERIALNUM 1

extern ULONG MamakuDebugLevel;
extern ULONG MamakuDebugCatagories;

NTSTATUS
MamakuCreatePdo(
    WDFDEVICE Device,
    OUT WDFDEVICE* Child
    )
{
    NTSTATUS status = STATUS_SUCCESS;
    PWDFDEVICE_INIT pDeviceInit;
    DECLARE_CONST_UNICODE_STRING(deviceId, BUS_HARDWARE_IDS);
    DECLARE_CONST_UNICODE_STRING(compatId, BUS_COMPATIBLE_IDS);
    DECLARE_CONST_UNICODE_STRING(deviceLocation, BUS_DEVICE_LOCATION);
    DECLARE_CONST_UNICODE_STRING(instanceId, BUS_DEVICE_INSTANCEID);
    DECLARE_CONST_UNICODE_STRING(deviceText, BUS_DEVICE_TEXT);
    WDF_DEVICE_PNP_CAPABILITIES pnpCaps;

    *Child = NULL;

    pDeviceInit = WdfPdoInitAllocate(Device);
    if (pDeviceInit == NULL)
    {
        MamakuPrint(DEBUG_LEVEL_INFO, DBG_PNP,
                "WdfPdoInitAllocate failed. Status: 0x%x\n", status);

        status = STATUS_INSUFFICIENT_RESOURCES;

        goto Cleanup;
    }

    //
    // Set DeviceType
    //
    WdfDeviceInitSetDeviceType(pDeviceInit, FILE_DEVICE_UNKNOWN);

    //
    // Provide DeviceID, HardwareIDs, CompatibleIDs and InstanceId
    //
    status = WdfPdoInitAssignDeviceID(pDeviceInit, &deviceId);
    if (!NT_SUCCESS(status))
    {
        MamakuPrint(DEBUG_LEVEL_INFO, DBG_PNP,
                "WdfPdoInitAssignDeviceID failed. Status: 0x%x\n", status);
        goto Cleanup;
    }

    //
    // Note same string  is used to initialize hardware id too
    //
    status = WdfPdoInitAddHardwareID(pDeviceInit, &deviceId);
    if (!NT_SUCCESS(status))
    {
        MamakuPrint(DEBUG_LEVEL_INFO, DBG_PNP,
                "WdfPdoInitAssignDeviceID failed. Status: 0x%x\n", status);
        goto Cleanup;
    }

    status = WdfPdoInitAddCompatibleID(pDeviceInit, &compatId);
    if (!NT_SUCCESS(status))
    {
        MamakuPrint(DEBUG_LEVEL_INFO, DBG_PNP,
                "WdfPdoInitAssignDeviceID failed. Status: 0x%x\n", status);
        goto Cleanup;
    }

    status = WdfPdoInitAssignInstanceID(pDeviceInit, &instanceId);
    if (!NT_SUCCESS(status))
    {
        MamakuPrint(DEBUG_LEVEL_INFO, DBG_PNP,
                "WdfPdoInitAssignDeviceID failed. Status: 0x%x\n", status);
        goto Cleanup;
    }

    //
    // Provide a description about the device. This text is usually read from
    // the device. In the case of USB device, this text comes from the string
    // descriptor. This text is displayed momentarily by the PnP manager while
    // it's looking for a matching INF. If it finds one, it uses the Device
    // Description from the INF file or the friendly name created by
    // coinstallers to display in the device manager. FriendlyName takes
    // precedence over the DeviceDesc from the INF file.
    //
    // You can call WdfPdoInitAddDeviceText multiple times, adding device
    // text for multiple locales. When the system displays the text, it
    // chooses the text that matches the current locale, if available.
    // Otherwise it will use the string for the default locale.
    // The driver can specify the driver's default locale by calling
    // WdfPdoInitSetDefaultLocale.
    //
    status = WdfPdoInitAddDeviceText(pDeviceInit,
                                        &deviceText,
                                        &deviceLocation,
                                        0x409);
    if (!NT_SUCCESS(status))
    {
        MamakuPrint(DEBUG_LEVEL_INFO, DBG_PNP,
                "WdfPdoInitAddDeviceText failed. Status: 0x%x\n", status);
        goto Cleanup;
    }

    WdfPdoInitSetDefaultLocale(pDeviceInit, 0x409);

    //
    // Create the device now
    //
    status = WdfDeviceCreate(&pDeviceInit,
                             WDF_NO_OBJECT_ATTRIBUTES,
                             Child);
    if (!NT_SUCCESS(status))
    {
        MamakuPrint(DEBUG_LEVEL_INFO, DBG_PNP,
                "WdfDeviceCreate failed. Status: 0x%x\n", status);
        goto Cleanup;
    }

    //
    // Set some properties for the child device.
    //
    WDF_DEVICE_PNP_CAPABILITIES_INIT(&pnpCaps);
    pnpCaps.Removable         = WdfTrue;
    pnpCaps.EjectSupported    = WdfTrue;
    pnpCaps.SurpriseRemovalOK = WdfTrue;

    pnpCaps.Address  = BUS_DEVICE_SERIALNUM;
    pnpCaps.UINumber = BUS_DEVICE_SERIALNUM;

    WdfDeviceSetPnpCapabilities(*Child, &pnpCaps);

    //
    // Add this device to the FDO's collection of children.
    //
    status = WdfFdoAddStaticChild(Device,
                                  *Child);
    if (!NT_SUCCESS (status)) 
    {
        MamakuPrint(DEBUG_LEVEL_INFO, DBG_PNP,
                "WdfFdoAddStaticChild failed. Status: 0x%x\n", status);
        goto Cleanup;
    }

Cleanup:

    //
    // Call WdfDeviceInitFree if you encounter an error before the
    // device is created. Once the device is created, framework
    // NULLs the pDeviceInit value.
    //
    if (pDeviceInit != NULL)
    {
        WdfDeviceInitFree(pDeviceInit);
    }

    if (*Child)
    {
        WdfObjectDelete(*Child);
    }

    return status;
}
