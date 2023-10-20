#!/usr/bin/python
import sys
sys.dont_write_bytecode = True
import os, subprocess

def ok_console(message):
    print("\033[92m" + str(message) + "\033[0m")

def err_console(message):
    print("\033[91m" + str(message) + "\033[0m")


if __name__ == '__main__':
    print("Compiling shader")
    script_directory = os.path.dirname(os.path.realpath(__file__))
    os.chdir(script_directory)
    if not os.path.exists("bin/"):
        os.mkdir("bin/")
    dirs = [d for d in os.listdir(".") if not d.endswith('.glsl') and not d.endswith('.py') and os.path.isfile(d)]
    for d in dirs:
        try:
            print('"{}"'.format(d))
            subprocess.run("glslc --target-env=vulkan1.2 -O -o bin/{0}.spv {0}".format(d), shell=True, check=True)
        except subprocess.CalledProcessError as e:
            err_console("ERROR")
            exit(1)
    ok_console("Shader compiled!")
