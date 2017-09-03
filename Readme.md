# :zap: SuperFast Codecs
## Multi-threaded Opus and AAC codec drivers for fre:ac

This repository provides multi-threaded Opus and AAC codec drivers for use with the [_fre:ac audio converter_](https://github.com/enzo1982/freac/). The components use multiple instances of the respective codecs in parallel to provide faster processing on systems with multiple CPU cores.

## Technology

The idea to use multiple codec instances to speed-up audio encoding goes back to 2006 when the [LAME MT](http://softlab-pro-web.technion.ac.il/projects/LAME/html/lame.html) project tried to build a multi-threaded MP3 encoder. This project picks up the original idea behind LAME MT and takes it to Opus and AAC encoding.

To achieve a speed-up, the audio stream is divided into overlapping chunks of audio frames. The chunks are then given to the codec instances in a round-robin manner. Finally, encoded packets are taken from the codec instances and written to the output file in the correct order.

For technical details on how this is implemented, please refer to [this blog post](https://freac.org/developer-blog-mainmenu-9/14-freac/257-introducing-superfast-conversions/) or [the PDF](https://github.com/enzo1982/superfast/doc/SuperFast Codecs.pdf).

## Download

Download an experimental fre:ac build with multi-threaded Opus, FAAC* and Core Audio converters:
- Windows: [x86-64](https://github.com/enzo1982/superfast/releases/download/v0.1/freac-1.1-alpha-20170902-superfast-windows-x64.zip), [i686](https://github.com/enzo1982/superfast/releases/download/v0.1/freac-1.1-alpha-20170902-superfast-windows.zip)
- macOS: [Universal Binary](https://github.com/enzo1982/superfast/releases/download/v0.1/freac-1.1-alpha-20170902-superfast-macos.dmg) (x86-64, i686 and PPC)
- Linux: [x86-64](https://github.com/enzo1982/superfast/releases/download/v0.1/freac-1.1-alpha-20170902-superfast-linux-x64.tar.gz), [i686](https://github.com/enzo1982/superfast/releases/download/v0.1/freac-1.1-alpha-20170902-superfast-linux.tar.gz)
- FreeBSD: [x86-64](https://github.com/enzo1982/superfast/releases/download/v0.1/freac-1.1-alpha-20170902-superfast-freebsd-x64.tar.gz), [i686](https://github.com/enzo1982/superfast/releases/download/v0.1/freac-1.1-alpha-20170902-superfast-freebsd.tar.gz)

\* The FAAC codec is provided as a fallback when the Core Audio codecs are not available.

## Repository Contents

The `components` folder contains multi-threaded drivers for the following encoders:

- Core Audio AAC/ALAC (iTunes)
- Free Advanced Audio Coder (FAAC)
- Opus

Support for additional codecs will be added in the future.

## Compiling

The following packages must be installed in order to compile these components:

- the [_smooth Class Library_](https://github.com/enzo1982/smooth/)
- the [_BoCA_](https://github.com/enzo1982/boca/) component framework

When all prerequisites are met, run `make` followed by `sudo make install` to compile and install the multi-threaded encoder components.

To actually use the components, please install the [_fre:ac audio converter_](https://github.com/enzo1982/freac/).

----
The official fre:ac homepage: https://www.freac.org/

robert.kausch@freac.org,
Robert Kausch
