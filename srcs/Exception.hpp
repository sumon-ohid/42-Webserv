#pragma once

#include <exception>

#define READ_ERROR	"read error"

class	ReadError : public std::exception
{
public:
	const char* what() const throw();
};
