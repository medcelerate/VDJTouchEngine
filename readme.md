## VirtualDJ TouchEngine
![DALL·E 2023-12-23 00 20 02 - 3D metallic text, colorful, logo with an alpha channel that Says Precisely TouchVDJ ](https://github.com/medcelerate/VDJTouchEngine/assets/32549017/066ea319-96c9-48ad-be9b-c76c9765942a)

[![ko-fi](https://ko-fi.com/img/githubbutton_sm.svg)](https://ko-fi.com/Q5Q6YUGIA)

Aim is to be able to develop virtual DJ plugins in a similar way to the oldschool Quartz Composer plugins.

### Current Builds
https://drive.google.com/drive/folders/11oUVYvIBQTUOGlM0LBgdYDdV4gO7Uexd?usp=sharing
### Usage

Download the latest release from the releases page and copy the 'VDJTouchEngine.dll' into `C:\Users\UserName\AppData\Local\VirtualDJ\Plugins64\VideoEffects`

Once loaded add the plugin from the effects menu and click the plus icon to open up the parameters window.

Click the load button to load a tox file.

### Configuring a Tox File

#### Video In
Create a TOP of type In named vdjtexturein. This will be the input texture from virtual DJ.

#### Video Out
Create a TOP of Type Out named vdjtextureout. This will be the output texture to virtual DJ.

#### Audio In
Create a CHOP of type In named vdjaudioin. This will be the input audio from virtual DJ.

#### Audio Out
Create a CHOP of type Out named vdjaudioout. This will be the output audio to virtual DJ.

#### Parameters In TD
Create a Table DAT named vdjparameters. This will be the parameters exposed from virtual DJ.
This will fill witht the first row being parameter names and the second row being the parameter values.
There will be one row per deck in virtual DJ.

### V1 Release Requirements
- Add BPM Input

### To-Do
- Add support for audio buffer injection. - Done
- Add ability for exposing parameters for control through effects panel - In Progress
- Add support for BPM control
- Add support for video buffer injection - Done.
- Add support for dyanmic sample rate
- Add support for system controlled framerates
- Add support for table DAT input params.

### Learnings

Useful tips for those wanting to integrate touchengine.

Make sure to include the TouchEngine.dll with your compiled program

However your draw call is implemented you should:

1. Set input Linkages
2. Cook Frame
3. Wait for Cook to finish
4. Get output Linkages
5. Draw


To create a `const float**` in c++ to pass to the value funciton you can do the following:

```c++
std::vector<float> values;
values.push_back(1.0f);
values.push_back(2.0f);

std::vector<const float*> valuesPtrs;
valuesPtrs.push_back(values.data());

Pass the valuesPtrs.data() to the value function.

```

If doing audio it's useful to compute your frame rate and sample times based off the sample rate.

```c++
int SampleRate = 48000;
result = TEInstanceSetFrameRate(instance, SampleRate, 800);

TEInstanceStartFrameAtTime(instance, totalSamples, SampleRate, false);

```
