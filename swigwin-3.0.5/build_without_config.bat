"%~dp0\swig" -c++ -csharp -o .\wrapper\libwav_wrap.cpp ..\libwav\libwav.i
copy /v /y /b "%~dp0\wrapper\libwav_wrap.cpp" "%~dp0\..\libwav"