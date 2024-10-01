#include "Exception.hpp"

const char* ReadError::what() const throw()
{
	return READ_ERROR;
};
