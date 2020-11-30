#include "CShaderColor.hpp"


/**
 * if this file is included by multiple cpp files which are compiled, the following lines have to be moved to a separate cpp file
 * to be linked only once! otherwise a compiler error would be generated
 */
// no shaders are initialized so far
CGlShader CShaderColor::vertShader;
CGlShader CShaderColor::fragShader;
bool CShaderColor::shaders_loaded = false;
int CShaderColor::usage_counter = 0;
