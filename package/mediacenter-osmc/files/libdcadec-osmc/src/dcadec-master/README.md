dcadec
======

dcadec is a free DTS Coherent Acoustics decoder with support for HD extensions.

Supported features:

* Decoding of standard DTS core streams with up to 5.1 channels
* Decoding of DTS-ES streams with discrete back channel
* Decoding of High Resolution streams with up to 7.1 channels and extended bitrate
* Decoding of 96/24 core streams
* Lossless decoding of Master Audio streams with up to 7.1 channels, 192 kHz
* Downmixing to stereo and 5.1 using embedded coefficients

Features not implemented:

* Decoding of DTS Express streams
* Applying dynamic range compression and dialog normalization

Usage
-----

Help screen of the program is reproduced below.
```
Usage: ./dcadec [-26bcfhlmnPqSx] <input.dts> [output.wav]
dcadec is a free DTS Coherent Acoustics decoder. Supported options:

-2  Extract embedded 2.0 downmix if present, otherwise extract 5.1 downmix.

-6  Extract embedded 5.1 downmix.

-b  Force fixed point DTS core interpolation. Developer option, degrades sound
    quality.

-c  Force decoding of DTS core only without extensions.

-f  Use FIR filter for floating point DTS core LFE channel interpolation.

-h  Show this help message.

-l  Enable lenient decoding mode. Attempt to recover from errors by skipping
    non-decodable parts of the stream.

-m  Write a mono WAV file for each native DTS channel. Output file name must
    include `%s' sub-string that will be replaced with DTS channel name.

-n  No-act mode. Parse DTS bitstream without writing WAV file(s).

-P  Disable progress indicator.

-q  Be quiet. Disables informational messages and progress indicator. Warnings
    and errors are still printed.

-S  Don't strip padding samples for streams within DTS-HD container.

-x  Force use of X96 synthesis filter for DTS core interpolation. Developer
    option, degrades sound quality.

When run without output file name argument, prints information about DTS file
to stdout and exits.

Single dash in place of input or output file name argument means to read from
stdin or write to stdout, respectively.

dcadec comes with ABSOLUTELY NO WARRANTY. This is free software, and you are
welcome to redistribute it under certain conditions; see GNU Lesser General
Public License version 2.1 for details.
```

Examples
--------

Some dcadec usage examples follow.

* Decode DTS file to WAV:  

  ```
  $ ./dcadec input.dts output.wav
  ```

* Decode DTS file to multiple mono WAVs:  

  ```
  $ ./dcadec -m input.dts output_%s.wav
  ```

* Decode DTS file and play with mpv:  

  ```
  $ ./dcadec input.dts - | mpv -
  ```

* Decode DTS file and re-encode to FLAC:  

  ```
  $ ./dcadec input.dts - | flac --ignore-chunk-sizes -o output.flac -
  ```

* Demux DTS track #1 from MKV file, decode and re-encode to FLAC:

  ```
  $ mkvextract tracks input.mkv -r /dev/null 1:/dev/stdout | \
  ./dcadec - - | flac --ignore-chunk-sizes -o output.flac -
  ```
