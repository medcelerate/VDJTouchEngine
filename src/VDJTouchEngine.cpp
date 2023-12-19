#include "VDJTouchEngine.h"

VDJTouchEngine::VDJTouchEngine()
{
	while (!::IsDebuggerPresent())
		::Sleep(100);
	int k = 1;
	k++;
}

VDJTouchEngine::~VDJTouchEngine()
{
	return;
}

HRESULT VDJ_API VDJTouchEngine::OnLoad()
{
	while (!::IsDebuggerPresent())
		::Sleep(100);
	// ADD YOUR CODE HERE WHEN THE PLUGIN IS CALLED
	pFileButton = 0;


	DeclareParameterButton(&pFileButton, ID_BUTTON_1, "Load", "Load");


	return S_OK;
}
//-----------------------------------------------------------------------------
HRESULT VDJ_API VDJTouchEngine::OnGetPluginInfo(TVdjPluginInfo8* infos)
{
	
	infos->PluginName = "VDJTouchEngine";
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
/*
//---------------------------------------------------------------------------
HRESULT VDJ_API VDJTouchEngine::OnGetUserInterface(TVdjPluginInterface8* pluginInterface)
{
	pluginInterface->Type = VDJINTERFACE_DEFAULT;

	return S_OK;
}

*/
//---------------------------------------------------------------------------
HRESULT VDJ_API VDJTouchEngine::OnParameter(int id)
{
	switch (id)
	{
	case ID_BUTTON_1:
		if (pFileButton == 1)
		{
			OpenFileDialog();
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
		TEInstanceSuspend(instance);
		TEInstanceUnload(instance);
		TERelease(&TEInput);
		TERelease(&TEOutput);
		TERelease(&TEOutputTexture);
		TERelease(&instance);
		instance = nullptr;
		D3DContext = nullptr;
		D3DDevice = nullptr;
	}

	return S_OK;
}

HRESULT VDJ_API VDJTouchEngine::OnDraw() {

	HRESULT hr;
	TVertex* verts = nullptr;

	//Need to set this up.
	auto result = TEInstanceStartFrameAtTime(instance, 0, 6000, false);

	if (VideoWidth != width || VideoHeight != height)
	{
		hr = OnVideoResize(width, height);
	}

	ID3D11ShaderResourceView* textureView = nullptr; //GetTexture doesn't AddRef, so doesn't need to be released
	hr = GetTexture(VdjVideoEngineDirectX11, (void**)&textureView, &verts);


	//Hving issues loading CComptr here, need to try to resolve this.
	Microsoft::WRL::ComPtr<ID3D11DeviceContext> devContext; //use smart pointer to automatically release pointer and prevent memory leak
	D3DDevice->GetImmediateContext(&devContext);

	Microsoft::WRL::ComPtr<ID3D11Resource> textureResource;
	textureView->GetResource(&textureResource);
	if (!textureResource) {
		return E_FAIL;
	}
	Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;

	textureResource->QueryInterface<ID3D11Texture2D>(&texture);

	if (!texture)
		return E_FAIL;
	D3D11_TEXTURE2D_DESC textureDesc;
	texture->GetDesc(&textureDesc);


	devContext->CopyResource(D3DTextureInput, texture.Get());

	//Need to setup D3D fence from TD in order to initialize texture transfer.

	//Need to texture transfer out.


	return S_OK;
}
HRESULT VDJTouchEngine::OnVideoResize(int VidWidth, int VidHeight)
{

	VideoWidth = width;
	VideoHeight = height;

	return S_OK;
}

HRESULT VDJ_API VDJTouchEngine::OnStart()
{
	return NO_ERROR;
}

HRESULT VDJ_API VDJTouchEngine::OnStop()
{
	return NO_ERROR;
}


bool VDJTouchEngine::OpenFileDialog()
{
	IFileOpenDialog* pFileOpen;

	// Create the FileOpenDialog object.
	HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_ALL,
		IID_IFileOpenDialog, reinterpret_cast<void**>(&pFileOpen));

	if (SUCCEEDED(hr)) {
		hr = pFileOpen->Show(nullptr);

		if (SUCCEEDED(hr)) {
			IShellItem* pItem;
			hr = pFileOpen->GetResult(&pItem);
			if (SUCCEEDED(hr)) {
				LPWSTR filePathW;
				hr = pItem->GetDisplayName(SIGDN_FILESYSPATH, &filePathW);
				if (SUCCEEDED(hr)) {
					std::wstring filePathWStr(filePathW);
					filePath = std::string(filePathWStr.begin(), filePathWStr.end());
					CoTaskMemFree(filePathW);
				}
				pItem->Release();
			}
		}
	}
	return true;
}

bool VDJTouchEngine::LoadTEFile()
{
	// Load the tox file into the TouchEngine
	// 1. Create a TouchEngine object

	if (instance == nullptr)
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

	TEInput = TED3D11TextureCreate(D3DTextureInput, TETextureOriginTopLeft, kTETextureComponentMapIdentity, nullptr, nullptr);
	res = TEInstanceLinkSetTextureValue(instance, "input", TEInput, D3DContext);

	if (res != TEResultSuccess)
	{
		isFX = false;
	}
	else
	{
		isFX = true;
		res = TEInstanceLinkGetTextureValue(instance, "output", TELinkValueCurrent, &TEOutputTexture);

		if (res != TEResultSuccess)
		{
			return false;
		}

		if (TEOutputTexture != nullptr) {
			if (TETextureGetType(TEOutputTexture) == TETextureTypeD3DShared)
			{
				res = TED3D11ContextGetTexture(D3DContext, (TED3DSharedTexture*)TEOutputTexture, &TEOutput);
				if (res != TEResultSuccess)
				{
					return false;
				}
			}
			else
			{
				return false;
			}
		}
	}

	res = TEInstanceResume(instance);

	if (res != TEResultSuccess)
	{
		return false;
	}

	return true;
}

void VDJTouchEngine::eventCallback(TEEvent event, TEResult result, int64_t start_time_value, int32_t start_time_scale, int64_t end_time_value, int32_t end_time_scale)
{
	switch (event) {
		case TEEventInstanceDidLoad:
			isLoaded = true;
			// The tox file has been loaded into the TouchEngine
			break;
		case TEEventFrameDidFinish:
			std::unique_lock<std::mutex> lock(frameMutex);
			isFrameBusy = false;
			// A frame has finished rendering
			break;
	}
}

void VDJTouchEngine::linkCallback(TELinkEvent event, const char* identifier)
{
	switch (event) {
		case TELinkEventAdded:
			// A link has been added
			break;

	}
}

void VDJTouchEngine::eventCallbackStatic(TEInstance* instance, TEEvent event, TEResult result, int64_t start_time_value, int32_t start_time_scale, int64_t end_time_value, int32_t end_time_scale, void* info)
{
	static_cast<VDJTouchEngine*>(info)->eventCallback(event, result, start_time_value, start_time_scale, end_time_value, end_time_scale);
}

void VDJTouchEngine::linkCallbackStatic(TEInstance* instance, TELinkEvent event, const char* identifier, void* info)
{
	static_cast<VDJTouchEngine*>(info)->linkCallback(event, identifier);
}