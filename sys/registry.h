#ifndef _REGISTRY_H_
#define _REGISTRY_H_

#include <wdm.h>
#include <wdf.h>

VOID MamakuRegistryInit(IN WDFDRIVER Driver);

BOOLEAN MamakuRegistryGetUseMultitouch(void);

#endif
