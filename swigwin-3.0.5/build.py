import os
import sys
import inspect
path = os.path.dirname(os.path.abspath(inspect.getfile(inspect.currentframe())))

import configparser
config = configparser.ConfigParser()
config.read(path + os.sep + "config.ini")

swig_script = "swig %s %s -o %s %s" % \
              (\
                  config["input"]["-c++"], \
                  config["input"]["target_language"], \
                  config["output"]["target_wrapper_dir"] + os.sep + config["output"]["cpp_wrapper_name"], \
                  config["input"]["interface_file"], \
                  )
print(swig_script)

#win32
copy_script = "copy /v /y /b %s %s" % \
              (\
                  config["output"]["target_wrapper_dir"].replace("/", os.sep).replace("\\", os.sep) + os.sep + config["output"]["cpp_wrapper_name"], \
                  config["output"]["cpp_wrapper_dir"].replace("/", os.sep).replace("\\", os.sep), \
                  )
print(copy_script)


os.system(swig_script)
os.system(copy_script)

input("Finished. \n")

