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



#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <limits>

#include "CObjFile.hpp"
#include "libmath/CMath.hpp"
#include "lib/CError.hpp"


bool CObjFile::readData(	bool storeData,
							std::ifstream &filestream,
							const std::string &obj_filename
					)
{
	// clear possible EOF flag - otherwise we are not allowed to 'seekg'
	filestream.clear();
	filestream.seekg(0);

	bool material_file_loaded = false;

	// create default material
	CMaterial default_material;
	if (storeData)
	{
		default_material.name = "default material";
		materials.push_front(default_material);
	}

	// create default group
	CGroup default_group;
	if (storeData)
	{
		default_group.name = "default group";
		groups.push_front(default_group);
	}

	CGroup *current_group = NULL;

	if (storeData)
	{
		current_group = &groups.front();
	}

	// create default group mesh
	CGroupMaterialMesh mm;
	if (storeData)
	{
		current_group->meshes.push_front(mm);
	}

	CGroupMaterialMesh *current_material_mesh = NULL;
	if (storeData)
	{
		current_material_mesh = &current_group->meshes.front();
		current_material_mesh->material = &materials.front();
	}

	if (storeData)
	{
		current_group->faces3_vertices = faces3_vertices;
		current_group->faces3_texture_coords = faces3_texture_coords;
	}

	// dummy storage
	float vertex_dummy[3];
	float texture_coord_dummy[3];
	int face3_dummy[3];
	int face3_tex_dummy[3];
	int face4_dummy[4];
	int face4_tex_dummy[4];

	face3_tex_dummy[0] = 1;	// face ids start at 1
	face3_tex_dummy[1] = 1;
	face3_tex_dummy[2] = 1;

	face4_tex_dummy[0] = 1;
	face4_tex_dummy[1] = 1;
	face4_tex_dummy[2] = 1;
	face4_tex_dummy[3] = 1;

	int vertices_counter = 0;
	int texture_coords_counter = 0;
	int faces3_counter = 0;

	int line = 0;

	char buffer[1024];
	while(!filestream.getline(buffer, 1024).eof())
	{
		char first_word_buffer[1024];

		if (sscanf(buffer, "%1023s", first_word_buffer) <= 0)
		{
			// probably empty line => ignore
		}
		else
		{
			int first_word_buffer_length = strlen(first_word_buffer);
			if (first_word_buffer[0] == '#')
			{
				// commentline
			}
			else if (strncmp(first_word_buffer, "v", first_word_buffer_length) == 0)
			{
				// VERTICES
				if (sscanf(buffer, "v %f %f %f", &vertex_dummy[0], &vertex_dummy[1], &vertex_dummy[2]) != 3)
				{
					error << "OBJ: v - error in line " << line << ": " << buffer << CError::endl;
					return false;
				}

				if (storeData)
				{
					vertices[vertices_counter*3+0] = vertex_dummy[0];
					vertices[vertices_counter*3+1] = vertex_dummy[1];
					vertices[vertices_counter*3+2] = vertex_dummy[2];
					current_group->vertices_count++;
				}
				vertices_counter++;
			}
			else if (strncmp(first_word_buffer, "vt", first_word_buffer_length) == 0)
			{
				// VERTEX TEXTURE COORD
				if (sscanf(buffer, "vt %f %f", &texture_coord_dummy[0], &texture_coord_dummy[1]) != 2)
				{
					error << "OBJ: vt - error in line " << line << ": " << buffer << CError::endl;
					return false;
				}

				if (storeData)
				{
					texture_coords[texture_coords_counter*2+0] = texture_coord_dummy[0];
					texture_coords[texture_coords_counter*2+1] = 1.0-texture_coord_dummy[1];
					current_group->texture_coords_count++;
				}
				texture_coords_counter++;
			}
			else if (strncmp(first_word_buffer, "g", first_word_buffer_length) == 0)
			{
				// NEW GROUP
				if (storeData)
				{
					char group_name[1024];

					if (sscanf(buffer, "g %1023s", group_name) != 1)
					{
						error << "OBJ: g - error in line " << line << ": " << buffer << CError::endl;
						return false;
					}

					if (storeData)
					{
						message << "Group[" << group_name << "]" << CMessage::endl;

						CGroup new_group;
						new_group.name = group_name;
						new_group.faces3_vertices = &faces3_vertices[faces3_counter*3];
						new_group.faces3_texture_coords = &faces3_texture_coords[faces3_counter*3];

						groups.push_back(new_group);
						current_group = &groups.back();

						CGroupMaterialMesh mm;
						current_group->meshes.push_back(mm);

						// create new material mesh for current group
						current_material_mesh = &current_group->meshes.back();
						current_material_mesh->material = &materials.front();	// default material
					}
				}
			}
			else if (strncmp(first_word_buffer, "f", first_word_buffer_length) == 0)
			{
				// NEW FACE

				// count white spaces
				// TODO: handle more than 1 adjacent whitespaces and start position
				int whitespace_count;
				char *c = buffer + 2;
				for (whitespace_count = 0; c; c = strchr(c, ' '))
				{
					whitespace_count++;
					c++;
				}

				if (whitespace_count == 3)
				{
					if (sscanf(buffer, "f %i %i %i",	&face3_dummy[0], &face3_dummy[1], &face3_dummy[2]) != 3)
					if (sscanf(buffer, "f %i/%i %i/%i %i/%i",	&face3_dummy[0], &face3_tex_dummy[0], &face3_dummy[1], &face3_tex_dummy[1], &face3_dummy[2], &face3_tex_dummy[2]) != 6)
					if (sscanf(buffer, "f %i/%i/%*i %i/%i/%*i %i/%i/%*i",	&face3_dummy[0], &face3_tex_dummy[0], &face3_dummy[1], &face3_tex_dummy[1], &face3_dummy[2], &face3_tex_dummy[2]) != 6)
					{
						error << "OBJ: f - error in line " << line << ": " << buffer << CError::endl;
						return false;
					}

					face3_dummy[0]--;		face3_dummy[1]--;		face3_dummy[2]--;
					face3_tex_dummy[0]--;	face3_tex_dummy[1]--;	face3_tex_dummy[2]--;

					if (	face3_dummy[0] >= vertices_counter	||	face3_dummy[0] < 0	||
							face3_dummy[1] >= vertices_counter	||	face3_dummy[1] < 0	||
							face3_dummy[2] >= vertices_counter	||	face3_dummy[2] < 0
					)
					{
						error << "invalid face in line " << line << ": " << buffer << CError::endl;
						error << "max index: " << vertices_counter << "      face indices: " << face3_dummy[0] << ", " << face3_dummy[1] << ", " << face3_dummy[2] << CError::endl;
						return false;
					}

					if (storeData)
					{
						faces3_vertices[faces3_counter*3+0] = face3_dummy[0];
						faces3_vertices[faces3_counter*3+1] = face3_dummy[1];
						faces3_vertices[faces3_counter*3+2] = face3_dummy[2];

						if (faces3_texture_coords)
						{
							faces3_texture_coords[faces3_counter*3+0] = face3_tex_dummy[0];
							faces3_texture_coords[faces3_counter*3+1] = face3_tex_dummy[1];
							faces3_texture_coords[faces3_counter*3+2] = face3_tex_dummy[2];
						}


						face3_tex_dummy[0] = 1;		// face ids start at 1
						face3_tex_dummy[1] = 1;
						face3_tex_dummy[2] = 1;

						current_group->faces3_count++;
						current_material_mesh->faces3_count++;
					}

					faces3_counter++;
				}
				else if (whitespace_count == 4)
				{
					if (sscanf(buffer, "f %i %i %i %i",	&face4_dummy[0], &face4_dummy[1], &face4_dummy[2], &face4_dummy[3]) != 4)
					if (sscanf(buffer, "f %i/%i %i/%i %i/%i %i/%i",	&face4_dummy[0], &face4_tex_dummy[0], &face4_dummy[1], &face4_tex_dummy[1], &face4_dummy[2], &face4_tex_dummy[2], &face4_dummy[3], &face4_tex_dummy[3]) != 8)
					if (sscanf(buffer, "f %i/%i/%*i %i/%i/%*i %i/%i/%*i %i/%i/%*i",	&face4_dummy[0], &face4_tex_dummy[0], &face4_dummy[1], &face4_tex_dummy[1], &face4_dummy[2], &face4_tex_dummy[2], &face4_dummy[3], &face4_tex_dummy[3]) != 8)
					{
						error << "OBJ: f - error in line " << line << ": " << buffer << CError::endl;
						return false;
					}

					face4_dummy[0]--;		face4_dummy[1]--;		face4_dummy[2]--;		face4_dummy[3]--;
					face4_tex_dummy[0]--;	face4_tex_dummy[1]--;	face4_tex_dummy[2]--;	face4_tex_dummy[3]--;

					if (	face4_dummy[0] >= vertices_counter	||	face4_dummy[0] < 0	||
							face4_dummy[1] >= vertices_counter	||	face4_dummy[1] < 0	||
							face4_dummy[2] >= vertices_counter	||	face4_dummy[2] < 0	||
							face4_dummy[3] >= vertices_counter	||	face4_dummy[3] < 0
					)
					{
						error << "invalid face in line " << line << ": " << buffer << CError::endl;
						error << "max index: " << vertices_counter << "      face indices: " << face4_dummy[0] << ", " << face4_dummy[1] << ", " << face4_dummy[2] << ", " << face4_dummy[3] << CError::endl;
						return false;
					}

					if (storeData)
					{
						faces3_vertices[faces3_counter*3+0] = face4_dummy[0];
						faces3_vertices[faces3_counter*3+1] = face4_dummy[1];
						faces3_vertices[faces3_counter*3+2] = face4_dummy[3];

						faces3_vertices[faces3_counter*3+3] = face4_dummy[1];
						faces3_vertices[faces3_counter*3+4] = face4_dummy[2];
						faces3_vertices[faces3_counter*3+5] = face4_dummy[3];

						if (faces3_texture_coords)
						{
							faces3_texture_coords[faces3_counter*3+0] = face4_tex_dummy[0];
							faces3_texture_coords[faces3_counter*3+1] = face4_tex_dummy[1];
							faces3_texture_coords[faces3_counter*3+2] = face4_tex_dummy[3];

							faces3_texture_coords[faces3_counter*3+3] = face4_tex_dummy[1];
							faces3_texture_coords[faces3_counter*3+4] = face4_tex_dummy[2];
							faces3_texture_coords[faces3_counter*3+5] = face4_tex_dummy[3];
						}

						current_group->faces3_count += 2;
						current_material_mesh->faces3_count += 2;
					}

					face4_tex_dummy[0] = 1;	// faces ids start at 1
					face4_tex_dummy[1] = 1;
					face4_tex_dummy[2] = 1;
					face4_tex_dummy[3] = 1;

					faces3_counter += 2;
				}
				else
				{
					error << "OBJ: f - error in line " << line << ": " << buffer << CError::endl;
					return false;
				}
			}
			else if (strncmp(first_word_buffer, "mtllib", first_word_buffer_length) == 0)
			{
				// MATERIAL FILE

				if (storeData)
				{
					if (material_file_loaded)
					{
						error << "OBJ: material file already loaded!" << std::endl;
						return false;
					}

					material_file_loaded = true;

					char material_file_buf[1024];

					if (sscanf(buffer, "mtllib %1023s", material_file_buf) != 1)
					{
						error << "OBJ: mtllib - error in line " << line << ": " << buffer << CError::endl;
						return false;
					}

					// truncate filepath to objfile after '/' character and append material filename
					size_t pos = obj_filename.rfind('/');
					std::string material_file;
					if (pos == std::string::npos)
						material_file = "";
					else
						material_file = obj_filename.substr(0, pos+1);

					material_file += material_file_buf;

					loadMaterialFromFile(material_file);
				}
			}
			else if (strncmp(first_word_buffer, "usemtl", first_word_buffer_length) == 0)
			{
				if (storeData)
				{
					char material_name_buf[1024];

					if (sscanf(buffer, "usemtl %1023s", material_name_buf) != 1)
					{
						error << "OBJ: usemtl - error in line " << line << ": " << buffer << CError::endl;
						return false;
					}

					std::string material_name = material_name_buf;

					if (material_name == "(null)")
						continue;

					// search material
					std::list<CMaterial>::iterator i;
					for (i = materials.begin(); i != materials.end(); i++)
					{
						if ((*i).name == material_name)
							break;
					}

					if (i == materials.end())
					{
						error << "OBJ: material " << material_name << " not found!" << CError::endl;
						return false;
					}

					CGroupMaterialMesh mm;
					current_group->meshes.push_back(mm);
					current_material_mesh = &current_group->meshes.back();

					current_material_mesh->material = &*i;
				}
			}
			else if (strncmp(first_word_buffer, "s", first_word_buffer_length) == 0)
			{
				// ?!?
			}
			else if (strncmp(first_word_buffer, "o", first_word_buffer_length) == 0)
			{
				// OBJECT NAME
			}
			else
			{
				message << "ignoring unknown line: " << first_word_buffer << CMessage::endl;
			}
		}

		line++;
	}

	if (!storeData)
	{
		vertices_count = vertices_counter;
		faces3_count = faces3_counter;
		texture_coords_count = texture_coords_counter;

		message << "OBJ: Vertices: " << vertices_count << ", ";
		message << "Texture coords: " << texture_coords_count << ", ";
		message << "Groups coords: " << groups.size() << ", ";
		message << "Groups coords: " << materials.size() << ", ";
		message << "Face3: " << faces3_counter << CMessage::endl;
	}

	return true;
}

void CObjFile::init()
{
	vertices_count = 0;
	vertices = NULL;
	normals = NULL;

	texture_coords_count = 0;
	texture_coords = NULL;

	faces3_vertices = NULL;
	faces3_texture_coords = NULL;
}

/**
 * initialize empty obj file structure
 */
CObjFile::CObjFile()
{
	init();
}


/**
 * initialize with data from .obj file
 * \param objfile	path to .obj file
 */
CObjFile::CObjFile(const std::string &objfile)
{
	init();

	load(objfile);
}

void CObjFile::computeNormals()
{
	if (!faces3_vertices)
		return;

	// calculate normals
	float *n = normals;
	for (int i = 0; i < vertices_count; i++)
	{
		n[0] = 0.0f;
		n[1] = 0.0f;
		n[2] = 0.0f;
		n += 3;
	}

	GLSL::vec3 a, b, norm;
	int *f = faces3_vertices;

	for (int i = 0; i < faces3_count; i++)
	{
		float *v0 = &vertices[f[0]*3];
		float *v1 = &vertices[f[1]*3];
		float *v2 = &vertices[f[2]*3];

		a[0] = v1[0] - v0[0];
		a[1] = v1[1] - v0[1];
		a[2] = v1[2] - v0[2];

		b[0] = v2[0] - v0[0];
		b[1] = v2[1] - v0[1];
		b[2] = v2[2] - v0[2];

		norm[0] = a[1]*b[2] - a[2]*b[1];
		norm[1] = a[2]*b[0] - a[0]*b[2];
		norm[2] = a[0]*b[1] - a[1]*b[0];

		for (int j = 0; j < 3; j++)
		{
			normals[f[j]*3+0] += norm[0];
			normals[f[j]*3+1] += norm[1];
			normals[f[j]*3+2] += norm[2];
		}

		f += 3;
	}

	n = normals;
	for (int i = 0; i < vertices_count; i++)
	{
		float inv_length = 1.0f/(CMath::sqrt<float>(n[0]*n[0] + n[1]*n[1] + n[2]*n[2]));
		n[0] *= inv_length;
		n[1] *= inv_length;
		n[2] *= inv_length;
		n += 3;
	}
}

/**
 * convert datastructures for rendering without 'indexing'
 */
void CObjFile::linearizeVertexAndTextureCoordinates()
{
	for (std::list<CGroup>::iterator i = groups.begin(); i != groups.end(); i++)
	{
		CGroup &g = *i;

		g.cleanup();	/// deallocate existing arrays

		// the following pointers are deallocated automatically in the deconstructor
		// allocate buffer for vertices, normals and texture coordinates

		int buffer_elements = g.faces3_count*3*3 + g.faces3_count*3*3;

		g.lin_faces3 = g.faces3_count;

//		if (texture_coords)
			buffer_elements += faces3_count*3*2;

		g.lin_buffer_size = buffer_elements*sizeof(GLfloat);
		g.lin_buffer = new GLfloat[buffer_elements];

		g.lin_vertices = g.lin_buffer;
		g.rel_vertices = 0;

		g.lin_normals = g.lin_buffer + g.faces3_count*3*3;
		g.rel_normals = (GLfloat*)0 + g.faces3_count*3*3;

#if 0
		g.lin_texture_coords = g.lin_buffer + g.faces3_count*3*3*2;
		g.rel_texture_coords = (GLfloat*)0 + g.faces3_count*3*3*2;
#else
		if (texture_coords)
		{
			g.lin_texture_coords = g.lin_buffer + g.faces3_count*3*3*2;
			g.rel_texture_coords = (GLfloat*)0 + g.faces3_count*3*3*2;
		}
		else
		{
			g.lin_texture_coords = NULL;
			g.rel_texture_coords = NULL;
		}
#endif
		GLfloat *new_vertex = g.lin_buffer;
		GLfloat *new_normal = g.lin_normals;
		GLfloat *new_texture_coord = g.lin_texture_coords;

		GLint *face3_vertex = g.faces3_vertices;
		GLint *face3_texture_coord = g.faces3_texture_coords;


		for (int i = 0; i < g.faces3_count*3; i++)
		{
			new_vertex[0] = vertices[(*face3_vertex)*3+0];
			new_vertex[1] = vertices[(*face3_vertex)*3+1];
			new_vertex[2] = vertices[(*face3_vertex)*3+2];
			new_vertex += 3;

			new_normal[0] = normals[(*face3_vertex)*3+0];
			new_normal[1] = normals[(*face3_vertex)*3+1];
			new_normal[2] = normals[(*face3_vertex)*3+2];
			new_normal += 3;

			face3_vertex++;

			if (new_texture_coord != NULL)
			{
				if (*face3_texture_coord > texture_coords_count)
				{
					std::cerr << "INVALID texture coords: " << (*face3_texture_coord) << " >= " << texture_coords_count << std::endl;
					assert(false);
				}

				new_texture_coord[0] = texture_coords[(*face3_texture_coord)*2+0];
				new_texture_coord[1] = texture_coords[(*face3_texture_coord)*2+1];
				new_texture_coord += 2;

				face3_texture_coord++;
			}
		}

		// remove empty meshes
		for (std::list<CGroupMaterialMesh>::iterator i = g.meshes.begin(); i != g.meshes.end();)
		{
			if ((*i).faces3_count == 0)
			{
				// leave at least one mesh
				if (g.meshes.size() > 1)
				{
					// remote mesh
					i = g.meshes.erase(i);
					continue;
				}
			}
			i++;
		}

		int vertex_index = 0;
		for (std::list<CGroupMaterialMesh>::iterator i = g.meshes.begin(); i != g.meshes.end(); i++)
		{
			CGroupMaterialMesh &m = *i;
			m.rel_vertex_start = vertex_index;
			m.vertex_count = m.faces3_count*3;

			vertex_index += m.vertex_count;
		}
	}

	// free vertices, normals and faces3 datastructures
	cleanup();
}

bool CObjFile::loadMaterialFromFile(const std::string &material_file)
{

	// open stream
	std::ifstream filestream(material_file.c_str());

	if (!filestream.is_open())
	{
		error << "OBJ: unable to open material file " << material_file << CError::endl;
		return false;
	}

	// default material already exists (hopefully :-) )!
	CMaterial *current_material = &materials.front();


	int line = 0;
	while(1)
	{
		char buffer[1024];
		filestream.getline(buffer, 1024);
		if (filestream.eof())
			break;

		char first_word_buffer[1024];

		if (sscanf(buffer, "%1023s", first_word_buffer)  <= 0)
		{
			// probably empty line => ignore
		}
		else
		{
//			std::cout << line << " " << first_word_buffer << " _ " << buffer << std::endl;
			int first_word_buffer_length = strlen(first_word_buffer);
			if (first_word_buffer[0] == '#')
			{
				// COMMENT
			}
			else if (strncmp(first_word_buffer, "newmtl", first_word_buffer_length) == 0)
			{
				// Material name

				char b[1024];
				if (sscanf(buffer, "newmtl %1023s", b) != 1)
				{
					error << "MTL: newmtl - error in line " << line << ": " << buffer << CError::endl;
					return false;
				}

				message << "MTL: loading material " << b << CMessage::endl;
				CMaterial new_material;
				new_material.name = b;
				materials.push_back(new_material);
				current_material = &materials.back();
			}
			else if (strncmp(first_word_buffer, "Ka", first_word_buffer_length) <= 0)
			{
				// AMBIENT COLOR
				if (sscanf(buffer, "Ka %f %f %f", &current_material->ambient_color3[0], &current_material->ambient_color3[1], &current_material->ambient_color3[2]) != 3)
				{
					error << "MTL: Ka - error in line " << line << ": " << buffer << CError::endl;
					return false;
				}
			}
			else if (strncmp(first_word_buffer, "Kd", first_word_buffer_length) == 0)
			{
				// DIFFUSE COLOR
				if (sscanf(buffer, "Kd %f %f %f", &current_material->diffuse_color3[0], &current_material->diffuse_color3[1], &current_material->diffuse_color3[2]) != 3)
				{
					error << "MTL: Kd - error in line " << line << ": " << buffer << CError::endl;
					return false;
				}
			}
			else if (strncmp(first_word_buffer, "Ks", first_word_buffer_length) == 0)
			{
				// SPECULAR COLOR
				if (sscanf(buffer, "Ks %f %f %f", &current_material->specular_color3[0], &current_material->specular_color3[1], &current_material->specular_color3[2]) != 3)
				{
					error << "MTL: Ks - error in line " << line << ": " << buffer << CError::endl;
					return false;
				}
			}
			else if (strncmp(first_word_buffer, "Ns", first_word_buffer_length) == 0)
			{
				// SPECULAR EXPONENT
				if (sscanf(buffer, "Ns %f", &current_material->specular_exponent) != 1)
				{
					error << "MTL: Ns - error in line " << line << ": " << buffer << CError::endl;
					return false;
				}
			}
			else if (strncmp(first_word_buffer, "map_Kd", first_word_buffer_length) == 0)
			{
				char b[1024];
				// TEXTURE FILENAME
				if (sscanf(buffer, "map_Kd %1023s", b) != 1)
				{
					error << "MTL: map_Kd - error in line " << line << ": " << buffer << CError::endl;
					return false;
				}

				current_material->texture_file = b;
			}
			else if (strncmp(first_word_buffer, "illum", first_word_buffer_length) == 0)
			{
				// ?!?
			}
			else if (strncmp(first_word_buffer, "Ni", first_word_buffer_length) == 0)
			{
				// ?!?
			}
			else if (strncmp(first_word_buffer, "d", first_word_buffer_length) == 0)
			{
				// ?!?
			}
			else
			{
				message << "MTL: ignoring unknown line: " << first_word_buffer << CMessage::endl;
			}
		}

		line++;
	}

	return true;
}

/**
 * load data from .obj file
 */
void CObjFile::load(	const std::string &objfile,		///< path to .obj file
						bool centerAndResizeFlag				///< normalize object to fit into [-1;1]^3 cube
					)
{
	cleanup();
	groups.clear();
	materials.clear();

	message << "OBJ: loading " << objfile << ":" << CMessage::endl;


	std::ifstream filestream(objfile.c_str());

	if (!filestream.is_open())
	{
		error << "OBJ: unable to open file " << objfile << CError::endl;
		return;
	}

	if (!readData(false, filestream, ""))
		return;

	vertices = new float[vertices_count*3];
	normals = new float[vertices_count*3];
	faces3_vertices = new int[faces3_count*3];

	if (texture_coords_count > 0)
	{
		texture_coords = new float[texture_coords_count*2];
		faces3_texture_coords = new int[faces3_count*3];

		std::cout << "allocated " << texture_coords_count*2 << " texture coordinates" << std::endl;
	}


	if (!readData(true, filestream, objfile))
	{
		cleanup();

		groups.clear();
		materials.clear();
		return;
	}
	filestream.close();

	computeNormals();

	if (centerAndResizeFlag)
		centerAndResize();

	linearizeVertexAndTextureCoordinates();
}


/**
 * center and resize to fit to a -[1;1]^3 domain
 */
void CObjFile::centerAndResize()
{
	if (!faces3_vertices)
		return;

	float inf = std::numeric_limits<float>::infinity();
	float min[3] = {inf, inf, inf};
	float max[3] = {-inf, -inf, -inf};

	float *v = vertices;
	for (int i = 0; i < vertices_count; i++)
	{
		for (int j = 0; j < 3; j++)
		{
			if (min[j] > v[j])
				min[j] = v[j];
			else if (max[j] < v[j])
				max[j] = v[j];
		}
		v+=3;
	}

	float dist[3];
	dist[0] = (max[0] - min[0]);
	dist[1] = (max[1] - min[1]);
	dist[2] = (max[2] - min[2]);

	float scale = 1.0f;
	if (dist[0] != 0 && dist[1] != 0 && dist[2] != 0)
	{
		scale = (dist[0] > dist[1] ? dist[0] : dist[1]);
		scale = (scale > dist[2] ? scale : dist[2]);
	}

	scale = 2.0f/scale;

	float center[3];
	center[0] = (min[0] + max[0])*0.5f;
	center[1] = (min[1] + max[1])*0.5f;
	center[2] = (min[2] + max[2])*0.5f;

	v = vertices;
	for (int i = 0; i < vertices_count; i++)
	{
		for (int j = 0; j < 3; j++)
			v[j] = (v[j] - center[j])*scale;
		v+=3;
	}
}

void CObjFile::cleanup()
{
	if (vertices != NULL)
	{
		delete [] vertices;
		vertices = NULL;
	}

	if (normals != NULL)
	{
		delete [] normals;
		normals = NULL;
	}

	if (faces3_vertices)
	{
		delete [] faces3_vertices;
		faces3_vertices = NULL;
	}

	if (faces3_texture_coords)
	{
		delete [] faces3_texture_coords;
		faces3_texture_coords = NULL;
	}

	if (texture_coords)
	{
		delete [] texture_coords;
		texture_coords = NULL;
	}
}


/**
 * cleanup all allocated
 */
CObjFile::~CObjFile()
{
	cleanup();
}


/**
 * cleanup all allocated
 */
void CObjFile::debug()
{
	for (std::list<CGroup>::iterator gi = groups.begin(); gi != groups.end(); gi++)
	{
		CGroup &g = *gi;
		std::cout << "----------------------" << std::endl;
		std::cout << "Group: " << g.name << std::endl;
		std::cout << "----------------------" << std::endl;

		for (std::list<CGroupMaterialMesh>::iterator mi = g.meshes.begin(); mi != g.meshes.end(); mi++)
		{
			CGroupMaterialMesh &m = *mi;
			std::cout << " M: " << m.material->name << std::endl;
			for (int i = m.rel_vertex_start; i < m.rel_vertex_start+m.vertex_count; i++)
			{
				std::cout << "  V [" << g.lin_vertices[i*3+0] << "," << g.lin_vertices[i*3+1] << "," << g.lin_vertices[i*3+2] << "]";
				std::cout << "  N [" << g.lin_normals[i*3+0] << "," << g.lin_normals[i*3+1] << "," << g.lin_normals[i*3+2] << "]";
				std::cout << "  T [" << g.lin_texture_coords[i*2+0] << "," << g.lin_texture_coords[i*2+1] << "]" << std::endl;
			}
		}
		std::cout << "----------------------" << std::endl;
	}
}

