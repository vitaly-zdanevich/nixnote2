# NixNote2

## Introduction

NixNote is an Evernote desktop client for Linux. It can also be built on macOS
and Windows, but Linux is the primary target.

* [Getting started](https://github.com/robert7/nixnote2/wiki/Getting-started) (historic upstream wiki)
* [Features](https://github.com/robert7/nixnote2/wiki/Features) (historic upstream wiki)
* [CHANGELOG](debian/changelog)
* [Contributing](CONTRIBUTING.md)
* [Releases](https://github.com/vitaly-zdanevich/nixnote2/releases)

![GitHub Actions](https://github.com/vitaly-zdanevich/nixnote2/actions/workflows/cicd.yaml/badge.svg)

## Packages
### Debian, Ubuntu and derivatives official repositories
If your distribution is based on **Debian 10 (Buster), Ubuntu 19.04 (Disco),
or a newer distribution version**, you may be able to install NixNote2 from
official repositories using:

``` bash
sudo apt update
sudo apt install nixnote2 -y
```

Distribution repositories may contain older NixNote2 builds. Use the GitHub
Releases page for current AppImages from this repository.

### Ubuntu
The historic NixNote PPA may contain older upstream builds. For the current Qt 6
port in this repository, prefer the GitHub Releases AppImages or build from
source.

``` bash
sudo add-apt-repository ppa:nixnote/nixnote2-stable -y
sudo apt update
sudo apt install nixnote2 -y
```

There is also a historic
[development branch PPA](https://code.launchpad.net/~nixnote/+archive/ubuntu/nixnote2-develop).
Do not enable both stable and development PPAs at the same time.

### AppImage
This is suitable for most recent Linux distributions. Download an AppImage from
the [GitHub Releases](https://github.com/vitaly-zdanevich/nixnote2/releases)
page, mark it as executable, and run it.

The CI workflow builds AppImages for `amd64` and `arm64` on `v*` release tags.
When the release workflow succeeds, the files are attached to the matching
versioned GitHub Release.

### Windows
Windows binaries can be found at a fork https://github.com/boo-yee/nixnote2 under [Releases](https://github.com/boo-yee/nixnote2/releases).

### Other
Official support is limited to the source tree and GitHub Release AppImages.
Here are links to further community builds:

#### Arch Linux
AUR package [nixnote2-git](https://aur.archlinux.org/packages/nixnote2-git/)
may be available for Arch Linux users.

#### Gentoo Linux
Current source build dependencies for Gentoo are listed in the manual build
section below. A historical custom Portage [overlay] also exists:

``` bash
layman -o https://raw.githubusercontent.com/bbugyi200/portage-overlay/master/repository.xml -f -a bbugyi200
emerge nixnote:2
```

Another option: [AppImage ebuild in Guru](https://github.com/gentoo/guru/tree/master/app-office/nixnote-bin), with only single dependency: fuse.

[overlay]: https://github.com/bbugyi200/portage-overlay/blob/master/app-misc/nixnote/nixnote-9999.ebuild

#### Fedora
https://copr.fedorainfracloud.org/coprs/nunodias/nixnote2/

#### OpenSUSE
https://software.opensuse.org/package/nixnote2

## Building from source

This app is mainly targeted at Linux. macOS and Windows build notes are kept
below, but they may need extra work after the Qt 6 migration.

Application development has historically used the
[CLion](https://www.jetbrains.com/clion/) IDE with an open source license from
[JetBrains](https://www.jetbrains.com/?from=nixnote2).

### Linux - docker build
Docker helpers are available, but the GitHub Actions workflow is the current
source of truth for reproducible AppImage builds. Basic familiarity with Docker
is helpful.

More info in: [DOCKER README](docs/DOCKER-README.md)

### Linux - manual build
* Install development dependencies - look in content of [this docker file](development/docker/Dockerfile.ubuntu_focal)
  or [.github/workflows/cicd.yaml](.github/workflows/cicd.yaml)
  or [debian/control](debian/control)
  to see examples of what is needed for Ubuntu. If you use another distribution/version,
  you may need adjust packages.
* Qt: install Qt 6 packages from your distribution, or download Qt 6 directly
  from [qt.io/download](https://www.qt.io/download).
  The qmake build now requires `qmake6`.
* On Gentoo, the relevant packages are:
  `dev-qt/qtbase:6`, `dev-qt/qtwebengine:6`, `dev-qt/qtwebchannel:6`,
  `app-text/poppler[qt6]`, `app-text/htmltidy`, `net-misc/curl`,
  `app-text/hunspell`, and `dev-util/pkgconf`.
* Get the latest source from GitHub.
  * Use the `master` branch for stable releases.
* Build.
* Optional: create an [AppImage package](https://appimage.org/) using
  [linuxdeploy](https://github.com/linuxdeploy/linuxdeploy).

```bash
./development/build-with-qmake.sh release clean /usr/lib browser
```
`build-with-qmake.sh` is a convenience script. You can also build without it like:
`qmake6 CONFIG+=release PREFIX=appdir/usr`, then `make && make install`.

This assumes that libtidy is installed in the system default location.

If you installed tidy from the historic NixNote PPA, for example with the
`nixnote2-tidy` package, the command could be:
`./development/build-with-qmake.sh release noclean /usr/lib/nixnote2/tidy browser`.

If the build succeeds, you should have a `qmake-build-release/nixnote2` binary
and a deployment copy in `appdir`. Run the deployment copy:
`./appdir/usr/bin/nixnote2`.

For a local Qt6 qmake build, install into the local `appdir` layout first:

```bash
PKG_CONFIG_PATH="$PWD/development/pkgconfig" qmake6 CONFIG+=release PREFIX=appdir/usr QMAKE_RPATHDIR+=/usr/lib nixnote2.pro
make -j$(nproc)
make -j$(nproc) install
```

Then run the installed binary from the repository root:

```bash
./appdir/usr/bin/nixnote2
```

The raw `qmake-build-release/nixnote2` binary is not enough by itself because NixNote
expects the runtime data directory next to the binary as `../share/nixnote2`.
Run the application inside a normal graphical session. The offscreen platform is only
useful for automated tests:

```bash
./development/run-cli-smoke-tests.sh
./development/run-tests.sh release noclean /usr/lib
```


```bash
# Optional second step: if all got well you may try to create AppImage package
./development/create-AppImage-linuxdeploy.sh
```

Preparation steps
* You can either install the `nixnote2-tidy` package from NixNote PPA or build yourself from source.
* Alternative 1: Install nixnote from the historic
  [PPA](https://github.com/robert7/nixnote2/wiki/NixNote-PPA):
  * ..this includes nixnote2-tidy package
  * in this case libtidy is installed in /usr/lib/nixnote2/tidy
* Alternative 2: Build the tidy library from source:
  * clone [source code](https://github.com/htacg/tidy-html5) and switch to the master branch
  * follow [build instructions](https://github.com/htacg/tidy-html5/blob/next/README/BUILD.md)
    * short version:
    * cd build/cmake
    * cmake ../..  -DCMAKE_INSTALL_PREFIX=/usr -DCMAKE_BUILD_TYPE=Release
    * make
    * make DESTDIR=/some/directory install
    * library is now copied to /some/directory/lib (/some/directory/lib should be then passed as 3rd argument to
      build-with-qmake.sh)

If it doesn't work, compare your local environment with
[.github/workflows/cicd.yaml](.github/workflows/cicd.yaml), which is the
current CI build recipe.

### macOS
Build from source. This is basically the same as for Linux:

```bash
./development/build-with-qmake.sh
```
`build-with-qmake.sh` is just kind of convenience script. You can also build without it like:
`qmake6 CONFIG+=debug PREFIX=appdir/usr`, then `make && make install`.

Upon successful completion you will have the NixNote2.app bundle in the build directory (e.g. qmake-build-debug/NixNote2.app).

Dependencies can come from MacPorts, Fink or HomeBrew (I use MacPorts).
Use Qt 6 packages. The expected dependencies are Qt 6, Qt WebEngine, Qt WebChannel,
poppler with Qt 6 bindings, hunspell, curl, and tidy.

The resulting application still depends on MacPorts (or Fink or HomeBrew). To turn this into a standalone app bundle that can be
deployed anywhere:

```bash
> cd build
> macdeployqt NixNote2.app [-no-strip]
```

As far as I can tell this will find and copy all required dependencies into the app bundle and modify them so they
can be loaded from inside that bundle (wherever it ends up).

### Windows
The Windows build notes are legacy and may need updates after the Qt 6 port.
Build with MinGW32 for a 32-bit NixNote2 (if you want a 64-bit one, you need to use MinGW-w64 or MSVC):

Unlike Unix-like systems, Windows is not shipped with a bash environment, so you need to install one first.

#### Download development dependencies:

##### Download the third-party libraries:
If you want to download binary third-party library files, make sure they are built for
the same compiler and Qt 6 version that you use for NixNote. If you want to build them
yourself, you can download them from the following links:

[poppler](https://poppler.freedesktop.org/)

[tidy](https://github.com/htacg/tidy-html5/)

[hunspell](https://github.com/hunspell/hunspell/)

##### Download Qt:
[Qt](https://download.qt.io/) with MinGW32

Install Qt 6 with Qt WebEngine and Qt WebChannel. QtWebKit is no longer used.

(Advice: You may want to add the path to qmake.exe and mingw32-make.exe to the PATH environment, so that you do not have to type the full path when building the application and libraries later. You can do this by hand or running qtenv2.bat.)

#### Build third-party libraries:
If you need to build tidy-html5 by yourself, its README file may help. The poppler link points to the binary files, you can use it directly.

About hunspell, as its README instructs us to use MSYS2 and Cygwin, which may be in x64, so to get compatible libraries, you have to set gcc and g++ as the one you used previously for tidy under MSYS2 or Cygwin (by alias or export command) and build like this:
```bash
MSYS2/Cygwin # autoreconf -vfi
MSYS2/Cygwin # ./configure --build=i686-pc-mingw32 --host=i686-pc-mingw32 --target=i686-pc-mingw32
MSYS2/Cygwin # mingw32-make.exe
```
Then you will get the hunspell dll file.

Then, create folders as winlib/includes in this repository folder and copy the files under poppler, tidy and hunspell include folders to winlib/includes. The structure is:

```bash
nixnote2
|
`--winlib
   |
   includes
   |
   `--poppler
   |  |
   |  `--qt6
   |  |  |
   |  |  `...
   |  `--cpp
   |     |
   |     `...
   `--tidy
   |  |
   |  `--tidyplatform.h
   |  |
   |  `--tidyenum.h
   |  |
   |  ...
   |
   `--hunspell
      |
      `--affentry.hxx
      |
      ...
```
And also copy the dll files libtidy.dll, libpoppler.dll, libpoppler-qt6.dll, libhunspell-1.7-0.dll to winlib.

#### Build the application:

(This part can be run under any bash environment, not necessarily MSYS or Cygwin.)

```bash
git clone https://github.com/vitaly-zdanevich/nixnote2.git
cd nixnote2
qmake.exe -set HUNSPELL_VERSION 1.7-0
qmake.exe CONFIG+=release nixnote2.pro
qmake.exe -unset HUNSPELL_VERSION
mingw32-make.exe
strip.exe qmake-build-release/nixnote2.exe
```

Change `HUNSPELL_VERSION` as needed. Finally, you will get
`qmake-build-release/nixnote2.exe`.

#### Deployment:

First, copy the nixnote2.exe to the deployment_folder, then execute the following commands:

```bash
windeployqt.exe --compiler-runtime --libdir [deployment_folder] [deployment_folder]
bash development/deploy-on-windows.sh [deployment_folder]
```

If you need spell check, you have to download the dictionary files and copy the .aff and .dic file to the deployment folder. You may want to download them [here](https://github.com/wooorm/dictionaries).


## Donations
If you would like to support the project, you can send me some little amount via paypal: https://paypal.me/nixnote2
