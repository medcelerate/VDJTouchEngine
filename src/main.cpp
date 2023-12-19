#include "VDJTouchEngine.h"

VDJ_EXPORT HRESULT VDJ_API DllGetClassObject(const GUID& rclsid, const GUID& riid, void** ppObject)
{
	if (memcmp(&rclsid, &CLSID_VdjPlugin8, sizeof(GUID)) == 0 && memcmp(&riid, &IID_IVdjPluginVideoFx8, sizeof(GUID)) == 0)
	{
		*ppObject = new VDJTouchEngine();
	}
	else
	{
		return CLASS_E_CLASSNOTAVAILABLE;
	}

	return NO_ERROR;
}


