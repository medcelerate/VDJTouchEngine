## VirtualDJ TouchEngine
![DALL·E 2023-12-23 00 20 02 - 3D metallic text, colorful, logo with an alpha channel that Says Precisely TouchVDJ ](https://github.com/medcelerate/VDJTouchEngine/assets/32549017/066ea319-96c9-48ad-be9b-c76c9765942a)

### Under Development - Not ready for production use

Aim is to be able to develop virtual DJ plugins in a similar way to the oldschool Quartz Composer plugins.

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

### To-Do
- Add support for audio buffer injection.
- Add ability for exposing parameters for control through effects panel
- Add support for BPM control
- Add better async file loading support.

### Learnings

Useful tips for those wanting to integrate touchengine.

Make sure to include the TouchEngine.dll with your compiled program

However your draw call is implemented you should:

1. Set input Linkages
2. Cook Frame
3. Wait for Cook to finish
4. Get output Linkages
5. Draw

