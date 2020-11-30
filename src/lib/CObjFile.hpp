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




#ifndef C_OBJFILE_HPP
#define C_OBJFILE_HPP

#include "libgl/incgl3.h"

#include <stdio.h>
#include "libmath/CMath.hpp"
#include "libmath/CGlSlMath.hpp"
#include "lib/CError.hpp"

#include <string>
#include <iostream>
#include <fstream>
#include <list>


/**
 * \brief	load mesh data from .obj file
 *
 * WARNING: plenty of checks for 'out of array' accesses are necessary to make this class safe.
 * dont use it for productive systems!!!
 */
class CObjFile
{
public:
	CError error;		///< error handler
	CMessage message;	///< message handler

	/**
	 * \brief	structure to handle material parameters
	 */
	class CMaterial {
		public:
			std::string name;			///< name of material
			GLSL::vec3 ambient_color3;	///< ambient color
			GLSL::vec3 diffuse_color3;	///< diffuse color
			float specular_exponent;	///< specular exponent
			GLSL::vec3 specular_color3;	///< specular color
			std::string texture_file;		///< texture name

			CMaterial()
			{
				ambient_color3		= GLSL::vec3(0.1, 0.1, 0.1);
				diffuse_color3		= GLSL::vec3(0.6, 0.6, 0.6);
				specular_color3		= GLSL::vec3(1.0, 1.0, 1.0);
				specular_exponent	= 20.0;
			}
	};


	/**
	 * \brief	meshes within groups and a specific assigned materials.
	 */
	class CGroupMaterialMesh
	{
	public:
		CMaterial *material;	///< assigned material

		int faces3_count;		///< number of faces3 associated with the mesh

		int rel_vertex_start;		///< start position within linearized vertex data
		int vertex_count;		///< number of vertices for the current mesh

		CGroupMaterialMesh()
		{
			material = NULL;

			rel_vertex_start = 0;	///< linear start vertex within vertex array of current group
			vertex_count = 0;	///< linear vertex counter for vertex array in current group

			faces3_count = 0;
		}
	};

	/**
	 * \brief	structure to access groups within loaded .obj file
	 *
	 * each group has it's own
	 */
	class CGroup
	{
	public:
		std::string name;			///< name of group

		int faces3_count;				///< number of faces - the faces are only important for preprocessing
		GLint *faces3_vertices;			///< pointer to faces array: 1 surface has 3 index values to vertex points
		GLint *faces3_texture_coords;	///< pointer to texture array with coordinates: 2 values per vertex point

		size_t lin_buffer_size;			///< sizeof buffer (in bytes)

		GLint lin_faces3;				///< number of vertices to draw
		GLfloat *lin_buffer;			///< pointer to buffer to store vertices, normals and texture coordinates

		GLfloat *lin_vertices;			///< pointer to vertices for current group after linearization
		GLint vertices_count;			///< number of vertices within this group
		GLfloat *lin_normals;			///< pointer to normals for current group after linearization
		GLint texture_coords_count;	///< number of texture coordinates within this group
		GLfloat *lin_texture_coords;	///< pointer to texture coords for current group after linearization

		GLfloat *rel_vertices;			///< relative pointer to vertices for current group after linearization (useful for opengl)
		GLfloat *rel_normals;			///< relative pointer to normals for current group after linearization (useful for opengl)
		GLfloat *rel_texture_coords;	///< relative pointer to texture coords for current group after linearization (useful for opengl)

		std::list<CGroupMaterialMesh> meshes;	///< Mesh within a group and assigned materials

		CGroup()
		{
			faces3_count = 0;
			faces3_vertices = NULL;
			faces3_texture_coords = NULL;

			lin_buffer_size = 0;

			lin_faces3 = 0;
			lin_buffer = NULL;

			lin_vertices = NULL;
			vertices_count = 0;
			lin_normals = NULL;

			texture_coords_count = 0;
			lin_texture_coords = NULL;

			rel_vertices = NULL;
			rel_normals = NULL;
			rel_texture_coords = NULL;
		}

		/**
		 * free unnecessary data
		 *
		 * faces3_*, lin_* and rel_* data are just pointers to arrays handled by other classes
		 */
		void cleanup()
		{
			if (lin_buffer)
			{
				delete [] lin_buffer;
				lin_buffer = NULL;
			}
		}

		~CGroup()
		{
			cleanup();
		}
	};


private:

	bool readData(	bool storeData,
					std::ifstream &filestream,
					const std::string &obj_filename
				);

	void init();
	void cleanup();			///< free data

public:
	int vertices_count;			///< number of vertex points
	GLfloat *vertices;			///< pointer vertex array
	GLfloat *normals;			///< pointer to normal array

	int texture_coords_count;	///< number of texture coordinates - this value is either 0 or equal to vertex_count
	GLfloat *texture_coords;	///< pointer to array of texture coordinates

	void centerAndResize();

	std::list<CGroup> groups;			///< list with groups
	std::list<CMaterial> materials;		///< list with materials

private:
	int faces3_count;			///< number of faces with 3 vertices
	GLint *faces3_vertices;	///< faces with 3 vertices
	GLint *faces3_texture_coords;		///< faces with 3 vertices

	void linearizeVertexAndTextureCoordinates();
	void computeNormals();
	bool loadMaterialFromFile(const std::string &material_file);

public:
	void load(	const std::string &objfile,		///< path to .obj file
				bool centerAndResizeFlag = true		///< normalize object to fit into [-1;1]^3 cube
			);
	CObjFile(const std::string &objfile);

	CObjFile();
	~CObjFile();

	void debug();
};

#endif
