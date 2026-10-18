# Dark Bedrock PvP HUD DLL — auto-build project

A visual-only x64 DLL prototype for Windows Bedrock. It creates a separate transparent, topmost HUD window when loaded. It does not patch game code, hook DirectX, read/write Minecraft memory, automate clicks, or change gameplay.

## Build without installing Visual Studio locally
1. Create a GitHub repository and upload all files from this folder.
2. Open the repository's **Actions** tab and enable Actions if prompted.
3. Run **Build Windows x64 DLL** using **Run workflow** (or push to `main`).
4. Open the finished workflow run, then download the `DarkBedrockPvPHud-x64` artifact.
5. Extract `DarkBedrockPvPHud.dll`.

The workflow compiles the DLL using Microsoft's Windows runner and MSVC. This builds the binary, but does not guarantee that Fate Injector or every Minecraft 1.26.x build supports it. Test only in a world/server where client modifications are allowed.

## HUD controls
- `Insert`: show/hide HUD
- `F6`: toggle HUD modules panel
- `F7`: toggle CPS
- `F8`: toggle Keystrokes
- `F9`: toggle FPS estimate
- `F10`: toggle crosshair
- `F11`: toggle Ping label

Keys are polled, not intercepted. CPS is observational only. FPS is the overlay refresh estimate, not Minecraft's authoritative game FPS. Ping displays N/A because this project does not read game network state.

## Notes
This is an early prototype. It draws in a separate transparent window, not through Minecraft's render pipeline. Some exclusive-fullscreen modes may hide overlays. Fate injection compatibility depends on the injector's supported architecture and target process.
