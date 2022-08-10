set DIR=%~dp0
set REPO_ROOT=%DIR%..\..

for %%i in ("%REPO_ROOT%") do SET REPO_ROOT=%%~fi

set PATH=%REPO_ROOT%/bison/bin;%PATH%
set BISON_EXE=%REPO_ROOT%/bison/bin/bison.exe
set PCRE_ROOT=%REPO_ROOT%/pcre2-pcre2-10.39/pcre-install

cmake --version && cmake .. -G "Visual Studio 16 2019" -DCMAKE_INSTALL_PREFIX=%REPO_ROOT%/swig-windows -DCMAKE_C_FLAGS="/DPCRE2_STATIC" -DCMAKE_CXX_FLAGS="/DPCRE2_STATIC" -DPCRE2_INCLUDE_DIR=%PCRE_ROOT%/include -DPCRE2_LIBRARY=%PCRE_ROOT%/lib/pcre2-8-static.lib -DBISON_EXECUTABLE=%BISON_EXE%
cmake --build . --config Release --target install