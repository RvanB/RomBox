![Screenshot of RomBox](https://i.imgur.com/lJTKEbj.png)

# Installation

## Setting up GTK3 with MSYS2
1. Download and install [MSYS2](https://www.msys2.org/)
2. Open the MSYS2 MinGW console
3. Install GTK3: `pacman -S mingw-w64-x86_64-gtk3`
4. Add MSYS2 bin to path
	a. Press Win+R and type `systempropertiesadvanced`, and press OK.
	b. Click on "Environment Variables"
	c. Add mingw64 bin folder to path (Default is `C:\msys64\mingw64\bin`)

## Compiling the project
1. Install mingw gcc: `pacman -S mingw-w64-x86_64-gcc`
2. Compile sources: `gcc -mwindows \`pkg-config --cflags gtk+-3.0\` -o RomBox.exe RomBox.c resources/icon.res \`pkg-config --libs gtk+-3.0`

The font used in the program can be found in resources/tetris.ttf.
