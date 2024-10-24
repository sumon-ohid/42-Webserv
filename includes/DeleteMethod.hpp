#pragma once
#include "Method.hpp"

class DeleteMethod : public Method
{
private:
	std::string	_pathToDelete;
	
public:
	DeleteMethod(void);
	DeleteMethod(const DeleteMethod& other);
	DeleteMethod& operator=(const DeleteMethod& other);
	~DeleteMethod(void);

	void	executeMethod(int socketFd, Client *client, Request &request);
	void	deleteObject(void);
	void	checkStatError(void);

	Method*	clone();
};