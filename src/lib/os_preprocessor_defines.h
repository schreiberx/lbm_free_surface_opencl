/*
 * Copyright 2010 Martin Schreiber
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */




#ifndef OS_PREPROCESSOR_DEFINES_HPP_
#define OS_PREPROCESSOR_DEFINES_HPP_

#define OS_APPLE	0
#define OS_UNIX		0
#define OS_LINUX	0
#define OS_WINDOWS	0

#if __APPLE__
	#undef OS_APPLE
	#define OS_APPLE	1
	#undef OS_UNIX
	#define OS_UNIX		1
#endif

#if __unix
	#undef OS_UNIX
	#define OS_UNIX		1
#endif

#if __linux
	#undef OS_LINUX
	#define OS_LINUX		1
	#undef OS_UNIX
	#define OS_UNIX		1
#endif

// TODO: add more options for that !(*@(#)!*@$ OS
#if WIN32
	#undef OS_WINDOWS
	#define OS_WINDOWS	1
#endif

#endif /* OS_PREPROCESSOR_DEFINES_HPP_ */
