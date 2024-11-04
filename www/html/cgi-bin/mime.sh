#!/bin/bash

echo -e 'HTTP/1.1 200 OK\n'
echo -e 'Content-Type: video/mp4\r\n'
echo -e '\r\n\r\n'
cat /home/msumon/webserv/www/html/cgi-bin/mime.mp4
