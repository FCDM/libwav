%module libwav
%{
#include	"libwav.h"
%}


%include	"std_string.i"
%include	"stdint.i"
%include	"cpointer.i"
%include	"std_except.i"
%include	<windows.i>

%apply	uint64_t	{REFERENCE_TIME}

%include "exception.i"

%exception {
  try {
    $action
  } catch (const std::exception& e) {
    SWIG_exception(SWIG_RuntimeError, e.what());
  }
}

%include	"libwav.h"
