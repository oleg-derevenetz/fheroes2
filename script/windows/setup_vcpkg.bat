@echo off

set VCPKG_ROOT=%~1
set PLATFORM=%~2
set PACKAGES=%~3

"%VCPKG_ROOT%\vcpkg.exe" --triplet "%PLATFORM%-windows" install sdl2 sdl2-mixer sdl2-image zlib
