#ifndef FILE_H
#define FILE_H

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iostream>
#include "lib/CError.hpp"
#include <errno.h>


/**
 * \brief read the contents of a file or store contents to a file
 */
class CFile
{
public:
	CError error;	///< error handler

/**
 * read the contents to string
 */
bool fileContents(	const std::string &i_file_path,	///< filePath to source file
					std::string &o_data			///< data where to store the filecontents
)
{
	int length;
	char *data;

	FILE *file = fopen(i_file_path.c_str(), "rb");
	if (file == NULL)
	{
		std::ostringstream ss;
		ss << "fileContents(" << i_file_path << ", ...) fopen: " << strerror(errno);
		throw std::runtime_error(ss.str());
	}

	fseek(file, 0, SEEK_END);
	length = ftell(file);
	fseek(file, 0, SEEK_SET);

	data = new char[length];

	int readLength;
	readLength = fread(data, 1, length, file);
	fclose(file);

	if (readLength != length)
		throw std::runtime_error("readLength != fileLength");

	o_data.assign(data, readLength);

	delete [] data;
	return true;
}

/**
 * store contents to a file
 */
bool storeToFile(	const char *filePath,	///< path to file to store content to
					void *data,				///< pointer to dataset
					size_t length			///< length of dataset in bytes
)
{
	FILE *file = fopen(filePath, "w");
	if (file == NULL)
	{
		error << "fileContent(" << filePath << ") fopen: " << strerror(errno) << std::endl;
		return 0;
	}

	// TODO: error check
	size_t x = fwrite(data, length, 1, file);
	fclose(file);

	if (x < length)	return false;

	return true;
}


/**
 * load data from file
 * \param filePath	file to load
 * \param data		storage area
 * \param max_length	maximum length
 * \return		-1: error	else: loaded bytes
 */
int loadFromFile(const char *filePath, void *data, int max_length)
{
	int length;

	FILE *file = fopen(filePath, "w");
	if (file == NULL)
	{
		error << "fileContent(" << filePath << ") fopen: " << strerror(errno) << std::endl;
		return 0;
	}

	fseek(file, 0, SEEK_END);
	length = ftell(file);
	fseek(file, 0, SEEK_SET);

	if (max_length != -1)
		if (length > max_length-1)
		{
			error << "not enough space (filesize: " << (size_t)length << ")" << std::endl;
			return -1;
		}

	// TODO: error check
	int read_length = fread(data, length, 1, file);
	fclose(file);

	return read_length;
}


};

#endif
