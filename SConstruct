#! /usr/bin/python

import os
import subprocess
import re
import sys


def exec_command(command):
    process = subprocess.Popen(command.split(' '), stdout=subprocess.PIPE, stderr=subprocess.PIPE)
    out, err = process.communicate()
    # combine stdout and stderr
    out = out+err
    out = out.decode("utf-8")
    out = out.replace("\r", "")
    return out


###################################################################
# configuration ends here - don't modify any line below this one
###################################################################

env = Environment()
env.Append(ENV=os.environ)

###################################################################
# Command line options
###################################################################

#
# build directory
#
AddOption(  '--buildname',
	        dest='buildname',
	        type='string',
	        nargs=1,
	        action='store',
	        help='build name (and output directory), default: \'./build\'')

env['buildname'] = GetOption('buildname')
if (env['buildname'] == None):
    env['buildname'] = 'build'



#
# compiler (gnu/intel)
#
AddOption(  '--compiler',
	        dest='compiler',
	        type='string',
	        nargs=1,
	        action='store',
	        help='specify compiler to use (gnu/intel), default: gnu')

env['compiler'] = GetOption('compiler')

if (env['compiler'] == None or (env['compiler'] not in ['gnu', 'intel', 'open64'])):
    env['compiler'] = 'gnu'




#
# balanceboard
#
AddOption(  '--balanceboard',
	        dest='balanceboard',
	        type='string',
	        nargs=1,
	        action='store',
	        help='activate blance board, default: false')

env['balanceboard'] = GetOption('balanceboard')

if (env['balanceboard'] == None or (env['balanceboard'] not in ['true', 'false'])):
    env['balanceboard'] = 'false'




#
# compile mode (debug/release)
#
AddOption(  '--mode',
	        dest='mode',
	        type='string',
	        nargs=1,
	        action='store',
	        help='specify release or debug mode (release/debug), default: release')

env['mode'] = GetOption('mode')

if (env['mode'] == None or (env['mode'] not in ['release', 'debug'])):
    env['mode'] = 'release'


######################
# INCLUDE PATH
######################

# ugly hack
for i in ['src/', 'src/include/']:
    env.Append(CXXFLAGS = ' -I'+os.environ['PWD']+'/'+i)


###################################################################
# SETUP COMPILER AND LINK OPTIONS
###################################################################


# add nvidia lib path when running on atsccs* workstation
hostname = exec_command('uname -n')
#if re.match("atsccs.*", hostname):
if True:
    env.Append(LIBPATH=['/usr/lib/nvidia-current/'])
    env.Append(LIBPATH=['/opt/intel/opencl/'])
    env.Append(CXXFLAGS = ' -I/usr/include/nvidia-current')


#env.Append(LIBPATH=[os.environ['HOME']+'/local/lib'])
env.Append(LIBS=['GL'])
env.Append(LIBS=['OpenCL'])

#
# SDL
#
reqversion = [2,0,0]
sdlversion = exec_command('sdl2-config --version').split('.')

for i in range(0, 3):
    if int(sdlversion[i]) >= int(reqversion[i]):
        break

    if int(sdlversion[i]) < int(reqversion[i]):
        print('libSDL Version 2.0.0 necessary.')
        Exit(1)

env.ParseConfig("sdl2-config --cflags --libs")
env.ParseConfig("pkg-config SDL2_image --cflags --libs")
env.ParseConfig("pkg-config freetype2 --cflags --libs")

#
# FREETYPE2
#
env.ParseConfig("pkg-config freetype2 --cflags --libs")

#
# SDL IMAGE
#
env.Append(LIBS=['SDL2_image'])

#
# xml
#
env.ParseConfig("pkg-config libxml-2.0 --cflags --libs")

if env['compiler'] == 'gnu':
	#env.Append(LINKFLAGS=' -static-libgcc')

    # eclipse specific flag
    env.Append(CXXFLAGS=' -fmessage-length=0')

    # XXX
	#env.Append(CXXFLAGS=' -std=c++11')

    # be pedantic to avoid stupid programming errors
    env.Append(CXXFLAGS=' -pedantic')



elif env['compiler'] == 'intel':
    # eclipse specific flag
    env.Append(CXXFLAGS=' -fmessage-length=0')

    # activate intel C++ compiler
    env.Replace(CXX = 'icpc')

#    env.Append(CXXFLAGS=' -std=c++11')


if env['mode'] == 'debug':
    env.Append(CXXFLAGS=' -DLBM_OPENCL_FS_DEBUG=1')

    if env['compiler'] == 'gnu':
        env.Append(CXXFLAGS=' -O0 -g3 -Wall')

    elif env['compiler'] == 'intel':
        env.Append(CXXFLAGS=' -O0 -g3')
#        env.Append(CXXFLAGS=' -traceback')

elif env['mode'] == 'release':
    env.Append(CXXFLAGS=' -DLBM_OPENCL_FS_DEBUG=0')

    env.Append(CXXFLAGS=' -DNDEBUG=1')

    if env['compiler'] == 'gnu':
        env.Append(CXXFLAGS=' -O3 -g -mtune=native')

    elif env['compiler'] == 'intel':
        env.Append(CXXFLAGS=' -xHOST -O3 -g -fast -fno-alias')

else:
    print('ERROR: please select mode (--mode=release)')
    Exit(1)



###################################################################
# DEPENDENCIES
###################################################################

# also include the 'src' directory to search for dependencies
env.Append(CPPPATH = ['.', 'src/'])


######################
# setup PROGRAM NAME base on parameters
######################
program_name = 'lbm_opencl_fs'

# compiler
program_name += '_'+env['compiler']

# mode
program_name += '_'+env['mode']

# balance board
if env['balanceboard'] == 'true':
    program_name += '_balanceboard'
    env.Append(CXXFLAGS = ' -I'+os.environ['HOME']+'/local/include')
    env.Append(LIBS=['cwiid', 'bluetooth'])

######################
# get source code files
######################

env.src_files = []

Export('env')
SConscript('src/SConscript', variant_dir='build/build_'+program_name, duplicate=0)
Import('env')

print("")
print('Building program "'+program_name+'"')
print

env.Program('build/'+program_name, env.src_files)

#Exit(0)
