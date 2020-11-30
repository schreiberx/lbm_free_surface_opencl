#ifndef CGL_SHADER_CUBE_MAP_HPP
#define CGL_SHADER_CUBE_MAP_HPP

#include "libgl/core/CGlTexture.hpp"
#include "libgl/core/CGlError.hpp"
#include "lib/CError.hpp"


class CShaderCubeMapMirror	: public CGlProgram
{
public:
	CGlUniform pvm_matrix_uniform;
	CGlUniform normal_matrix3_uniform;
	CGlUniform view_model_matrix_uniform;
	CGlUniform view_matrix_uniform;
	CGlUniform transp_view_normal_matrix3_uniform;

	CShaderCubeMapMirror()
	{
		initVertFragShadersFromDirectory("material_cube_map_mirror");
		if (error())
		{
			error << error.getString() << std::endl;
			return;
		}

		link();
		if (error())
		{
			std::string infoLog;
			getInfoLog(infoLog);
			error << "info Log: linking: " << infoLog << std::endl;
			return;
		}

		this->setupUniform(pvm_matrix_uniform, "pvm_matrix");
		this->setupUniform(normal_matrix3_uniform, "normal_matrix3");
		this->setupUniform(view_model_matrix_uniform, "view_model_matrix");
		this->setupUniform(view_matrix_uniform, "view_matrix");
		this->setupUniform(transp_view_normal_matrix3_uniform, "transp_view_normal_matrix3");
	}

	~CShaderCubeMapMirror()
	{
	}
};


#endif
