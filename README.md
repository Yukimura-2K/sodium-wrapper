# sodium-wrapper

This is a set of C++11 wrappers to the [libsodium](https://download.libsodium.org/doc/) library.

## Current status

* **Experimental and incomplete, very early alpha.**
* Interfaces are incomplete and API is subject to change.
* No cryptographic audit yet.

I'm still figuring out how to best map libsodium's C-API to C++
classes. Therefore, the following C++ API is subject to change
at any time. Don't use yet for production code or anything serious.
This is an (self-)educational repo/project for now.

USE AT YOUR OWN RISK. YOU'VE BEEN WARNED.

Criticism and pull requests welcome, of course.

## Roadmap (tentative)

* Fix: one unit test still fails (only on Debug builds) on Windows.
* Update to newest libsodium (in progress).
* Add wrappers to missing libsodium calls (in progress).
* Try to turn it into a header-only wrapper (in progress).
* Change API to lower case to make it more C++11, STL- and Boost-ish (in progress).
* Change API to reflect more faithfully libsodium's C-API naming scheme.
* Add wrappers to new 1.0.14+ streaming API (done).
* Replace ad-hoc streaming classes by new 1.0.14+ streaming API.
* Adapt Boost.Iostreams filters to use the new 1.0.14+ streaming API.
* Use updated API in some (toy) projects to test for suitability.
* Tag 0.1 to indicate semi-stable API. Seek user feedback. Update API if needed. Repeat.
* API freeze, lots more of testing and auditing, more user feedback.
* Cryptographic audit, e.g. to check for unintended side-channel attacks in wrappers.
* Initial release of API 1.0.
* Setting up release branch.
* More developments, tracking libsodium's updates, etc.

## Requirements

* Libraries:
  * [libsodium](https://github.com/jedisct1/libsodium) 1.0.16+
  * [Boost](https://www.boost.org/) 1.66.0+
    * Boost.Test for unit testing
	* Boost.Iostreams for streaming APIs.

* Build System:
  * [CMake](https://cmake.org/) 3.5.1+
  * A C++11 capable/compatible compiler:
    * (Unix) [Clang](https://clang.llvm.org/) 3.8.0+
	* (Unix) [GCC](https://gcc.gnu.org/) 5.4.0+
	* (Windows) [Microsoft Visual Studio 2017](https://www.visualstudio.com/vs/) 15.6.6+ and [vcpkg](https://github.com/Microsoft/vcpkg).

## Building

### Building on Unix (*BSD, Linux, ...)

First of all, get and install all the prerequisites above.
If your package manager has installed older versions of libsodium, boost,
cmake etc in */usr* prefix, get the newest ones as source, compile and
install them into prefix */usr/local*. Make sure that */usr/local/bin*
precedes */usr/bin* in `PATH`.

To compile:

```
cd sodium-wrapper    # where CMakeLists.txt is located
mkdir build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
make
```

Use `Debug` instead of `Release` to generate a debug build.

If you wish CMake to choose a specific compiler, set `CXX` and
`CC` environment variables accordingly:

```
env CXX=clang++ CC=clang cmake ..
```

or

```
env CXX=g++6 CC=gcc6 cmake ..
```

### Building on Windows

1. First of all, install [Visual Studio 2017](https://www.visualstudio.com/vs/).
2. Then, install [vcpkg](https://docs.microsoft.com/en-us/cpp/vcpkg):
     * Open "Developer Command Prompt for VS 2017"
	 * `cd ROOTDIR_OF_VCPKG` # e.g. in \Users\YOU, will install \Users\YOU\vcpkg
	 * `git clone https://github.com/Microsoft/vcpkg.git`
	 * `cd vcpkg`
	 * `.\bootstrap-vcpkg.bat`
	 * `vcpkg integrate install`
	 * `vcpkg integrate powershell`
3. Fetch and compile libsodium, Boost, and dependencies:
     * `vcpkg install boost:x86-windows`
	 * `vcpkg install boost:x64-windows`
	 * `vcpkg install sodium:x86-windows`
	 * `vcpkg install sodium:x64-windows`

vcpkg will fetch, compile, and install boost, libsodium, and
all of their dependencies in both 32-bit (*x86-windows*) and
64-bit (*x64-windows*) debug and release architectures.
	 
Thanks to the magic of vcpkg, installed packages will be automatically
found by Visual Studio: there is no need to add include or library folders
to your VS projects for them.

4. Edit *CMakeSettings.json* by adjusting the path to YOUR
installation of vcpkg.

5. In Visual Studio 2017, open the folder *sodium-wrapper*.
VS will detect CMakeLists.txt and CMakeSettings.json and
will run cmake automatically in the background. Choose a platform
like `x64-Debug`, `x64-Release`, `x86-Debug`, `x86-Release`, wait for cmake
to generate the VS project files (the CMake menu will then show
*Build All*, *Rebuild All* etc.), and then build the project via
VS's CMake menu. If the CMake menu initially doesn't show *Build All*
and so on, you can force a CMake invocation by saving *CMakeLists.txt*
again.

## Running the executables

Successfully compiling sodium-wrapper will create 3 types of binaries:

1. A dynamic library *libwrapsodium.so* or *wrapsodium.dll*
2. A stand-alone test executable *sodiumtester* or *sodiumtester.exe*
3. A set of test units *test\_SOMETHING* or *test\_SOMETHING.exe*

From a user perspective, the wrapper per se consists of the headers
in the *include* directory, and the dynamic library. Later on, when
going header-only (if possible), the wrapper will be only the *include*
directory.

*sodiumtester* in an interactive demo that shows a couple of simple tests.

The regression test suite *test\_SOMETHING* exercises different aspects of the
wrapper / API.

### Running on Unix

On Unix, just execute the binaries. Assuming you're still in the
*build* directory:

```
./sodiumtester
make test         # run all tests
cd tests
./test_key        # run individual tests
./test_nonce
```

Some tests output useful information messages such as timing
information etc. To display those, add `--log_level=message`
on the command line of the individual test:

```
./test_helpers --log_level=message
```

### Running on Windows

#### Running via Visual Studio

You can run the executables via Visual Studio's CMake menu.

To run the test suite:
1. build the whole project first (CMake / Build All),
2. then copy manually *wrapsodium.dll* in the folder containing the
   *test\_SOMETHING* exe files,
3. finally run the whole test suite
   (CMake / Tests / Run sodiumwrapper CTests)

Failed tests will show up in the Output window.

Manually debug one test unit with
CMake / Debug from Build Folder / test\_SOMETHING.exe

#### Running manually from PowerShell or cmd.exe

The executables are in the path specified by *buildRoot* in
*CMakeSettings.json*. The main point to consider is that the
executables need both *libsodium.dll* and *wrapsodium.dll*
in the same folder. You need to copy the *wrapsodium.dll*
(of the corresponding build and architecture) into
the *tests\Debug* or *tests\Release* folder before running
the binaries from there.

On my system:

```
cd \Users\fhajji\CMakeBuilds\{some-hash}\build\x64-Debug\Debug
.\sodiumtester.exe
copy wrapsodium.dll ..\tests\Debug
cd ..\tests\Debug
.\test_key.exe
.\test_nonce.exe
```

Replace `Debug` by `Release` to test the release build.
Replace `x64` by `x86` to test the 32-bit versions.

## Copyright

sodium-wrapper is Copyright (C) 2018 Farid Hajji. It is released under
the ISC License. Please refer to the file LICENSE.md
