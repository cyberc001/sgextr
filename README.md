**sgextr** is a simple utility for extracting files from MPK archives from following games:
- STEINS; GATE
- STEING; GATE 0

You can locate the files in the games' files, in USRDIR directory.
They usually contain:
- OGG files for voices/music
- PNG files for backdrops
- some unknown combination of PNG files with all non-transparent pixels laid in rows without any gaps and LAY files, presumably to save space
- some unknown SC3 script files
- images in DDS (DirectDraw surface) format

**Usage:**

`./sgextr <MPK archive file name> [file to extract 1] [file to extract 2] ... `

Specify no files to list files in a MPK archive.

Extract all files by specifying 'all' as first file name.

**Building:**

Compile with `compile.sh` bash script or by simply passing sgextr.c to a C compiler.

**MPK file format description:**

*Archive header:*

[0] 4-byte file signature MPK\0 (4D 50 4B 00)

[4] unknown

[8] 4-byte count of file entries

The header is then followed by an array of file entries.

*File entries:*

[0] 4-byte file number

[4] 8-byte file offset (in bytes from the beginning of the archive)

[12] 8-byte file size in bytes

[20] 8-byte redundant file size in bytes

[28] 224-byte file name, null-terminated and padded with zeroes

Each file entry has a total length of 256 bytes.

The array of file entries is then followed by continious file data which file entries point to and separate.
