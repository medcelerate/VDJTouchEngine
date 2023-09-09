#include "VDJTouchEngine.h"

HRESULT VDJ_API VDJTouchEngine::OnLoad()
{
	// ADD YOUR CODE HERE WHEN THE PLUGIN IS CALLED
	pFileButton = 0;


	DeclareParameterButton(&pFileButton, ID_BUTTON_1, "Load", "Load");


	return S_OK;
}
//-----------------------------------------------------------------------------
HRESULT VDJ_API VDJTouchEngine::OnGetPluginInfo(TVdjPluginInfo8* infos)
{
	infos->PluginName = "VDjTouchEngine";
	infos->Author = "Evan Clark";
	infos->Description = "Loads tox files as video FX";
	infos->Version = "1.0";
	infos->Flags = 0x00;
	infos->Bitmap = NULL;

	return S_OK;
}
//---------------------------------------------------------------------------
ULONG VDJ_API VDJTouchEngine::Release()
{
	// ADD YOUR CODE HERE WHEN THE PLUGIN IS RELEASED

	delete this;
	return 0;
}
//---------------------------------------------------------------------------
HRESULT VDJ_API VDJTouchEngine::OnGetUserInterface(TVdjPluginInterface8* pluginInterface)
{
	pluginInterface->Type = VDJINTERFACE_DEFAULT;

	return S_OK;
}
//---------------------------------------------------------------------------
HRESULT VDJ_API VDJTouchEngine::OnParameter(int id)
{
	switch (id)
	{
	case ID_BUTTON_1:
		if (pFileButton == 1)
		{
			// Function to open file dialog and return path
		}
		break;
	}

	return S_OK;
}
//---------------------------------------------------------------------------
HRESULT VDJ_API VDJTouchEngine::OnGetParameterString(int id, char* outParam, int outParamSize)
{

	return S_OK;
}

//-------------------------------------------------------------------------------------------------------------------------------------
// BELOW, ADDITIONAL FUNCTIONS ONLY TO EXPLAIN SOME FEATURES (CAN BE REMOVED)
//-------------------------------------------------------------------------------------------------------------------------------------

HRESULT VDJ_API VDJTouchEngine::OnDeviceInit() {

	HRESULT hr;
	hr = GetDevice(VdjVideoEngineDirectX11, (void**)&D3DDevice);
	if (FAILED(hr))
	{
		return hr;
	}

	VideoWidth = width;
	VideoHeight = height;

	return S_OK;
}

HRESULT VDJ_API VDJTouchEngine::OnDeviceClose() {

	if (instance != nullptr)
	{
		TERelease(&instance);
		instance = nullptr;
		D3DContext = nullptr;
		D3DDevice = nullptr;
	}

	return S_OK;
}

HRESULT VDJ_API VDJTouchEngine::OnDraw() {

	HRESULT hr;
	TVertex *verts = nullptr;


	if (VideoWidth != width || VideoHeight != height)
	{
		hr = OnVideoResize(width, height);
	}

	ID3D11ShaderResourceView* textureView = nullptr; //GetTexture doesn't AddRef, so doesn't need to be released
	hr = GetTexture(VdjVideoEngineDirectX11, (void**)&textureView, &verts);
	CComPtr<ID3D11DeviceContext> devContext; //use smart pointer to automatically release pointer and prevent memory leak
	D3DDevice->GetImmediateContext(&devContext.p);

	return S_OK;
}
HRESULT VDJTouchEngine::OnVideoResize(int VidWidth, int VidHeight)
{

	VideoWidth = width;
	VideoHeight = height;

	return S_OK;
}


bool VDJTouchEngine::OpenFileDialog()
{
	
	return true;
}

bool VDJTouchEngine::LoadTEFile()
{
	// Load the tox file into the TouchEngine
	// 1. Create a TouchEngine object

	if (instance != nullptr)
	{


		TEResult res = TEInstanceCreate(eventCallbackStatic, linkCallbackStatic, this, &instance);
		if (res != TEResultSuccess)
		{
			return false;
		}
		res = TED3D11ContextCreate(D3DDevice, &D3DContext);
		if (res != TEResultSuccess)
		{
			return false;
		}

		res = TEInstanceAssociateGraphicsContext(instance, D3DContext);
		if (res != TEResultSuccess)
		{
			return false;
		}
	}

	// 2. Load the tox file into the TouchEngine
	TEResult res = TEInstanceConfigure(instance, filePath.c_str(), TETimeExternal);
	if (res != TEResultSuccess)
	{
		return false;
	}

	res = TEInstanceLoad(instance);
	if (res != TEResultSuccess)
	{
		return false;
	}
	// Need to check for in and out params here. If there are none, then it is not an FX.
	// If there are, then it is an FX and we need to set the in and out params.



	res = TEInstanceResume(instance);

	return true;
}

void VDJTouchEngine::eventCallbackStatic(TEInstance* instance, TEEvent event, TEResult result, int64_t start_time_value, int32_t start_time_scale, int64_t end_time_value, int32_t end_time_scale, void* info)
{
	// This is the callback function that will be called by the TouchEngine when an event occurs
	// 1. Get the instance of the VDJTouchEngine
	// 2. Call the eventCallback function
}

void VDJTouchEngine::linkCallbackStatic(TEInstance* instance, TELinkEvent event, const char* identifier, void* info)
{
	// This is the callback function that will be called by the TouchEngine when a link event occurs
	// 1. Get the instance of the VDJTouchEngine
	// 2. Call the linkCallback function
}