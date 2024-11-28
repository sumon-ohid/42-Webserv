#!/usr/bin/env python

import os
import cgitb

# Enable debugging to see any potential CGI errors
cgitb.enable()

print("""
Content-Type: text/html\r\n
\r\n\r\n""")

# Begin HTML output
print("<html><body><h1>Environment Variables</h1>")
print("<pre>")

# Print each environment variable and its value
for key, value in os.environ.items():
    print(f"{key}: {value}")

print("</pre>")
print("</body></html>")
