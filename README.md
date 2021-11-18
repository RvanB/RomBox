![RomBox interaction](https://i.imgur.com/AE5d3el.gif)

# Installation

Install the custom font used in RomBox. It can be found in ```resources/tetris.ttf```.

RomBox requires GTK3, a widget toolkit that is used for the GUI. If will only be using binary releases of RomBox, I highly recommend using the [GTK3 for Windows Runtime Environment Installer](https://github.com/tschoonj/GTK-for-Windows-Runtime-Environment-Installer) by tschoonj. In the installer, make sure that the option to add to the system path is selected. Otherwise you'll need to add it to your path manually.

If you plan to compile RomBox from source, I recommend installing GTK3 in the MSYS2 MinGW console. See instructions below.

### Compiling from source
1. Clone this GitHub repository
	```
	git clone https://github.com/RvanB/RomBox.git
	```
2. Download and install [MSYS2](https://www.msys2.org/)
3. Open the MSYS2 MinGW console
4. Install mingw gcc: 
	```
	pacman -Syu mingw-w64-x86_64-gcc
	```
5. Install GTK3:
	```
	pacman -Syu mingw-w64-x86_64-gtk3
	```
6. Add MSYS2 bin to path
	1. Press Win+R and type `systempropertiesadvanced`, and press OK.
	2. Click on "Environment Variables"
	3. Add mingw64 bin folder to path (Default is `C:\msys64\mingw64\bin`)
7. Change into RomBox directory
	```
	cd RomBox
	```
3. Compile:
	```
	make
	```
### Releases
No binary releases have been posted, but the first version will be posted when all functionality is completed.
