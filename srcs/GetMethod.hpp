#pragma once

class Request;

#include "Method.hpp"

class GetMethod : public Method {
	private:
		// Add member variables here

	public:
		GetMethod();
		GetMethod(const GetMethod& other);
		GetMethod& operator=(const GetMethod& other);
		~GetMethod();

		void	executeMethod(int socketFd, Request& request);

};
