#pragma once
#include "VDJ/vdjVideo8.h"
// we include stdio.h only to use the sprintf() function
// we define _CRT_SECURE_NO_WARNINGS for the warnings of the sprintf() function
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")
#define WIN32_LEAN_AND_MEAN
#include <wrl.h>
#include <string>
#include <memory>
#include <mutex>
#include <windows.h>
#include <shobjidl.h>
#include "TouchEngine/TouchObject.h"
#include "TouchEngine/TEGraphicsContext.h"
#include "TouchEngine/TED3D11.h"

class VDJTouchEngine : public IVdjPluginVideoFx8
{
public:
	VDJTouchEngine();
	~VDJTouchEngine();
	HRESULT VDJ_API OnLoad();
	HRESULT VDJ_API OnGetPluginInfo(TVdjPluginInfo8* infos);
	ULONG VDJ_API Release();
	HRESULT VDJ_API OnDeviceInit();
	HRESULT VDJ_API OnDeviceClose();
	//HRESULT VDJ_API OnGetUserInterface(TVdjPluginInterface8* pluginInterface);
	HRESULT VDJ_API OnParameter(int id);
	HRESULT VDJ_API OnGetParameterString(int id, char* outParam, int outParamSize);
	HRESULT VDJ_API OnDraw();
	HRESULT VDJ_API OnStart();
	HRESULT VDJ_API OnStop();

	static void eventCallbackStatic(TEInstance* instance, TEEvent event, TEResult result, int64_t start_time_value, int32_t start_time_scale, int64_t end_time_value, int32_t end_time_scale, void* info);
	static void linkCallbackStatic(TEInstance* instance, TELinkEvent event, const char* identifier, void* info);

	void eventCallback(TEEvent event, TEResult result, int64_t start_time_value, int32_t start_time_scale, int64_t end_time_value, int32_t end_time_scale);
	void linkCallback(TELinkEvent event, const char* identifier);

private:
	int pFileButton = 0;
	std::string filePath;
	TEInstance* instance = nullptr;
	ID3D11Device* D3DDevice = nullptr;
	ID3D11Texture2D* D3DTextureInput = nullptr;

	TETexture * TEOutputTexture = nullptr;
	TED3D11Context* D3DContext = nullptr;
	TED3D11Texture* TEOutput = nullptr;
	TED3D11Texture* TEVideoInput = nullptr;
	TETexture* TEVideoInputTexture = nullptr;

	bool OpenFileDialog();
	bool LoadTEFile();
	bool isLoaded = false;
	bool isFX = false;
	bool isFrameBusy = false;
	bool isReady = false;
	std::mutex frameMutex;


	HRESULT OnVideoResize(int VidWidth, int VidHeight);
	int VideoWidth = 0;
	int VideoHeight = 0;


	Microsoft::WRL::ComPtr<ID3D11SamplerState>			mySampler;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	myTextureView;

protected:
	typedef enum _ID_Interface
	{
		ID_BUTTON_1,
	} ID_Interface;
};