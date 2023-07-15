## How to create a release

1. Go to GitHub releases
2. Create a new release
3. Create new tag with version #
4. Wait for CI to finish

## Building from source

To build dll and Launcher:
- `./build.sh` - Requires DirectX June 2010, MSBuild, Visual Studio 2019 and Git Bash.

To build the installer:
- `VERSION=X.X.X ./package.sh` - Expects a version to be provided by setting an environment variable. Saves and builds artifacts to `build/`. Requires Inno Setup and Git Bash.

## Troubleshooting
- `%localappdata%\Temp` - Installer logs are saved in here. E.g., `Setup Log 2023-07-13 #010.txt`
- `Get-MpThreatDetection` - Run this command in Windows PowerShell to get Windows Defender threat history to see if any of the multiplayer files were affected. 

## Contributors
Thanks to these wonderful people :)
- [btbd](https://github.com/btbd)
- [LucasOe](https://github.com/LucasOe)
- [Toyro98](https://github.com/Toyro98)
- [Meteos](https://github.com/masoukaze)
- [SeungKang](https://github.com/SeungKang)
- [stephen-fox](https://github.com/stephen-fox)
