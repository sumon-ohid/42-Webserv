#pragma once

class Request;

#include "Method.hpp"

class GetHeadMethod : public Method {
	private:
		// Add member variables here

	public:
		GetHeadMethod();
		GetHeadMethod(const GetHeadMethod& other);
		GetHeadMethod& operator=(const GetHeadMethod& other);
		~GetHeadMethod();

		void	executeMethod(int socketFd, Request& request) const;

};
