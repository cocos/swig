set PCRE_ROOT=C:/pcre

cmake .. -G "Visual Studio 16 2019%VSARCH%" -DCMAKE_INSTALL_PREFIX="%PCRE_ROOT:\=/%" && cmake --build . --config Release --target install

