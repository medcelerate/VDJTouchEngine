#include "VDJTouchEngine.h"

std::string GetLogFilePath() {
	char* buffer = nullptr;
	size_t size = 0;
	_dupenv_s(&buffer, &size, "USERPROFILE");

	if (buffer == nullptr)
	{
		return std::string();
	}

	std::string userpath(buffer);
	free(buffer);

	std::string path = userpath + "\\AppData\\Local\\VirtualDJ\\Plugins64\\VideoEffect\\TouchEngine.log";

	return path;
}

std::string GetSeverityString(TESeverity severity) {
	switch (severity)
	{
	case TESeverityWarning:
		return "Warning";
	case TESeverityError:
		return "Error";
	default:
		return "Unknown";
	}

}

VDJTouchEngine::VDJTouchEngine()
{
	logger = spdlog::basic_logger_mt("VDJTouchEngine", GetLogFilePath());
}

VDJTouchEngine::~VDJTouchEngine()
{
	return;
}

HRESULT VDJ_API VDJTouchEngine::OnLoad()
{

	// ADD YOUR CODE HERE WHEN THE PLUGIN IS CALLED
	pFileButton = 0;
	pTouchReloadButton = 0;

	DeclareParameterButton(&pFileButton, 0, "Load", "Load");
	DeclareParameterButton(&pTouchReloadButton, 1, "Reload", "Reload");

	totalSamples = -1 * SampleRate;

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
	if (id == 0) {
		if (pFileButton == 1)
		{
			OpenFileDialog();
		}
		return S_OK;
	}

	Parameter& param = parameters[id];

	switch (param.type)
	{
		case ParamTypeButton:
		{
			if (param.direction == Input)
			{
				TEInstanceLinkSetBooleanValue(instance, param.identifier.c_str(), true);
			}
			else
			{
				TEInstanceLinkGetBooleanValue(instance, param.identifier.c_str(), TELinkValueCurrent, (bool*)param.value);
			}
			break;
		}
		case ParamTypeSwitch:
		{
			if (param.direction == Input)
			{
				TEInstanceLinkSetBooleanValue(instance, param.identifier.c_str(), true);
			}
			else
			{
				TEInstanceLinkGetBooleanValue(instance, param.identifier.c_str(), TELinkValueCurrent, (bool*)param.value);
			}
			break;
		}
		case ParamTypeFloat:
		{
			if (param.direction == Input)
			{
				TEInstanceLinkSetDoubleValue(instance, param.identifier.c_str(), (double*)param.value, 1);
			}
			else
			{
				TEInstanceLinkGetDoubleValue(instance, param.identifier.c_str(), TELinkValueCurrent, (double*)param.value, 1);
			}
			break;
		}
		case ParamTypeInt:
		{
			if (param.direction == Input)
			{
				TEInstanceLinkSetIntValue(instance, param.identifier.c_str(), (int*)param.value, 1); //Probably should replace this with variant
			}
			else
			{
				TEInstanceLinkGetIntValue(instance, param.identifier.c_str(), TELinkValueCurrent, (int*)param.value, 1);
			}
			break;
		}

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

	CreateVertexBuffer();
	//Attach texture to shader

	CreateShaderResources();

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
//	devContext->Flush();



	if (hasVideoInput) {

		TEVideoInputD3D.take(TED3D11TextureCreate(texture.Get(), TETextureOriginTopLeft, kTETextureComponentMapIdentity, (TED3D11TextureCallback)textureCallback, nullptr));

		TEResult result = TEInstanceLinkSetTextureValue(instance, "op/vdjtexturein", TEVideoInputD3D, D3DContext);
		if (result != TEResultSuccess)
		{
			isPluginFX = false;
		}
	}

	std::unique_lock<std::mutex> lock(frameMutex);
	isTouchFrameBusy = true;
	lock.unlock();
	TEResult result = TEInstanceStartFrameAtTime(instance, totalSamples, SampleRate, false);

	if (result != TEResultSuccess)
	{
		std::unique_lock<std::mutex> lock(frameMutex);
		isTouchFrameBusy = false;
		return S_FALSE;
	}


	while (isTouchFrameBusy)
	{

	}

	lock.lock();
	isTouchFrameBusy = false;
	lock.unlock();

	if (hasVideoOutput) {

		TouchObject<TETexture> CurrentOutputTexture;
		result = TEInstanceLinkGetTextureValue(instance, "op/vdjtextureout", TELinkValueCurrent, CurrentOutputTexture.take());

		if (result == TEResultSuccess && CurrentOutputTexture != nullptr) {

			if (TETextureGetType(CurrentOutputTexture) == TETextureTypeD3DShared && result == TEResultSuccess)
			{
				TouchObject<TED3D11Texture> CurrentD3DTargettexture;
				result = TED3D11ContextGetTexture(D3DContext, static_cast<TED3DSharedTexture*>(CurrentOutputTexture.get()), CurrentD3DTargettexture.take());
				if (result != TEResultSuccess)
				{
					return S_FALSE;
				}

				//Get the native texture from TD
				auto tex = TED3D11TextureGetTexture(CurrentD3DTargettexture);

				if (tex == nullptr) {
					return S_FALSE;
				}

				D3D11_TEXTURE2D_DESC desc2;
				ZeroMemory(&desc, sizeof(desc2));

				tex->GetDesc(&desc2);

				TDOutputWidth = desc2.Width;
				TDOutputHeight = desc2.Height;

				ID3D11ShaderResourceView* CurrenttextureView = nullptr; //GetTexture doesn't AddRef, so doesn't need to be released

				D3D11_SHADER_RESOURCE_VIEW_DESC desc;
				ZeroMemory(&desc, sizeof(desc));
				desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // Matching format is important
				desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
				desc.Texture2D.MostDetailedMip = 0;
				desc.Texture2D.MipLevels = 1;
				HRESULT hr = D3DDevice->CreateShaderResourceView(tex, &desc, &CurrenttextureView);

				bitblt(D3DDevice, CurrenttextureView);
			}
		}

		if (result != TEResultSuccess)
		{
			return S_FALSE;
		}
	}

	devContext->Flush();
	frameCount += SampleRate;
	return S_OK;
}

HRESULT VDJTouchEngine::OnAudioSamples(float* buffer, int nb)
{
	if (hasAudioInput) {
		TouchObject<TEFloatBuffer> CurentInputBuffer;
		if (totalSamples == 0) {
			TEAudioInFloatBuffer1.take(TEFloatBufferCreateTimeDependent(SampleRate, 2, nb, nullptr));
			CurentInputBuffer.take(TEFloatBufferCreateCopy(TEAudioInFloatBuffer1));
		}
		else if (frameCount != 0 && TEAudioInFloatBuffer2 == nullptr && TEFloatBufferGetCapacity(TEAudioInFloatBuffer1) != nb) {
			TEAudioInFloatBuffer2.take(TEFloatBufferCreateTimeDependent(SampleRate, 2, nb, nullptr));
			CurentInputBuffer.take(TEFloatBufferCreateCopy(TEAudioInFloatBuffer2));
		}
		else {
			if (nb == TEFloatBufferGetCapacity(TEAudioInFloatBuffer1)) {
				CurentInputBuffer.take(TEFloatBufferCreateCopy(TEAudioInFloatBuffer1));
			}
			else {
				CurentInputBuffer.take(TEFloatBufferCreateCopy(TEAudioInFloatBuffer2));
			}
		}



		std::vector<float> leftBuffer;
		std::vector<float> rightBuffer;
		std::vector<const float*> channels(2);

		for (int i = 0; i < nb; i++)
		{
			leftBuffer.push_back(buffer[i * 2]);
			rightBuffer.push_back(buffer[i * 2 + 1]);
		}
		
		channels[0] = leftBuffer.data();
		channels[1] = rightBuffer.data();


		TEResult result = TEFloatBufferSetValues(CurentInputBuffer, channels.data(), nb);

		if (result != TEResultSuccess)
		{
			return S_FALSE; 
		}

		result = TEFloatBufferSetStartTime(CurentInputBuffer, totalSamples);
		if (result != TEResultSuccess)
		{
			return S_FALSE;
		}
		totalSamples += nb;

		result = TEInstanceLinkAddFloatBuffer(instance, "op/vdjaudioin", CurentInputBuffer);

		if (result != TEResultSuccess)
		{
			return S_FALSE;
		}

	}

	//We can get this working last
	if (hasAudioOutput) {
		if (TEAudioOutput == nullptr) {
			TEAudioOutput.take(TEFloatBufferCreateTimeDependent(SampleRate, 2, 700, nullptr));
		}
	//	TEResult result = TEInstanceLinkGetFloatBufferValue(instance, "op/vdjaudioout", TELinkValueCurrent, TEAudioOutput.take());

		uint32_t valueCount = TEFloatBufferGetValueCount(TEAudioOutput);
		int32_t channelCount = TEFloatBufferGetChannelCount(TEAudioOutput);
		if (valueCount < nb) {
			return S_FALSE;
		}

		if (channelCount != 2) {
			return S_FALSE;
		}


		const float* const* values = TEFloatBufferGetValues(TEAudioOutput);

		if (values == nullptr) {
			return S_FALSE;
		}

		for (int i = 0; i < nb; i++)
		{
			buffer[i * 2] = values[0][i];
			buffer[i * 2 + 1] = values[1][i];
		}


	}


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
					logger->info("Loading file: {}", filePath);
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

	result = TEInstanceSetFrameRate(instance, SampleRate, 800);

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


	GetAllParameters();


	result = TEInstanceResume(instance);

	if (result != TEResultSuccess)
	{
		return false;
	}


	if (TEVideoInputD3D == nullptr) {
		TEVideoInputD3D.take(TED3D11TextureCreate(D3DTextureInput, TETextureOriginTopLeft, kTETextureComponentMapIdentity, nullptr, nullptr));
	}


	return true;
}

void VDJTouchEngine::LoadTouchEngine() {

	if (instance == nullptr) {
		logger->info("Loading TouchEngine");
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
		logger->error("Failed to create texture input");
		return hr;
	}

	hr = D3DDevice->CreateTexture2D(&description, nullptr, &D3DTextureOutput);

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
		logger->error("Failed to create texture output");
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

HRESULT VDJTouchEngine::CreateVertexBuffer() {
	D3D11_BUFFER_DESC desc = { 0 };
	desc.Usage = D3D11_USAGE_DYNAMIC;
	desc.ByteWidth = sizeof(TLVERTEX) * 4;
	desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
	desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
	HRESULT hr = D3DDevice->CreateBuffer(&desc, nullptr, &D3DVertexBuffer);
	if (FAILED(hr))
	{
		logger->error("Failed to create vertex buffer");
		return hr;
	}

	D3DDevice->CreatePixelShader(PixelShaderCode, sizeof(PixelShaderCode), nullptr, &D3DPixelShader);

	return S_OK;

}

HRESULT VDJTouchEngine::CreateShaderResources() {

	D3D11_SHADER_RESOURCE_VIEW_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM; // Matching format is important
	desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	desc.Texture2D.MostDetailedMip = 0;
	desc.Texture2D.MipLevels = 1;

	HRESULT hr = D3DDevice->CreateShaderResourceView(D3DTextureOutput, &desc, &D3DOutputTextureView);

	if (FAILED(hr))
	{
		logger->error("Failed to create shader resource view");
		return hr;
	}


	return S_OK;
}



void VDJTouchEngine::GetAllParameters()
{
	TouchObject<TEStringArray> groupLinkInfo;


	TEResult result = TEInstanceGetLinkGroups(instance, TEScopeInput, groupLinkInfo.take());

	if (result != TEResultSuccess)
	{
		return;
	}



	for (int i = 0; i < groupLinkInfo->count; i++)
	{
		TouchObject<TEStringArray> links;
		result = TEInstanceLinkGetChildren(instance, groupLinkInfo->strings[i], links.take());
		
		if (result != TEResultSuccess)
		{
			return;
		}

		for (int j = 0; j < links->count; j++)
		{
			TouchObject<TELinkInfo> linkInfo;
			result = TEInstanceLinkGetInfo(instance, links->strings[j], linkInfo.take());

			if (result != TEResultSuccess)
			{
				continue;
			}


			if (linkInfo->domain == TELinkDomainParameter) {
				Parameter object;
				object.identifier = linkInfo->identifier;
				object.name = linkInfo->name;
				object.direction = Input;
				object.vdj_id = j+2; //OpenFile is always 0

				switch (linkInfo->type)
				{
					case TELinkTypeTexture:
					{
						continue;
					}

					case TELinkTypeDouble:
					{
					
						object.type = ParamTypeFloat;
						object.max = 0.0;
						object.min = 0.0;
						object.step = 0.1;
						object.value = new char[4];
						result = TEInstanceLinkGetDoubleValue(instance, linkInfo->identifier, TELinkValueMaximum, &std::get<double>(object.max), 1);
						if (result != TEResultSuccess)
						{
							continue;
						}

						result = TEInstanceLinkGetDoubleValue(instance, linkInfo->identifier, TELinkValueMinimum, &std::get<double>(object.min), 1);
						if (result != TEResultSuccess)
						{
							continue;
						}
						break;

					}
					case TELinkTypeInt:
					{
						object.type = ParamTypeInt;
						object.max = 0;
						object.min = 0;
						object.step = 1;
						object.value = new char[4];

						result = TEInstanceLinkGetIntValue(instance, linkInfo->identifier, TELinkValueMaximum, &std::get<int>(object.max), 1);
						if (result != TEResultSuccess)
						{
							continue;
						}

						result = TEInstanceLinkGetIntValue(instance, linkInfo->identifier, TELinkValueMinimum, &std::get<int>(object.max), 1);
						if (result != TEResultSuccess)
						{
							continue;
						}
						break;
					}
					case TELinkTypeBoolean:
					{
						if (linkInfo->intent == TELinkIntentMomentary) {
							object.type = ParamTypeButton;
						}
						else
						{
							object.type = ParamTypeSwitch;
						}

						object.max = 1;
						object.min = 0;
						object.step = 1;
						object.value = new char[1];
						break;
					}
				}
				parameters[object.vdj_id] = object;
			}
			else if (linkInfo->domain == TELinkDomainOperator) {
				if (strcmp(linkInfo->name, "vdjtexturein") == 0 && linkInfo->type == TELinkTypeTexture)
				{
					isPluginFX = true;
					hasVideoInput = true;
				}

				else if (strcmp(linkInfo->name, "vdjaudioin") == 0 && linkInfo->type == TELinkTypeFloatBuffer)
				{
					hasAudioInput = true;
				}

			}

		}

	}

	for (auto& param : parameters)
	{
		if (param.second.type == ParamTypeButton || param.second.type == ParamTypeSwitch)
		{
			DeclareParameterButton((int*)param.second.value, param.second.vdj_id, param.second.identifier.c_str(), param.second.name.c_str());
		}
		else if (param.second.type == ParamTypeFloat)
		{
			DeclareParameterSlider((float*)param.second.value, param.second.vdj_id, param.second.identifier.c_str(), param.second.name.c_str(), std::get<double>(param.second.min));
		}
		else if (param.second.type == ParamTypeInt)
		{
			//There is currently an error throwing saying bad usage
			DeclareParameterSlider((float*)param.second.value, param.second.vdj_id, param.second.identifier.c_str(), param.second.name.c_str(), static_cast<double>(std::get<int>(param.second.min)));
		}
	}


	result = TEInstanceGetLinkGroups(instance, TEScopeOutput, groupLinkInfo.take());

	if (result != TEResultSuccess)
	{
		logger->error("Failed to get output link groups");
		return;
	}

	for (int i = 0; i < groupLinkInfo->count; i++)
	{
		TouchObject<TEStringArray> links;
		result = TEInstanceLinkGetChildren(instance, groupLinkInfo->strings[i], links.take());

		if (result != TEResultSuccess)
		{
			logger->error("Failed to get output link children");
			return;
		}

		for (int j = 0; j < links->count; j++)
		{
			TouchObject<TELinkInfo> linkInfo;
			result = TEInstanceLinkGetInfo(instance, links->strings[j], linkInfo.take());

			if (result != TEResultSuccess)
			{
				return;
			}

			if (strcmp(linkInfo->name, "vdjtextureout") == 0 && linkInfo->type == TELinkTypeTexture)
			{
				hasVideoOutput = true;
			}
			else if (strcmp(linkInfo->name, "vdjaudioout") == 0 && linkInfo->type == TELinkTypeFloatBuffer)
			{
				hasAudioOutput = true;
			}

		}
	}

}



void VDJTouchEngine::bitblt(ID3D11Device* d3dDev, ID3D11ShaderResourceView* textureView)
{
	int nbVertices = 6;

	if (!D3DVertexBuffer)
	{
		D3D11_BUFFER_DESC bd{ 0 };
		bd.Usage = D3D11_USAGE_DYNAMIC;                // write access access by CPU and GPU
		bd.ByteWidth = sizeof(TLVERTEX) * nbVertices;
		bd.BindFlags = D3D11_BIND_VERTEX_BUFFER;       // use as a vertex buffer
		bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;    // allow CPU to write in buffer
		HRESULT hr = d3dDev->CreateBuffer(&bd, nullptr, &D3DVertexBuffer);
		if (!D3DVertexBuffer)
			return;
	}

	Microsoft::WRL::ComPtr<ID3D11DeviceContext> devContext; //use smart pointer to automatically release pointer and prevent memory leak
	D3DDevice->GetImmediateContext(&devContext);

	D3D11_MAPPED_SUBRESOURCE ms;
	HRESULT hr = devContext->Map(D3DVertexBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms);	// map the buffer
	if (hr != S_OK)
		return;

	int srcX = 0, srcY = 0, srcWidth = TDOutputWidth, srcHeight = TDOutputHeight, dstX = 0, dstY = 0, dstWidth = width, dstHeight = height;

	TLVERTEX* vertices = (TLVERTEX*)ms.pData;
	setVertexDst(vertices, (float)dstX, (float)dstY, (float)dstWidth, (float)dstHeight, RGB(255, 255, 255));
	setVertexSrc(vertices, (float)srcX, (float)srcY, (float)srcWidth, (float)srcHeight, (float)TDOutputWidth, (float)TDOutputHeight);
	devContext->Unmap(D3DVertexBuffer, 0);
	UINT stride = sizeof(TLVERTEX);
	UINT offset = 0;
	devContext->IASetVertexBuffers(0, 1, &D3DVertexBuffer, &stride, &offset);

	devContext->OMSetBlendState(nullptr, nullptr, 0xffffffff);
	if (!D3DPixelShader)
	{
		hr = d3dDev->CreatePixelShader(PixelShaderCode, sizeof(PixelShaderCode), nullptr, &D3DPixelShader);
	}
	devContext->PSSetShader(D3DPixelShader, nullptr, 0);

	devContext->PSSetShaderResources(0, 1, &textureView);

	devContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	devContext->Draw(6, 0);
}


void VDJTouchEngine::UnloadTouchEngine() {
	if (instance != nullptr)
	{
		TEInstanceSuspend(instance);
		TEInstanceUnload(instance);
		D3DContext.reset();
		D3DDevice->Release();
	}
}


void VDJTouchEngine::eventCallback(TEEvent event, TEResult result, int64_t start_time_value, int32_t start_time_scale, int64_t end_time_value, int32_t end_time_scale)
{
	

	if (result == TEResultComponentErrors)
	{
		TouchObject<TEErrorArray> errors;
		TEResult result =  TEInstanceGetErrors(instance, errors.take());
		if (result != TEResultSuccess)
		{
			return;
		}

		for (int i = 0; i < errors->count; i++)
		{
			logger->error("TouchEngine Error: Severity: {}, Location: {}, Description: {}", GetSeverityString(errors->errors[i].severity), errors->errors[i].location, errors->errors[i].description);
		}

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
		case TELinkEventValueChange:
			if (hasAudioOutput && strcmp(identifier, "op/vdjaudioout") == 0) {
					audioMutex.lock();
					TEAudioOutput.reset();
					TEInstanceLinkGetFloatBufferValue(instance, identifier, TELinkValueCurrent, TEAudioOutput.take());
					audioMutex.unlock();
			}
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
