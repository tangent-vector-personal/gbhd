=============================================================================
                      gbhd - Play Game Boy games in HD
=============================================================================

This application is a demonstration of the technique described at:

tangentvector.wordpress.com/2011/10/20/emulating-classic-2d-games-in-hd/

It is an emulator for the original Game Boy system that can run games
with replacement tile and sprite graphics.

================================= Running ===================================

The controls for the game are currently hard-wired:

A, B        = A, B
I, J, K, L  = Up, Left, Down, Right
S           = Start
T           = Select

Space       = Pause / Resume
9           = Dump tile/sprite graphics used for the next frame
0           = Cycle through renderers (if mulitple renderers are loaded)

For the Mac build:
Command-O   = Select a ROM file to open
Command-M   = Set the media folder to use when loading replacement images.

================================= Building ==================================

Currently this project is set up to build with XCode 4.
The project file can be found in build/xcode4/gbhd.xcodeproj.

======================= Setting Up the Media Folder =========================

In order to load replacement graphics for a game, you must set up a media/
folder for the program to use, and then set that media folder in
the application (Command-M).

Your media folder should be laid out something like:

media/names.txt
media/<Game Name>/dump/
media/<Game Name>/replace/replace.txt
media/<Game Name>/replace/<Image Name>.png

== The names.txt File ==

This file maps the raw name embedded in a game ROM to the "pretty" name
that you'd like to use for the game's media folder, e.g.:

SUPER MARIOLAND = Super Mario Land
TETRIS          = Tetris

== The dump/ Folder ==

If you dump tiles when playing a game (9 key), then they will be added
to the dump/ folder for the current game. Names for tiles will look like:

38387c44ee82ce8ace8afe927c443838p0123.png

Everything before the 'p0123' is an encoding of the tile's image data, while
the 'p0123' tells us the palette that was used for this tile when it was
dumped.

== The replace/ Folder ==

You put your replacement tile images (png format, 3-channel RGB or 4-channel
RGBA) into this folder, and then reference them from the replace.txt file.

Keep your replacement images in grayscale; the emulator will de-color-ize them
anyway when loading.

== The replace/replace.txt File ==

This file tells the emulator how to load the replacement images and associate
them with in-game tiles. It consists of lines with different commands:

layer fg
layer bg

These commands set the layer (foreground or background) for which we will
be replacing images. Sprites use only a single layer (foreground) while tile
maps use both the foreground and background layers. In cases where a sprite
appears to go "behind" a background tile, you will need to put the foreground
elements (e.g., a pipe that Mario will go into) in the foreground image, and
any background elements (e.g., the sky behind the pipe) into the background.

palette 0123

Sets the palette to assume for any subsequent tiles we replace until the next
time we set a palette. This needs to be four digits, each from 0-3. This
should almost always be the palette that was used to dump the tile (so if
the tile was dumped with suffix "p3210" then you should set "palette 3210".
The emulator uses this information to allow the images to be recolored into
any palette the game uses at run-time.

image fileName.png

Loads the image "fileName.png" from the replace/ folder. This image will
be used for any subsequent tiles we replace until the next time we load an
image.

rect 0 0 1 1

Specifies a sub-rectangle of the loaded image to use for the next tile. Use
"rect 0 0 1 1" if you want the whole image. The order of components here is
left, top, right, bottom. Coordinates range from 0 to 1 across the image
(that is, they are texture coordinates rather than pixel coordinates).

tile 38387c44ee82ce8ace8afe927c443838

Replaces the tile with the given encoded image data using the currently-set
layer, palette, image and rectangle.

============================== Known Issues =================================

This is a proof-of-concept project, rather than something I expect people
to actually use to play games for fun. As such there are a lot of problems
that I am not likely to address:

- Only a small subset of games currently load and run correctly. In particular
  gbhd only supports the simplest form of memory-bank switching (MBC1).
  
- There is no quick-save/load support, nor support for save memory.

- There is no sound support.

- All of the controls are hard-wired.

- The full-screen mode doesn't hide the cursor.

- The way that gbhd draws graphcis with OpenGL is sub-optimal. Much better
  performance should be possible, but currently some games will suffer from
  dropped frames or other issues.
  
- The emulator core is missing a few features required for full accuracy.
