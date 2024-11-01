# webserv

https://docs.google.com/document/d/1F30X8uyTnhKQTIqGte4__X8xU85ej6YWzmdL8os1zxE/edit?usp=sharing


# TO TEST POST

```
curl -F "file=@testUpload.txt" http://localhost:8000/
```

# To check a port process

```
lsof -t -i :PORT
```

# Siege Stress Test

```
siege -c 256 -r 1000 http://localhost:8000
```
