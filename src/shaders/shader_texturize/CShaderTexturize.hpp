#ifndef CGL_SHADER_TEureXTURIZE_HPP
#define CGL_SHADER_TEXTURIZE_HPP

#include "libgl/core/CGlTexture.hpp"
#include "libgl/core/CGlError.hpp"
#include "lib/CError.hpp"


class CShaderTexturize	: public CGlProgram
{
public:
	CGlUniform pvm_matrix_uniform;

	CShaderTexturize()
	{
		initVertFragShadersFromDirectory("shader_texturize");

		link();

		this->setupUniform(pvm_matrix_uniform, "pvm_matrix");
	}

	~CShaderTexturize()
	{
	}
};


#endif
