# cebeq

An incremental backup system written in C.
### Features
- cli and gui client
- full and incremental backups
- merging of incremental backups

> [!WARNING]
> This project is still in development!

> [!IMPORTANT]
> The intended build system is [nob](https://github.com/tsoding/nob.h).
> To bootstrap nob, simply run: `cc -o nob nob.c`
> After that, nob will detect changes in its source files and recompile on its own.


## Dependencies

This project uses [raylib](https://github.com/raysan5/raylib) as a graphical backend. See their wiki for build instructions. 

Make sure to either place the compiled `.a` file into the `lib` directory or have it in your path.