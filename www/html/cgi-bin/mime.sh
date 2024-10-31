#!/bin/bash

echo -e 'HTTP/1.1 200 OK\n'
echo -e 'Content-Type: image/png\r\n'
echo -e '\r\n\r\n'
cat /project/workspace/www/html/cgi-bin/mime2.png
