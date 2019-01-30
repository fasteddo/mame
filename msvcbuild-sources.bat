@rem old
@rem make -j11 MSVC_BUILD=1 DEBUG=1 SYMBOLS=1 NOWERROR=1 DIRECTINPUT=8 DONT_USE_NETWORK=1 MINGW64=c:\msys64\mingw64

@rem http://wiki.mamedev.org/index.php?title=Building_MAME_using_Microsoft_Visual_Studio_compilers

@rem make SUBTARGET=tiny vs2015 
@rem make vs2015 SUBTARGET=drivername SOURCES=src\mame\drivers\drivername.cpp
make vs2017 SOURCES=^
src\mame\drivers\1942.cpp,^
src\mame\drivers\centiped.cpp,^
src\mame\drivers\dkong.cpp,^
src\mame\drivers\galaga.cpp,^
src\mame\drivers\galaxian.cpp,^
src\mame\drivers\m52.cpp,^
src\mame\drivers\pacman.cpp ^
PYTHON_EXECUTABLE=d:/msys64/mingw64/bin/python.exe
