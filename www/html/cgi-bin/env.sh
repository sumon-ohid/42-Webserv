while true; do
	sleep 1
	# echo 'j'
done
#!/bin/bash
echo -e 'Content-Type: text/html\r\n'
echo -e '\r\n\r\n'
echo '<!DOCTYPE html>'
echo '<html lang="en">'
echo '<head>'
echo '    <meta charset="UTF-8">'
echo '    <meta name="viewport" content="width=device-width, initial-scale=1.0">'
echo '    <title>Environment Variables</title>'
echo '    <style>'
echo '        body { font-family: Arial, sans-serif; margin: 20px; }'
echo '        h1 { color: #333; }'
echo '        h4 { color: #333; }'
echo '        pre { background: #f4f4f4; padding: 10px; border: 1px solid #ddd; }'
echo '    </style>'
echo '</head>'
echo '<body>'
echo '    <h1>Webserv CGI test.</h1>'
echo '    <h4>Environment Variables</h4>'
echo '    <pre>'
env
echo '    </pre>'
echo '    <h4>System OS Type</h4>'
echo '    <pre>'
uname
echo '    </pre>'
echo '    <h4>Current Directory</h4>'
echo '    <pre>'
ls
echo '    </pre>'
echo '</body>'
echo '</html>'
