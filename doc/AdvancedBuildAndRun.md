# Dependencies

For the patching we rely on the [D2.Detours](https://github.com/Lectem/D2.Detours.git) project, which is included as a git submodule. (use the `git submodule update --init` command, or clone this project with `git clone --recursive`)
You will also need to install the [CMake](https://cmake.org) build system and Visual C++ (or any C++ compiler that can generate .DLLs on Windows) which are freely available.


# Build the project

The recommended way is to use CMake presets.

## With presets

You can generate a VS solution (in `out/build/VS20XX`) using the command-line:

```sh
# Configure the CMake project
cmake --preset DefaultGenerator
# Build the release config
cmake --build --preset DefaultGenerator --config Release
# Optional install/package step
cmake --build --preset DefaultGenerator --config Release --target install
```
and/or using the CMake GUI to configure and modify compilation options:
![CMake GUI](assets/img/CMake-GUI.png)


Of course you can replace `DefaultGenerator` by `VS2022` if you are using Visual Studio 2022.

> Note: The `ninja` preset requires to run the `cmake` configuration step to be ran from the *x86 Native Command Prompt*.

## Without presets

If you do not wish to use the presets:

```sh
# Configure the CMake project
cmake -A Win32 -B YOU_BUILD_DIR
# Build the release config
cmake --build YOU_BUILD_DIR --config Release
# Optional install/package step
cmake --install YOU_BUILD_DIR --config Release --prefix YOUR_INSTALL_FOLDER
```

By default, the reconstructed patch DLLs that are normally installed under `bin/patch` are now also copied there automatically after each build, so running `install` is no longer required between iterations just to refresh the patch folder. This currently includes `D2Client`, `D2Common`, `D2Game`, and `D2Debugger`.

- Default build-time patch destination: `${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_BINDIR}/patch`
- Override it with the `D2MOO_PATCH_OUTPUT_DIR` CMake cache variable
- Disable automatic syncing with `D2MOO_AUTO_COPY_PATCH_DLLS=OFF`

## "Open Folder" and Visual Studio Code

It is also possible to use *Visual Studio* "Open folder" feature or *Visual Studio Code*, however you will need to configure [`Launch.vs.json`](https://learn.microsoft.com/en-us/cpp/build/configure-cmake-debugging-sessions?view=msvc-170#launchvsjson-reference) manually, which is **not practical nor recommended** .

# Running in Visual Studio

If you are using a default Diablo2 install and generated `.sln` through *CMake*, you are good to go!

Otherwise (for example if you need to configure your installation folder) have a look at the [Debugging](Debugging.md) documentation.

# Running manually (For advanced users)

Launch *Diablo2* (or *D2SE*) by using the command `D2.DetoursLauncher [Path to Game.exe or D2SE.exe]`.
- If the `Game.exe`/`D2SE.exe` executable is omitted, `D2.DetoursLauncher` will look for executables using the registry, working directory and `PATH`.
- You can pass the executable parameters by using the `--` parameter.  
  For example: `D2.DetoursLauncher [Game.exe] -- -direct -txt -w`.
- The *detours* .dll expects patch `.dll`s to be under the `./patch` folder of the current working directory. This can be overriden using the `DIABLO2_PATCH` environment variable. CMake will set this variable by default for debugging.

