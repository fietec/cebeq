# cebeq

An incremental backup system written in C.

> [!WARNING]
> This project is still in development and not even near completion!

> [!IMPORTANT]
> The intended build system is [nob](https://github.com/tsoding/nob.h). 
> For those not based enough to use nob, I added a Makefile as well, though I won't guarantee that it will always be up to date.

## Dependencies

This project uses [raylib](https://github.com/raysan5/raylib) as a graphical backend. See their wiki for build instructions. 

Make sure to either place the compiled `.a` file into the `lib` directory or modify the build system to link with the library.