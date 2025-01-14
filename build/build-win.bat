@echo %off

set PCRE_ROOT=C:/pcre
set PATH=C:\bison\bin;%PATH%
cmake --version && cmake .. -G "Visual Studio 16 2019%VSARCH%" -DCMAKE_INSTALL_PREFIX="%CD:\=/%/install2" -DCMAKE_C_FLAGS="/DPCRE2_STATIC" -DCMAKE_CXX_FLAGS="/DPCRE2_STATIC" -DPCRE2_INCLUDE_DIR=%PCRE_ROOT%/include -DPCRE2_LIBRARY=%PCRE_ROOT%/lib/pcre2-8-static.lib -DBISON_EXECUTABLE=C:/bison/bin/bison.exe
cmake --build . --config Release --target install

pause
