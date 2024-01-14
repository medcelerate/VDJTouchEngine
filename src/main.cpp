#include "VDJTouchEngine.h"

VDJ_EXPORT HRESULT VDJ_API DllGetClassObject(const GUID& rclsid, const GUID& riid, void** ppObject)
{
	char* buffer = nullptr;
	size_t size = 0;
	_dupenv_s(&buffer, &size, "USERPROFILE");
	
	if (buffer == nullptr)
	{
		return CLASS_E_CLASSNOTAVAILABLE;
	}

	std::string userpath (buffer);
	free(buffer);

	std::string path = userpath + "\\AppData\\Local\\VirtualDJ\\Plugins64\\VideoEffect\\TouchEngine.dll";

	HMODULE hProcDLL = LoadLibraryA(path.c_str());

	if (hProcDLL == nullptr)
	{
		return CLASS_E_CLASSNOTAVAILABLE;
	}

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


