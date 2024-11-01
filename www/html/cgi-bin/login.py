#!/usr/bin/env python

import cgi
import cgitb
import sqlite3
import os
import sys
from urllib.parse import parse_qs

# Enable debugging
cgitb.enable()

# Read the input data from stdin
content_length = int(os.environ.get('CONTENT_LENGTH', 0))
request_body = sys.stdin.read(content_length)

# Parse the input data
form = parse_qs(request_body)
login = form.get("login", [""])[0].replace("%20", " ")
passwd = form.get("passwd", [""])[0].replace("%20", " ")

# Validate and sanitize input data
if not login or not passwd:
    print("<html><body><h1>Error: Missing login or password</h1></body></html>")
    exit()

# Connect to the SQLite database
conn = sqlite3.connect('./www/html/database/users.db')
cursor = conn.cursor()

# Check if the user exists and the password matches
cursor.execute('''
SELECT * FROM users WHERE login = ? AND password = ?
''', (login, passwd))

user = cursor.fetchone()

if user:
    # Display a success message
    print("""
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8" />
        <meta name="viewport" content="width=device-width, initial-scale=1.0" />
        <title>42-webserv</title>
        <link rel="stylesheet" href="../assets/style.css" />
        <link
        rel="stylesheet"
        href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0-beta3/css/all.min.css"
        />
        <link
        rel="apple-touch-icon"
        sizes="180x180"
        href="../assets/favicons/apple-touch-icon.png"
        />
        <link
        rel="icon"
        type="image/png"
        sizes="32x32"
        href="../assets/favicons/favicon-32x32.png"
        />
        <link
        rel="icon"
        type="image/png"
        sizes="16x16"
        href="../assets/favicons/favicon-16x16.png"
        />
        <link rel="manifest" href="../assets/favicons/site.webmanifest" />
    </head>
    <body>
        <!-- Navigation bar -->
        <div class="nav_container">
        <a href="../index.html">
            <button id="nav-toggle" class="nav-toggle">
            <i class="fa-solid fa-house"></i>
            </button>
        </a>
        <a href="../account/login.html">
            <button id="nav-toggle" class="nav-toggle">
            <i class="fa-solid fa-user"></i>
            </button>
        </a>
        <a href="../gallery/gallery.html">
            <button id="nav-toggle" class="nav-toggle">
            <i class="fa-solid fa-image"></i>
            </button>
        </a>

        <!-- Mode switch -->
        <button id="mode-switch" class="nav-toggle">
            <i class="fas fa-moon"></i>
        </button>
        </div>
        <!-- small texts -->
        <div class="small-text"></div>
        <!-- Header -->
        <div>
        <a href="index.html" class="logo">
            <h1 class="highlight">Webserv</h1>
        </a>
        </div>
        <!-- 42 icon -->
        <div class="icon">
        <img src="https://simpleicons.org/icons/42.svg" alt="42" />
        </div>

        <!-- Main container -->
        <div class="container">
            <div class="button-container">
            <h1>Login Successful!</h1>
            <p></p>
            <a href="/index.html" class="button">Go to Home Page</a>
        </div>
        <script src="../assets/script.js"></script>
    </body>
    <footer>
        <!-- copyright -->
        <div class="footer">
        <p>
            © 2024
            <a href="https://profile.intra.42.fr/users/msumon" class="highlight"
            >msumon</a
            >
        </p>
        </div>
    </footer>
    </html>

    """)
else:
    # Display an error message
    print("""
    <!DOCTYPE html>
    <html lang="en">
    <head>
        <meta charset="UTF-8" />
        <meta name="viewport" content="width=device-width, initial-scale=1.0" />
        <title>42-webserv</title>
        <link rel="stylesheet" href="../assets/style.css" />
        <link
        rel="stylesheet"
        href="https://cdnjs.cloudflare.com/ajax/libs/font-awesome/6.0.0-beta3/css/all.min.css"
        />
        <link
        rel="apple-touch-icon"
        sizes="180x180"
        href="../assets/favicons/apple-touch-icon.png"
        />
        <link
        rel="icon"
        type="image/png"
        sizes="32x32"
        href="../assets/favicons/favicon-32x32.png"
        />
        <link
        rel="icon"
        type="image/png"
        sizes="16x16"
        href="../assets/favicons/favicon-16x16.png"
        />
        <link rel="manifest" href="../assets/favicons/site.webmanifest" />
    </head>
    <body>
        <!-- Navigation bar -->
        <div class="nav_container">
        <a href="../index.html">
            <button id="nav-toggle" class="nav-toggle">
            <i class="fa-solid fa-house"></i>
            </button>
        </a>
        <a href="../account/login.html">
            <button id="nav-toggle" class="nav-toggle">
            <i class="fa-solid fa-user"></i>
            </button>
        </a>
        <a href="../gallery/gallery.html">
            <button id="nav-toggle" class="nav-toggle">
            <i class="fa-solid fa-image"></i>
            </button>
        </a>

        <!-- Mode switch -->
        <button id="mode-switch" class="nav-toggle">
            <i class="fas fa-moon"></i>
        </button>
        </div>
        <!-- small texts -->
        <div class="small-text"></div>
        <!-- Header -->
        <div>
        <a href="index.html" class="logo">
            <h1 class="highlight">Webserv</h1>
        </a>
        </div>
        <!-- 42 icon -->
        <div class="icon">
        <img src="https://simpleicons.org/icons/42.svg" alt="42" />
        </div>

        <!-- Main container -->
        <div class="container">
            <div class="button-container">
            <h1>ERROR!</h1>
            <p>Invalid login or password</p>
            <p> </p>
            <a href='../account/login.html' class="button" >Go back to login</a>
        </div>
        <script src="../assets/script.js"></script>
    </body>
    <footer>
        <!-- copyright -->
        <div class="footer">
        <p>
            © 2024
            <a href="https://profile.intra.42.fr/users/msumon" class="highlight"
            >msumon</a
            >
        </p>
        </div>
    </footer>
    </html>

    """)

# Close the connection
conn.close()
