#include "VDJTouchEngine.h"

// This is the standard DLL loader for COM object.

HRESULT VDJ_API DllGetClassObject(const GUID& rclsid, const GUID& riid, void** ppObject)
{
	if (memcmp(&rclsid, &CLSID_VdjPlugin8, sizeof(GUID)) == 0 && memcmp(&riid, &IID_IVdjPluginBasic8, sizeof(GUID)) == 0)
	{
		*ppObject = new CMyPlugin8();
	}
	else
	{
		return CLASS_E_CLASSNOTAVAILABLE;
	}

	return NO_ERROR;
}