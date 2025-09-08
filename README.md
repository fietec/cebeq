# cebeq

An incremental backup system written in C.
### Features
- full and incremental backups
- merging of incremental backups
- cli and gui applications

![Failed to load image](gui.png)

> [!WARNING]
> This project is still in development!


### Building
The intended build system is [nob](https://github.com/tsoding/nob.h).  
To bootstrap nob, simply run: `cc -o nob nob.c` or `make`.  
After that use `./nob` with a specified target to build the project.  
See `./nob --help` for more information.

### Dependencies

This project uses [raylib](https://github.com/raysan5/raylib) as a graphical backend. See their wiki for build instructions. 

Make sure to either place the compiled `.a` file into the `lib` directory or have it in your path.
