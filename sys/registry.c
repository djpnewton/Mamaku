#include "registry.h"
#include "trace.h"

static ULONG UseMultitouch = 0;
static ULONG DragThreshold = 20;

VOID MamakuRegistryInit(IN WDFDRIVER Driver)
{
    NTSTATUS status;
    WDFKEY key;
    DECLARE_CONST_UNICODE_STRING(UseMultitouchName, L"UseMultitouchDebug");
    DECLARE_CONST_UNICODE_STRING(DragThresholdName, L"DragThreshold");

    status = WdfDriverOpenParametersRegistryKey(Driver, STANDARD_RIGHTS_ALL, WDF_NO_OBJECT_ATTRIBUTES, &key);

    if (!NT_SUCCESS(status)) 
    {
        MamakuPrint(DEBUG_LEVEL_ERROR, DBG_INIT,
            "WdfDriverOpenParametersRegistryKey failed with status 0x%x\n", status);

        return;
    }

    status = WdfRegistryQueryULong(key, &UseMultitouchName, &UseMultitouch);

    if (!NT_SUCCESS(status)) 
    {
        MamakuPrint(DEBUG_LEVEL_ERROR, DBG_INIT,
            "WdfRegistryQueryULong(UseMultitouch) failed with status 0x%x\n", status);
    }

    MamakuPrint(DEBUG_LEVEL_ERROR, DBG_INIT,
        "UseMultitouchDebug = %d\n", UseMultitouch);

    status = WdfRegistryQueryULong(key, &DragThresholdName, &DragThreshold);

    if (!NT_SUCCESS(status)) 
    {
        MamakuPrint(DEBUG_LEVEL_ERROR, DBG_INIT,
            "WdfRegistryQueryULong(DragThreshold) failed with status 0x%x\n", status);
    }

    MamakuPrint(DEBUG_LEVEL_ERROR, DBG_INIT,
        "DragThreshold = %d\n", DragThreshold);

    WdfRegistryClose(key);
}

BOOLEAN MamakuRegistryGetUseMultitouch(void)
{
    return (BOOLEAN)UseMultitouch;
}

ULONG MamakuRegistryGetDragThreshold(void)
{
    return DragThreshold;
}
