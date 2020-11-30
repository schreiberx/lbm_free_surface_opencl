
# License

Copyright 2010 Martin Schreiber

Licensed under the Apache License, Version 2.0 (the "License");
you may not use this file except in compliance with the License.
You may obtain a copy of the License at

http://www.apache.org/licenses/LICENSE-2.0

Unless required by applicable law or agreed to in writing, software
distributed under the License is distributed on an "AS IS" BASIS,
WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
See the License for the specific language governing permissions and
limitations under the License.



# Installation instructions


required graphic card: NVIDIA with development drivers installed

required packages:
	libgl1-mesa-dev
	libx11-dev


latest SDL2.0 version necessary:
	hg clone http://hg.libsdl.org/SDL

latest SDL2_image version necessary:
	hg clone http://hg.libsdl.org/SDL_image
	configure SDL2_image with '--enable-jpg --disable-jpg-shared'



scons has to be used when compiling lbm_opencl_fs.


EXECUTION:
	e.g.
	./build/lbm_opencl_fs_gnu_debug  -c -n -g -X 32
	to start with volume resolution of 32x32x32.
	More options are printed to the console with the '-h' program parameter.


for balanceboard:
	git clone git://github.com/abstrakraft/cwiid.git
