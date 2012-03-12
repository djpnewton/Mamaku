#include "registry.h"
#include "trace.h"

static ULONG UseMultitouch;

VOID MamakuRegistryInit(IN WDFDRIVER Driver)
{
    NTSTATUS status;
    WDFKEY key;
    DECLARE_CONST_UNICODE_STRING(UseMultitouchName, L"UseMultitouchDebug");

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
            "WdfRegistryQueryULong failed with status 0x%x\n", status);

        goto cleanup;
    }

cleanup:

    WdfRegistryClose(key);
}

BOOLEAN MamakuRegistryGetUseMultitouch(void)
{
    return (BOOLEAN)UseMultitouch;
}
