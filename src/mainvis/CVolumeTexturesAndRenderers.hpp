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

/*
 * CVolumeRenderers.hpp
 *
 *  Created on: Mar 22, 2010
 *      Author: martin
 */

#ifndef CVOLUME_TEXTURES_AND_RENDERERS_HPP_
#define CVOLUME_TEXTURES_AND_RENDERERS_HPP_



#include "libgl/volume_renderer/CGlVolumeRendererSimple.hpp"
#include "libgl/volume_renderer/CGlVolumeRendererCubeSteps.hpp"
#include "libgl/volume_renderer/CGlVolumeRendererInterpolated.hpp"
#include "libgl/volume_renderer/CGlVolumeRendererSimpleRefraction.hpp"
#include "libgl/volume_renderer/CGlVolumeRendererCubeStepsRefraction.hpp"
#include "libgl/volume_renderer/CGlVolumeRendererInterpolatedRefraction.hpp"
#include "libgl/volume_renderer/CGlVolumeRendererMarchingCubes.hpp"
#include "libgl/volume_renderer/CGlVolumeRendererMarchingCubesRefraction.hpp"

/**
 * \brief	class to handle differnt test texture (with a sphere) for volume rendering
 */
class CVolumeTexturesAndRenderers
{
	bool verbose;

public:
	CError error;	///< error handler

	CGlTexture test_volume_texture;	///< test volume texture

	CGlVolumeRendererSimple volume_renderer_simple;		///< simple volume renderer
	CGlVolumeRendererSimpleRefraction volume_renderer_simple_refraction;		///< simple volume renderer with refractions

	CGlVolumeRendererInterpolated volume_renderer_interpolated;	///< volume renderer with interpolation of last iteration
	CGlVolumeRendererInterpolatedRefraction volume_renderer_interpolated_refraction;	///< volume renderer with interpolation of last iteration and refractions

	CGlVolumeRendererCubeSteps volume_renderer_cube_steps;		///< cube step volume renderer
	CGlVolumeRendererCubeStepsRefraction volume_renderer_cube_steps_refraction;		///< cube step volume renderer with refractions

	CGlVolumeRendererMarchingCubes volume_renderer_marching_cubes;	///< marching cubes volume renderer
	CGlVolumeRendererMarchingCubesRefraction volume_renderer_marching_cubes_refraction;	///< marching cubes volume renderer with refractions


	/**
	 * initialize volumetric test dataset
	 */
	void initVolumeData(	CGlTexture &test_volume_texture,
							CVector<3,int> &domain_size,
							GLfloat radius
	)
	{
		/**
		 * INITIALIZE VOLUME STUFF
		 */
		int half_x = (float)domain_size[0] / 2.0f;
		int half_y = (float)domain_size[1] / 2.0f;
		int half_z = (float)domain_size[2] / 2.0f;

		float *test_data = new float[domain_size.elements()];

		float *dptr = test_data;
		for (int z = 0; z < domain_size[2]; z++)
		{
			for (int y = 0; y < domain_size[1]; y++)
			{
				for (int x = 0; x < domain_size[0]; x++)
				{
					float posx = (float)x;
					float posy = (float)y;
					float posz = (float)z;

					float dx = posx - half_x + 0.5f;
					float dy = posy - half_y + 0.5f;
					float dz = posz - half_z + 0.5f;

					float dist = CMath::sqrt(dx*dx + dy*dy + dz*dz);

					float value = radius - dist;
					value *= 0.5;
					if (value < 0.0)	value = 0.0;
					if (value > 1.0)	value = 1.0;
					*dptr = value;

					dptr++;
				}
			}
		}

		test_volume_texture.bind();
		test_volume_texture.resize(domain_size[0], domain_size[1], domain_size[2]);

		test_volume_texture.setData(test_data);
		glFinish();

		delete[] test_data;

		test_volume_texture.unbind();
	}


	/**
	 * initialize texture with test dataset
	 */
	void initTexture(	CGlTexture &test_volume_texture,	///< texture to initialize
						CVector<3,int> &domain_size			///< new size of volume texture
	)
	{
		initVolumeData(test_volume_texture, domain_size, domain_size.min()/2-3);

		test_volume_texture.bind();
		test_volume_texture.setParam(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		test_volume_texture.setParam(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		test_volume_texture.setParam(GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		test_volume_texture.setParam(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		test_volume_texture.setParam(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		test_volume_texture.unbind();

		CError_Append(test_volume_texture);
	}

	/**
	 * setup volume textures and renderers
	 *
	 * \param	create_volumes	set to true, if 3d volume textures should be created e.g. if no data is created by the fluid simulation
	 * \param	domain_size		domain size to initialize 3d volume texture
	 */
	void setup(	bool create_volumes,
				CVector<3,int> &domain_size)
	{
		if (create_volumes)
			initTexture(test_volume_texture, domain_size);

		CError_Append(volume_renderer_simple);
		CError_Append(volume_renderer_cube_steps);
		CError_Append(volume_renderer_interpolated);
		CError_Append(volume_renderer_marching_cubes);

		CError_Append(volume_renderer_simple_refraction);
		CError_Append(volume_renderer_cube_steps_refraction);
		CError_Append(volume_renderer_interpolated_refraction);
		CError_Append(volume_renderer_marching_cubes_refraction);
	}



	CVolumeTexturesAndRenderers(bool p_verbose)	:
		verbose(p_verbose),
		test_volume_texture(GL_TEXTURE_3D, GL_R32F, GL_RED, GL_FLOAT)
	{
	}
};



#endif /* CVOLUMERENDERERS_HPP_ */
