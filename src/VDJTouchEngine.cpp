#include "VDJTouchEngine.h"

VDJTouchEngine::VDJTouchEngine()
{
}

VDJTouchEngine::~VDJTouchEngine()
{
	return;
}

HRESULT VDJ_API VDJTouchEngine::OnLoad()
{

	// ADD YOUR CODE HERE WHEN THE PLUGIN IS CALLED
	pFileButton = 0;


	DeclareParameterButton(&pFileButton, ID_BUTTON_1, "Load", "Load");



	if (instance == nullptr)
	{

		LoadTouchEngine();
	}


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

	LoadTEGraphicsContext(true);

	CreateTexture();

	//Attach texture to shader

	LoadTEFile();
	return S_OK;
}

HRESULT VDJ_API VDJTouchEngine::OnDeviceClose() {
	
	if (instance != nullptr)
	{
		TEInstanceSuspend(instance);
		TEInstanceUnload(instance);
		D3DContext.reset();
		D3DDevice->Release();
	}
	
	return S_OK;
}

HRESULT VDJ_API VDJTouchEngine::OnDraw() {

	HRESULT hr;

	TVertex* vertices = nullptr;

	if (!isTouchEngineReady) {
		std::unique_lock<std::mutex> lock(frameMutex);
		isTouchFrameBusy = false;
		return S_FALSE;
	}

	if (VideoWidth != width || VideoHeight != height)
	{
		hr = OnVideoResize(width, height);

	}


	ID3D11ShaderResourceView* textureView = nullptr; //GetTexture doesn't AddRef, so doesn't need to be released
	hr = GetTexture(VdjVideoEngineDirectX11, (void**)&textureView, nullptr);

	if (FAILED(hr))
	{
		return hr;
	}


	Microsoft::WRL::ComPtr<ID3D11DeviceContext> devContext; //use smart pointer to automatically release pointer and prevent memory leak
	D3DDevice->GetImmediateContext(&devContext);

	if (devContext == nullptr) {
		return E_FAIL;
	}


	Microsoft::WRL::ComPtr<ID3D11Resource> textureResource;
	textureView->GetResource(&textureResource);

	if (textureResource == nullptr) {
		return E_FAIL;
	}

	Microsoft::WRL::ComPtr<ID3D11Texture2D> texture;

	hr = textureResource->QueryInterface<ID3D11Texture2D>(&texture);

	if (FAILED(hr))
	{
		return hr;
	}

	if (!texture) {
		return E_FAIL;
	}

	hr = DrawDeck();

	if (FAILED(hr))
	{
		return hr;
	}


	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	texture->GetDesc(&desc);


	//devContext->CopyResource(D3DTextureInput, texture.Get());
	devContext->Flush();

	TouchObject<TELinkInfo> linkInfo;

	auto y  = TEInstanceLinkGetInfo(instance, "op/vdjin", linkInfo.take());

	TEVideoInputD3D.take(TED3D11TextureCreate(texture.Get(), TETextureOriginTopLeft, kTETextureComponentMapIdentity, (TED3D11TextureCallback)textureCallback, nullptr));

	TEResult result = TEInstanceLinkSetTextureValue(instance, "op/vdjin", TEVideoInputD3D, D3DContext);

	

	if (result != TEResultSuccess)
	{
		isPluginFX = false;
	}


	isTouchFrameBusy = true;
	result = TEInstanceStartFrameAtTime(instance, frameCount, 60, false);

	if (result != TEResultSuccess)
	{
		std::unique_lock<std::mutex> lock(frameMutex);
		isTouchFrameBusy = false;
		return S_FALSE;
	}


	while (isTouchFrameBusy)
	{

	}

	std::unique_lock<std::mutex> lock(frameMutex);
	isTouchFrameBusy = false;
	lock.unlock();



	result = TEInstanceLinkGetTextureValue(instance, "op/out1", TELinkValueCurrent, TEVideoOutputTexture.take());

	if (result != TEResultSuccess)
	{
		return S_FALSE;
	}


	if (TEVideoOutputTexture != nullptr) {
		if (TETextureGetType(TEVideoOutputTexture) == TETextureTypeD3DShared)
		{
			result = TED3D11ContextGetTexture(D3DContext, static_cast<TED3DSharedTexture*>(TEVideoOutputTexture.get()), TEVideoOutputD3D.take());
			if (result != TEResultSuccess)
			{
				return S_FALSE;
			}
		}
		else
		{
			return S_FALSE;
		}
	}
	devContext->Flush();
	frameCount++;
	return S_OK;
}
HRESULT VDJTouchEngine::OnVideoResize(int VidWidth, int VidHeight)
{

	VideoWidth = width;
	VideoHeight = height;

	CreateTexture();

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
	//LoadTEFile();
	return true;
}

bool VDJTouchEngine::LoadTEFile()
{
	// Load the tox file into the TouchEngine
	// 1. Create a TouchEngine object

	if (instance == nullptr)
	{
		LoadTouchEngine();

	}

	// 2. Load the tox file into the TouchEngine
	TEResult result = TEInstanceConfigure(instance, filePath.c_str(), TETimeExternal);
	if (result != TEResultSuccess)
	{
		return false;
	}

	result = TEInstanceLoad(instance);
	if (result != TEResultSuccess)
	{
		return false;
	}


	while (isTouchEngineLoaded == false) {
		::Sleep(100);
	}

	while (isTouchEngineReady == false)
	{
		::Sleep(100);
	}

	result = TEInstanceResume(instance);

	if (result != TEResultSuccess)
	{
		return false;
	}


	if (TEVideoInputD3D == nullptr) {
		TEVideoInputD3D.take(TED3D11TextureCreate(D3DTextureInput, TETextureOriginTopLeft, kTETextureComponentMapIdentity, nullptr, nullptr));
	}

	/*
	else 
	{
		isFX = true;
		res = TEInstanceLinkGetTextureValue(instance, "op/out1", TELinkValueCurrent, &TEOutputTexture);

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

	*/

	return true;
}

void VDJTouchEngine::LoadTouchEngine() {

	if (instance == nullptr) {

		TEResult result = TEInstanceCreate(eventCallbackStatic, linkCallbackStatic, this, instance.take());
		if (result != TEResultSuccess)
		{
			return;
		}

	}


}

bool VDJTouchEngine::LoadTEGraphicsContext(bool reload) {

	if (instance == nullptr || reload) {

		TEResult result = TED3D11ContextCreate(D3DDevice, D3DContext.take());
		if (result != TEResultSuccess)
		{
			return false;
		}

		result = TEInstanceAssociateGraphicsContext(instance, D3DContext);
		if (result != TEResultSuccess)
		{
			return false;
		}

	}

	return true;

}

HRESULT VDJTouchEngine::CreateTexture() {
	

	if (D3DTextureInput != nullptr)
	{
		D3DTextureInput->Release();
		D3DTextureInput = nullptr;
	}

	D3D11_TEXTURE2D_DESC description = { 0 };
	description.Width = width;
	description.Height = height;
	description.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
	description.Usage = D3D11_USAGE_DEFAULT;
	description.CPUAccessFlags = 0;
	description.MiscFlags = 0;
	description.MipLevels = 0;
	description.ArraySize = 1;
	description.SampleDesc.Count = 1;
	description.SampleDesc.Quality = 0;
	description.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;

	HRESULT hr = D3DDevice->CreateTexture2D(&description, nullptr, &D3DTextureInput);
	if (FAILED(hr))
	{
		switch (hr) {
		case DXGI_ERROR_INVALID_CALL:
			//str += "DXGI_ERROR_INVALID_CALL";
			break;
		case E_INVALIDARG:
		//	str += "E_INVALIDARG";
			break;
		case E_OUTOFMEMORY:
		//	str += "E_OUTOFMEMORY";
			break;
		default:
		//	str += "Unlisted error";
			break;
		}
		return hr;
	}
	/*
//	D3DTextureInput->GetDesc(&description);

	D3D11_SHADER_RESOURCE_VIEW_DESC textureViewDescription;
	ZeroMemory(&textureViewDescription, sizeof(textureViewDescription));
	textureViewDescription.Format = description.Format;
	textureViewDescription.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	textureViewDescription.Texture2D.MipLevels = description.MipLevels;
	textureViewDescription.Texture2D.MostDetailedMip = 0;

	hr = D3DDevice->CreateShaderResourceView(D3DTextureInput, &textureViewDescription, &myTextureView);
	if (FAILED(hr))
	{
		return hr;
	}

	D3D11_SAMPLER_DESC samplerDescription;
	ZeroMemory(&samplerDescription, sizeof(samplerDescription));

	samplerDescription.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;

	samplerDescription.MaxAnisotropy = 0;
	samplerDescription.AddressU = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDescription.AddressV = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDescription.AddressW = D3D11_TEXTURE_ADDRESS_WRAP;
	samplerDescription.MipLODBias = 0.0f;
	samplerDescription.MinLOD = 0;
	samplerDescription.MaxLOD = D3D11_FLOAT32_MAX;
	samplerDescription.ComparisonFunc = D3D11_COMPARISON_NEVER;
	samplerDescription.BorderColor[0] = 0.0f;
	samplerDescription.BorderColor[1] = 0.0f;
	samplerDescription.BorderColor[2] = 0.0f;
	samplerDescription.BorderColor[3] = 0.0f;

	hr = D3DDevice->CreateSamplerState(&samplerDescription, &mySampler);
	if (FAILED(hr))
	{
		return hr;
	}
	*/

	return S_OK;
}

void VDJTouchEngine::eventCallback(TEEvent event, TEResult result, int64_t start_time_value, int32_t start_time_scale, int64_t end_time_value, int32_t end_time_scale)
{
	if (result == TEResultComponentErrors)
	{
		TouchObject<TEErrorArray> errors;
		auto f = TEInstanceGetErrors(instance, errors.take());
		auto y = "";
		// The TouchEngine has encountered an error
		// You can get the error message with TEInstanceGetError
	}
	switch (event) {
		case TEEventInstanceDidLoad:
			if (LoadTEGraphicsContext()) {
				isTouchEngineLoaded = true;
			}
			// The tox file has been loaded into the TouchEngine
			break;
		case TEEventFrameDidFinish: {
			std::unique_lock<std::mutex> lock(frameMutex);
			isTouchFrameBusy = false;
			// A frame has finished rendering
			break;
		}
		case TEEventInstanceReady:
			isTouchEngineReady = true;
		// The TouchEngine is ready to start rendering frames
			break;

		case TEEventInstanceDidUnload:
			isTouchEngineLoaded = false;
			break;
	}
}

void VDJTouchEngine::linkCallback(TELinkEvent event, const char* identifier)
{
	auto y = event;
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

void VDJTouchEngine::textureCallback(TED3D11Texture* texture, TEObjectEvent event, void* info) {
	auto y = event;
	return;
}