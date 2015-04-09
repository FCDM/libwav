libwav

Compiling --- How To
1*. adjust config.ini in ./swigwin-3.0.5 (if you know what your changes mean to the script)
2*. run build.py with working directory of ./swigwin-3.0.5
3. open visualstudio 2013, build the solution
Done Compiling.
4. wrappers are at ./swigwin-3.0.5/wrappers (include all of them) (if you did not edit the config)
5. remember to put libwav.dll under your runtime directory (you might or might not have to rename it to libwav)

*if you don't need a swig wrapping, simply remove the dependency in vs solution.

