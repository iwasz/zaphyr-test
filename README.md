# Tips
1. When performing an update do it from `/home/iwasz/workspace/zephyrproject` directory. Use `west update`
2. Use the default serial port that shows up (usually `/dev/ttyACM0` in my case).

# Building in the command line
These are the steps I took to bring up a [NUCLEO-H743ZI](https://www.st.com/en/evaluation-tools/nucleo-h743zi.html)/STM32F746G-DISCO on Linux Manjaro and run a Zephyr 2.2.99 on it. I also wanted to set up a vscode project, and maybe try to recreate a toolchain using ct-ng (just for fun).

As of writing this my setup is as following:
- Linux futureboy 5.4.31-1-MANJARO #1 SMP PREEMPT Wed Apr 8 10:25:32 UTC 2020 x86_64 GNU/Linux
- vscode 1.44.1
- cmake version 3.17.1
- zephyr-sdk-0.11.2
- Zephyr version : 2.2.99

After following instructions from [Zephyr documentation](https://docs.zephyrproject.org/latest/getting_started/index.html) I was left with :

- ```~/workspace/zephyrproject``` : sources including SDKs for all platforms Zephyr runs on (a little waste of disk space, but west tool downloads everything by default).
- ```~/workspace/zephyr-sdk-0.11.2``` : toolchains for, again, all available platforms like ARM, xtensa etc.


I copied a sample project (cpp_synchronization)

I set environment variables (actually I have them stored in ~/.zephyrrc, and I source them in `.bashrc`):

```sh
export ZEPHYR_TOOLCHAIN_VARIANT=zephyr
export ZEPHYR_SDK_INSTALL_DIR=/home/iwasz/workspace/zephyr-sdk-0.11.2
export ZEPHYR_BASE=/home/iwasz/workspace/zephyrproject/zephyr/
```

And then I was able to build the project as usual (mind the paths are refering to my system, but I'm sure you can figure it out):

```sh
cd build
cmake -GNinja -DBOARD=nucleo_h743zi ..
ninja
# either 
openocd -f /home/iwasz/local/share/openocd/scripts/interface/stlink.cfg -f /home/iwasz/local/share/openocd/scripts/target/stm32h7x.cfg -c 'program zephyr.elf verify reset exit'
# or 
west flash
```

As simple as that. The next very important piece of information to refer to is [this page](https://docs.zephyrproject.org/latest/application/index.html).

# Updating Zephyr source and the toolchain (SDK)
Zephyr source:

```sh
cat ~/.zephyrrc 
cd ~/workspace/zephyrproject
west update
```

Updating the toolchain : !?!?!?

# Building in vscode
So now I'm going to try to recreate the above experience using only the vscode. 

# vscode setup
My vscode version is 1.44.1, and what is important is that I do not use [ms-vscode.cpptools](https://marketplace.visualstudio.com/items?itemName=ms-vscode.cpptools) extension. Instead, I have those (among others, which are irrelevant here):

- llvm-vs-code-extensions.vscode-clangd
- ms-vscode.cmake-tools
- marus25.cortex-debug
- webfreak.debug
- twxs.cmake

They replace the original plugin functionality and give a much better experience in my opinion. Especially crucial for me is seamless integration with the clangd language server which understands complex C++ as true compilers do.

Relevant pieces of configuration regarding these plugins I listed above would be (copy-paste from settings.json):

```json
"cmake.copyCompileCommands": "${workspaceFolder}/compile_commands.json"
```
This tells the cmake plugin to copy compile_commands.json from the build directory into the root directory of our project. This is where clangd is looking for it (tip : ```---query-driver``` accepts a coma separated list of globs).

```json
"clangd.arguments": [
    "--query-driver=/home/iwasz/workspace/zephyr-sdk-0.11.2/*",
    "--background-index",
    "--header-insertion=never"
]
```

The above instructs the clangd backend what toolchains are available. I'm not sure why couldn't it figure it basing on the ```compile_commands.json``` but I found you can run into problems when this parameter is not set properly. 

```json
"[cpp]": {
    "editor.defaultFormatter": "llvm-vs-code-extensions.vscode-clangd"
},
"[c]": {
    "editor.defaultFormatter": "llvm-vs-code-extensions.vscode-clangd"
},
"clangd.semanticHighlighting": false
```

The above 3 parameters aren't strictly required, but I fount they worked for me and improved the overall experience.


# Building
- Delete the build ```directory``` to get rid of our previous build.
- select kit [Unspecified]
- If you were to select a kit named ```[Unspecified]```, you would notice that configuring fails with : ```Could NOT find Zephyr (missing: Zephyr_DIR)```. This is the same message as when I invoked cmake without sourcing my ```~/.zephyrrc```.
- To resolve this add environment variables to the settings :
```json
"cmake.configureEnvironment": {
    "ZEPHYR_TOOLCHAIN_VARIANT": "zephyr",
    "ZEPHYR_SDK_INSTALL_DIR": "/home/iwasz/workspace/zephyr-sdk-0.11.2",
    "ZEPHYR_BASE": "/home/iwasz/workspace/zephyrproject/zephyr/"
}
```
- Add a kit which will tell vscode which board to use. This way you can have many of those kits and switch them when necessary (pro tip : a kit configuration also allows for the ```environmentVariables``` property to be set):
```json
{
    "name": "stm32h743zi-zephyr",
    "cmakeSettings": {
        "BOARD": "nucleo_h743zi"
    }
}
```

- Finally select a kit:

![Select a kit](http://iwasz.pl/files/zephyr-vscode/select-a-kit.png "Select")

- Configure and build. 

# Debugging

```json
{
    // Use IntelliSense to learn about possible attributes.
    // Hover to view descriptions of existing attributes.
    // For more information, visit: https://go.microsoft.com/fwlink/?linkid=830387
    "version": "0.2.0",
    "configurations": [
        {
            "cwd": "${workspaceRoot}",
            "executable": "./build/zephyr/zephyr.elf",
            "name": "Debug Microcontroller",
            "request": "launch",
            "type": "cortex-debug",
            "servertype": "openocd",
            "device": "STM32H743ZI",
            "configFiles": [
                "/home/iwasz/local/share/openocd/scripts/interface/stlink.cfg",
                "/home/iwasz/local/share/openocd/scripts/target/stm32h7x.cfg"
            ],
            // https://github.com/posborne/cmsis-svd/tree/master/data/STMicro
            "svdFile": "STM32H743x.svd"
        }
    ]
}
```

# Random tips
No ```<cstdio>``` or other C++ header? Check these settings in menuconfig:

- C Library -> C Library Implementation
  - Minimal C Library [is hosted here](https://github.com/zephyrproject-rtos/zephyr/tree/master/lib/libc/minimal). Now, I don't know if this implementation is used somewhere outside the Zephyr but using it disables libstdc++ (which is included in the toolchain).
  - Newlib C Library. [Here are the syscalls for newlib](https://github.com/zephyrproject-rtos/zephyr/blob/master/lib/libc/newlib/libc-hooks.c).
  - External C Library
- Changing prj.conf file clears ```ninja guiconfig``` config.
- **Save minimal** in ```ninja guiconfig``` works correctly only after **Save** was clicked beforehand.

# TODO

```
[cmake] CMake Warning at /home/iwasz/workspace/zephyrproject/zephyr/CMakeLists.txt:1396 (message):
[cmake]   
[cmake] 
[cmake]         The CMake build type was set to 'Debug', but the optimization flag was set to '-Os'.
[cmake]         This may be intentional and the warning can be turned off by setting the CMake variable 'NO_BUILD_TYPE_WARNING'
```

https://github.com/zephyrproject-rtos/zephyr/issues/21119
https://github.com/bus710/zephyr-rtos-development-in-linux

Have to tweak ```ninja menuconfig``` to turn off the optimisations (-Os is the default)

