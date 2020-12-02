![Screenshot of RomBox](https://i.imgur.com/PgE5ly4.png)

# Installation

## Setting up GTK3 with MSYS2
1. Download and install [MSYS2](https://www.msys2.org/)
2. Open the MSYS2 MinGW console
3. Install GTK3: 
	```
	pacman -S mingw-w64-x86_64-gtk3
	```
4. Add MSYS2 bin to path
	1. Press Win+R and type `systempropertiesadvanced`, and press OK.
	2. Click on "Environment Variables"
	3. Add mingw64 bin folder to path (Default is `C:\msys64\mingw64\bin`)

## Getting RomBox
You have two options, compiling the project yourself from sources, or downloading one of the posted releases.

### Compiling the project yourself
1. Clone this GitHub repository
	```
	git clone https://github.com/RvanB/RomBox.git
	```
2. Install mingw gcc: 
	```
	pacman -S mingw-w64-x86_64-gcc
	```
3. Compile sources:
	```
  	gcc -mwindows \`pkg-config --cflags gtk+-3.0\` -o RomBox.exe RomBox.c resources/icon.res \`pkg-config --libs gtk+-3.0\`
	```
### Releases
No binary releases have been posted, but the first version will be posted when all functionality is completed.

The font used in the program can be found in resources/tetris.ttf.
