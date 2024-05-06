#pragma once
#include "VDJ/vdjVideo8.h"
// we include stdio.h only to use the sprintf() function
// we define _CRT_SECURE_NO_WARNINGS for the warnings of the sprintf() function
#include <d3d11.h>
#pragma comment(lib, "d3d11.lib")
#define WIN32_LEAN_AND_MEAN
#include <wrl.h>
#include <string>
#include <unordered_map>
#include <memory>
#include <mutex>
#include <variant>
#include <array>
#include <windows.h>
#include <shobjidl.h>
#include "spdlog/spdlog.h"
#include "spdlog/sinks/basic_file_sink.h"
#include "TouchEngine/TouchObject.h"
#include "TouchEngine/TEGraphicsContext.h"
#include "TouchEngine/TED3D11.h"
#include "PixelShader.h"
#include "D3Dvertex.h"

#ifdef _DEBUG
#include "renderdoc_app.h"
#endif // _DEBUG



typedef enum InOutType {

	Input = 0,
	Output = 1,

} InOutType;

typedef enum ParamType {

	ParamTypeInt = 0,
	ParamTypeFloat = 1,
	ParamTypeString = 2,
	ParamTypeButton = 3,
	ParamTypeSwitch = 4,

} ParamType;

typedef struct Parameter {
	std::string identifier;
	std::string name;
	int vdj_id = 0;
	ParamType type;
	InOutType direction;
	std::variant<double, int> min = 0;
	std::variant<double, int> max = 0;
	std::variant<double, int> step = 0;
	char* value = nullptr;
} Parameter;



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
	HRESULT VDJ_API OnAudioSamples(float* buffer, int nb);

	static void eventCallbackStatic(TEInstance* instance, TEEvent event, TEResult result, int64_t start_time_value, int32_t start_time_scale, int64_t end_time_value, int32_t end_time_scale, void* info);
	static void linkCallbackStatic(TEInstance* instance, TELinkEvent event, const char* identifier, void* info);
	static void textureCallback(TED3D11Texture* texture, TEObjectEvent event, void* info);

	void eventCallback(TEEvent event, TEResult result, int64_t start_time_value, int32_t start_time_scale, int64_t end_time_value, int32_t end_time_scale);
	void linkCallback(TELinkEvent event, const char* identifier);

private:
#ifdef _DEBUG
	RENDERDOC_API_1_1_2* rdoc_api = nullptr;
#endif // _DEBUG
	std::shared_ptr<spdlog::logger> logger;

	//Params for VirtualDJ
	int pFileButton = 0;
	int pTouchReloadButton = 0;

	//VDJ textures and devices (Make these COM smart pointers)
	ID3D11Device* D3DDevice = nullptr;
	ID3D11Buffer* D3DVertexBuffer = nullptr;
	ID3D11Texture2D* D3DTextureInput = nullptr;
	ID3D11Texture2D* D3DTextureOutput = nullptr;
	ID3D11PixelShader* D3DPixelShader = nullptr;
	ID3D11ShaderResourceView* D3DOutputTextureView = nullptr;


	int VideoWidth = 0;
	int VideoHeight = 0;
	std::string filePath;

	std::unordered_map<int, Parameter> parameters;

	//TouchEngineState
	bool isTouchEngineLoaded = false;
	bool isTouchEngineReady = false;
	bool isPluginFX = false;
	bool isTouchFrameBusy = false;

	//Touch file capabilities
	bool hasVideoInput = false;
	bool hasAudioInput = false;
	bool hasVideoOutput = false;
	bool hasAudioOutput = false;


	TouchObject<TEInstance> instance;


	//TouchEngine objects
	TouchObject<TED3D11Texture> TEVideoInputD3D;
	TouchObject<TED3D11Texture> TEVideoOutputD3D;
	TouchObject<TETexture> TEVideoInputTexture;
	TouchObject<TETexture> TEVideoOutputTexture;
	TouchObject<TED3D11Context> D3DContext;
	TouchObject<TEFloatBuffer> TEAudioInFloatBuffer1;
	TouchObject<TEFloatBuffer> TEAudioInFloatBuffer2;
	TouchObject<TEFloatBuffer> TEAudioOutput;

	int frameCount = 0;
	int totalSamples = 0;
	int TDOutputWidth = 0;
	int TDOutputHeight = 0;
	//VDJ Functions
	HRESULT OnVideoResize(int VidWidth, int VidHeight);




	std::mutex frameMutex;
	std::mutex audioMutex;

	Microsoft::WRL::ComPtr<ID3D11SamplerState>			mySampler;
	Microsoft::WRL::ComPtr<ID3D11ShaderResourceView>	myTextureView;


	bool OpenFileDialog();
	bool LoadTEFile();
	void LoadTouchEngine();
	bool LoadTEGraphicsContext(bool reload = false);
	void GetAllParameters();
	void bitblt(ID3D11Device* d3dDev, ID3D11ShaderResourceView* textureView);
	void UnloadTouchEngine();

	HRESULT CreateTexture();
	HRESULT CreateVertexBuffer();
	HRESULT CreateShaderResources();

};