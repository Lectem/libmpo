libmpo

A future library to decode and encode MPO (multiple picture object) files.
Those are used mainly for 3D cameras and the Nintendo 3DS.


Main goal :
- Load / Write 3D images (Multi-frame Disparity) through libjpeg.
- Give others enough material to work with as the standard isn't clear on some points(especially for the Index IFD)


Currently working :
- MPF Index IFD parsing

Partially working :

- Simple Multi-frame Disparity MPO file compression


Requirements :
- libjpeg8 or later


TODO (In *order* of priority):
- Compression : Update Index IFD with images lengths and offsets
- Give a good interface for compression and decompression
- Attribute IFD support
- A good refactoring
- Remove printf and other debugging stuff
- A makefile (and pre-built binaries?)
- Everything else


Will not be done :
- Exif and other markers support (you will be able to write/read it trough your own handlers)

Might not be done :
- MPO files with other type than Multi-frame Disparity