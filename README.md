# J.A.C.K. Plugin for Diffusion

Plugin for mapping for [Diffusion](https://aynekko.itch.io/diffusion) game.

Currently it lets you see env_cable entities in J.A.C.K. (Jackhammer) editor.

Version needed: 1.2.4603 (Steam, public beta) (API 121)

## Usage
Download vpDiffusionGamex64.dll and put it in ..\Steam\steamapps\common\JACK\plugins

Make sure the FGD is up to date - [download from here](https://aynekko.github.io/DiffusionWiki/mapping/mapping/)

## Editing and building the source code
Download the [JackSDK](https://github.com/SanyaSho/JackSDK).

In its CMakeLists.txt: replace the line `option( OVERRIDE_API_VERSION "Change defininon of JACK_API_VERSION" -1 )` with `set( OVERRIDE_API_VERSION 121 )` (or your desired API version)

Add a line in the end: `add_subdirectory( "plugins/vpDiffusionGame" )`

Put vpDiffusionGame folder inside JackSDK-master/plugins folder

Open JackSDK-master with Visual Studio, switch to CMake target view and build "diffusiongame"

