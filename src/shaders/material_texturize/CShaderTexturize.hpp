#ifndef CGL_SHADER_TEXTURIZE_HPP
#define CGL_SHADER_TEXTURIZE_HPP

#include "libgl/core/CGlTexture.hpp"
#include "libgl/core/CGlError.hpp"
#include "lib/CError.hpp"


class CShaderTexturize	: public CGlProgram
{
public:
	CGlUniform pvm_matrix_uniform;
//	CGlUniform normal_matrix_uniform;

	CShaderTexturize()
	{
		initVertFragShadersFromDirectory("material_texturize");
		if (error())
			return;

		link();
		if (error())
		{
			std::string infoLog;
			getInfoLog(infoLog);
			error << "info Log: linking: " << infoLog << std::endl;
			return;
		}

		this->setupUniform(pvm_matrix_uniform, "pvm_matrix");
//		normal_matrix_uniform.init(program, "normal_matrix");
	}

	~CShaderTexturize()
	{
	}
};


#endif
