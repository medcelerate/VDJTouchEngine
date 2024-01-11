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

		if (totalSamples < SampleRate) {
			totalSamples = totalSamples - SampleRate; //Comment this if this break ssomething
			TEResult result = TEFloatBufferSetStartTime(CurentInputBuffer, totalSamples);
			if (result != TEResultSuccess)
			{
				return S_FALSE;
			}
			totalSamples += nb;
		}
		else {
			TEResult result = TEFloatBufferSetStartTime(CurentInputBuffer, totalSamples);
			if (result != TEResultSuccess)
			{
				return S_FALSE;
			}
			totalSamples += nb;
		}

		result = TEInstanceLinkAddFloatBuffer(instance, "op/vdjaudioin", CurentInputBuffer);

		if (result != TEResultSuccess)
		{
			return S_FALSE;
		}

	}

	//We can get this working last
	if (hasAudioOutput) {
		if (TEAudioOutput == nullptr) {
			TEAudioOutput.take(TEFloatBufferCreate(SampleRate, 2, nb, nullptr));
		}
		TEResult result = TEInstanceLinkGetFloatBufferValue(instance, "op/vdjaudioout", TELinkValueCurrent, TEAudioOutput.take());

		if (result != TEResultSuccess)
		{
			return S_FALSE;
		}
		uint32_t valueCount = TEFloatBufferGetValueCount(TEAudioOutput);
		int32_t channelCount = TEFloatBufferGetChannelCount(TEAudioOutput);

		if (channelCount != 2) {
			return S_FALSE;
		}

		const float* const* values = TEFloatBufferGetValues(TEAudioOutput);

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


			if (linkInfo->domain == TELinkDomainParameter && true == false) {
				Parameter object;
				object.identifier = linkInfo->identifier;
				object.name = linkInfo->name;
				object.direction = Input;

				switch (linkInfo->type)
				{
					case TELinkTypeTexture:
					{
						continue;
					}

					case TELinkTypeDouble:
					{
					
						object.type = ParamTypeFloat;
						object.max = 0;
						object.min = 0;
						object.step = 0.1;
						object.value = new char[4];
						result = TEInstanceLinkGetDoubleValue(instance, linkInfo->identifier, TELinkValueMaximum, &std::get<double>(object.max), 0);
						if (result != TEResultSuccess)
						{
							continue;
						}

						result = TEInstanceLinkGetDoubleValue(instance, linkInfo->identifier, TELinkValueMinimum, &std::get<double>(object.min), 0);
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

						result = TEInstanceLinkGetIntValue(instance, linkInfo->identifier, TELinkValueMaximum, &std::get<int>(object.max), 0);
						if (result != TEResultSuccess)
						{
							continue;
						}

						result = TEInstanceLinkGetIntValue(instance, linkInfo->identifier, TELinkValueMinimum, &std::get<int>(object.max), 0);
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
				parameters[linkInfo->identifier] = object;
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
				else if (strcmp(linkInfo->name, "vdjaudioout") == 0 && linkInfo->type == TELinkTypeFloatBuffer)
				{
					hasAudioOutput = true;
				}
				else if (strcmp(linkInfo->name, "vdjtextureout") == 0 && linkInfo->type == TELinkTypeTexture)
				{
					hasVideoOutput = true;
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
			DeclareParameterSlider((float*)param.second.value, param.second.vdj_id, param.second.identifier.c_str(), param.second.name.c_str(), std::get<double>(param.second.max));
		}
		else if (param.second.type == ParamTypeInt)
		{
			//There is currently an error throwing saying bad usage
			DeclareParameterSlider((float*)param.second.value, param.second.vdj_id, param.second.identifier.c_str(), param.second.name.c_str(), std::get<double>(param.second.max));
		}
	}


	result = TEInstanceGetLinkGroups(instance, TEScopeOutput, groupLinkInfo.take());

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
			if (hasAudioOutput) {
				if (strcmp(identifier, "op/vdjaudioout") == 0) {
					audioMutex.lock();
					TEInstanceLinkGetFloatBufferValue(instance, identifier, TELinkValueCurrent, TEAudioOutput.take());
					audioMutex.unlock();
				}
			}
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