@echo off

set VCPKG_ROOT=%~1
set PLATFORM=%~2
set PACKAGES=%~3

"%VCPKG_ROOT%\vcpkg.exe" --triplet "%PLATFORM%-windows" install sdl2 sdl2-mixer[fluidsynth,libflac,libvorbis,mpg123] sdl2-image zlib

if not exist "%PACKAGES%\zlib\include"          mkdir "%PACKAGES%\zlib\include"
if not exist "%PACKAGES%\zlib\lib\%PLATFORM%"   mkdir "%PACKAGES%\zlib\lib\%PLATFORM%"
if not exist "%PACKAGES%\sdl2\include"          mkdir "%PACKAGES%\sdl2\include"
if not exist "%PACKAGES%\sdl2\lib\%PLATFORM%"   mkdir "%PACKAGES%\sdl2\lib\%PLATFORM%"
if not exist "%PACKAGES%\extras\lib\%PLATFORM%" mkdir "%PACKAGES%\extras\lib\%PLATFORM%"

xcopy /Y /Q    "%VCPKG_ROOT%\packages\sdl2_%PLATFORM%-windows\bin\SDL2.dll"             "%PACKAGES%\sdl2\lib\%PLATFORM%"
xcopy /Y /Q /S "%VCPKG_ROOT%\packages\sdl2_%PLATFORM%-windows\include\SDL2"             "%PACKAGES%\sdl2\include"
xcopy /Y /Q    "%VCPKG_ROOT%\packages\sdl2_%PLATFORM%-windows\lib\SDL2.lib"             "%PACKAGES%\sdl2\lib\%PLATFORM%"

xcopy /Y /Q    "%VCPKG_ROOT%\packages\sdl2-image_%PLATFORM%-windows\bin\SDL2_image.dll" "%PACKAGES%\sdl2\lib\%PLATFORM%"
xcopy /Y /Q /S "%VCPKG_ROOT%\packages\sdl2-image_%PLATFORM%-windows\include\SDL2"       "%PACKAGES%\sdl2\include"
xcopy /Y /Q    "%VCPKG_ROOT%\packages\sdl2-image_%PLATFORM%-windows\lib\SDL2_image.lib" "%PACKAGES%\sdl2\lib\%PLATFORM%"

xcopy /Y /Q    "%VCPKG_ROOT%\packages\sdl2-mixer_%PLATFORM%-windows\bin\SDL2_mixer.dll" "%PACKAGES%\sdl2\lib\%PLATFORM%"
xcopy /Y /Q /S "%VCPKG_ROOT%\packages\sdl2-mixer_%PLATFORM%-windows\include\SDL2"       "%PACKAGES%\sdl2\include"
xcopy /Y /Q    "%VCPKG_ROOT%\packages\sdl2-mixer_%PLATFORM%-windows\lib\SDL2_mixer.lib" "%PACKAGES%\sdl2\lib\%PLATFORM%"

xcopy /Y /Q    "%VCPKG_ROOT%\packages\zlib_%PLATFORM%-windows\bin\zlib1.dll"            "%PACKAGES%\zlib\lib\%PLATFORM%"
xcopy /Y /Q /S "%VCPKG_ROOT%\packages\zlib_%PLATFORM%-windows\include"                  "%PACKAGES%\zlib\include"
xcopy /Y /Q    "%VCPKG_ROOT%\packages\zlib_%PLATFORM%-windows\lib\zlib.lib"             "%PACKAGES%\zlib\lib\%PLATFORM%"

xcopy /Y /Q    "%VCPKG_ROOT%\packages\libflac_%PLATFORM%-windows\bin\FLAC.dll"          "%PACKAGES%\extras\lib\%PLATFORM%"
xcopy /Y /Q    "%VCPKG_ROOT%\packages\libogg_%PLATFORM%-windows\bin\ogg.dll"            "%PACKAGES%\extras\lib\%PLATFORM%"
xcopy /Y /Q    "%VCPKG_ROOT%\packages\libvorbis_%PLATFORM%-windows\bin\vorbis.dll"      "%PACKAGES%\extras\lib\%PLATFORM%"
xcopy /Y /Q    "%VCPKG_ROOT%\packages\libvorbis_%PLATFORM%-windows\bin\vorbisenc.dll"   "%PACKAGES%\extras\lib\%PLATFORM%"
xcopy /Y /Q    "%VCPKG_ROOT%\packages\libvorbis_%PLATFORM%-windows\bin\vorbisfile.dll"  "%PACKAGES%\extras\lib\%PLATFORM%"
xcopy /Y /Q    "%VCPKG_ROOT%\packages\mpg123_%PLATFORM%-windows\bin\mpg123.dll"         "%PACKAGES%\extras\lib\%PLATFORM%"
