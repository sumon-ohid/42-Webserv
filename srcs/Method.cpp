#include "Method.hpp"

const std::string Method::_mArray[ARRAY_SIZE] = {"OPTIONS", "HEAD", "GET", "POST"};

Method::Method() {}

Method::Method(const std::string name) {

	for (int i = 0; i < ARRAY_SIZE; i++) {
		if (_mArray[i] == name) {
			this->_name = name;
		}
	}
}

Method::Method(const Method& other) {}

Method& Method::operator=(const Method& other) {
	if (this == &other)
		return *this;
	// copy variables here
	return *this;
}

Method::~Method() {}
