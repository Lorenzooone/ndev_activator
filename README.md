# ndev_activator

ndev_activator is a simple multi-platform program to allow proper boot of a NDEV console.

## Dependencies

ndev_activator has three build dependencies: CMake, g++ and git.
Make sure all are installed.
On MacOS, [Homebrew](https://brew.sh/) can be used to install both CMake and git. An automatic popup should appear to install g++ at Compile time.

ndev_activator has one library dependency: [libusb](https://libusb.info/).
It should get downloaded automatically via CMake during the building process.

Below, the command for installing the compile-time dependencies on Debian-based distributions.

```
sudo apt update
sudo apt install \
    g++ \
    git \
    cmake
```

On Windows, you may need to install the Visual C++ Redistributable set of libraries. They are available here: [Official Microsoft VC Redist Link](https://learn.microsoft.com/en-us/cpp/windows/latest-supported-vc-redist?view=msvc-170#latest-microsoft-visual-c-redistributable-version).

## Compilation

To compile the program, assuming CMake, git and g++ are installed on the system, this is the command which should be launched:

```
cmake -B build ; cmake --build build --config Release
```

This will download libusb, which may take a while during the first execution of the command. Later runs should be much faster.
On MacOS, you may also be prompted to install the Apple Command Line Developer Tools first.

### Docker Compilation

Alternatively, one may use Docker to compile the Linux version for its different architectures by running: `docker run --rm -it -v ${PWD}:/home/builder/ndev_activator lorenzooone/ndev_activator:<builder>`
The following builders are available: builder32, builder64, builderarm32 and builderarm64.

## Notes
- On Linux, you may need to include the udev USB access rules. You can use the .rules files available in the repository's usb\_rules directory, or define your own. For ease of use, releases come bundled with a script to do it named install\_usb\_rules.sh. It may require elevated permissions to execute properly. You may get a permission error if the rules are not installed.
- When using NDEV devices on Windows, ndev_activator is compatible with WinUSB, meaning there is no need to have access to the official driver. To install and use WinUSB, plug in your device, download a software to install drivers like [Zadig](https://zadig.akeo.ie/), select the device in question and select WinUSB. Then install the driver and wait for it to finish. The devices should now work properly with this application.
