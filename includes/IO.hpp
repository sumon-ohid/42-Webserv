#pragma once

#include <cstddef>
#include <string>
#include <sys/types.h>
#include <vector>

class Client;

#define		IO_SIZE		64000

class IO
{
private:
	int					_fd;
	size_t				_size;
	ssize_t				_byteTracker;
	size_t				_totalBytesSent;
	std::vector<char>	_response;
	std::string			_responseStr;
	bool				_mimeCheckDone;
public:
	IO();
	IO(const IO&);
	IO&	operator=(const IO&);
	~IO();

	// writing
	void	writeToChildFd(Client*);
	void	writeToFd(Client*);
	void	finishWrite(Client*);
	void	finishWriteCgi(Client*);

	// reading
	void	readFromChildFd(Client*);
	void	readFromFile(Client*);
	void	readFromFd();
	void	MimeTypeCheck(Client*);
	void	extractMimeType(size_t, std::string&, Client*);
	void	finishReadingFromFd(Client*);

	// general
	void	checkReadOrWriteError(Client*);
	void	resetIO(Client*);


	void	setFd(int);
	void	setSize(size_t);

	int		getFd() const;
	size_t	getSize() const;
};