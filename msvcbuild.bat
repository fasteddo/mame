@rem old
@rem make -j11 MSVC_BUILD=1 DEBUG=1 SYMBOLS=1 NOWERROR=1 DIRECTINPUT=8 DONT_USE_NETWORK=1 MINGW64=c:\msys64\mingw64

@rem http://wiki.mamedev.org/index.php?title=Building_MAME_using_Microsoft_Visual_Studio_compilers

@rem make SUBTARGET=tiny vs2015 
@rem make vs2015 SUBTARGET=drivername SOURCES=src\mame\drivers\drivername.cpp
make vs2017 PYTHON_EXECUTABLE=c:/msys64/mingw64/bin/python.exe
