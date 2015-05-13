"%~dp0\swig" -c++ -csharp -o "%~dp0\wrapper\libwav_wrap.cpp" "%~dp0\..\libwav\libwav.i"
copy /v /y /b "%~dp0\wrapper\libwav_wrap.cpp" "%~dp0\..\libwav"