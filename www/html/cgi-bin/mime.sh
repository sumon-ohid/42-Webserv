#!/bin/bash

echo -e 'HTTP/1.1 200 OK\n'
echo -e 'Content-Type: image/jpeg\r\n'
echo -e '\r\n\r\n'
cat www/html/cgi-bin/mime.png