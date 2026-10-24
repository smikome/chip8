# chip8
Chip-8 emulator written in C++

# Setup
## Install vcpkg (if not already installed).
### Windows
```
cd ~
git clone https://github.com/microsoft/vcpkg
cd vcpkg
./bootstrap-vcpkg.bat
```
#### Adding vcpkg to the PATH
1. Open the Start Menu and search for "Environment Variables".
2. Click "Edit the system environment variables".
3. In the System Properties window, click the "Environment Variables" button.
4. Under "User variables" (or "System variables"), find the Path variable and click "Edit".
5. Click "New" and add the path to your vcpkg directory (the folder containing vcpkg.exe).
6. Click "OK" to save your changes.
7. Restart your terminal and run vcpkg --version to verify the installation.

### MacOS
```
cd ~
git clone https://github.com/microsoft/vcpkg
cd vcpkg
./bootstrap-vcpkg.sh
```
#### Adding vcpkg to the PATH
1. Open your shell configuration file in a text editor. Use ~/.zshrc if you are using zsh (the default on macOS), or ~/.bash_profile if you are using bash.
2. Add the following line, replacing $HOME/vcpkg with the actual path to your vcpkg directory:
```
export PATH="$HOME/vcpkg:$PATH"
```
4. Save the file and reload your shell configuration:
```
source ~/.zshrc
```
5. Verify the installation by running:
```
vcpkg --version
```
6. Optional: You can also set the VCPKG_ROOT environment variable to make it easier to integrate vcpkg with CMake and other build systems:
```
export VCPKG_ROOT="$HOME/vcpkg"
```

## Build
```
vcpkg install
cmake --preset default
cmake --build build
```

# TODO
