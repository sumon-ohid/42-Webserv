# webserv

## Demo website for testing
<img width="1459" alt="webservHome" src="https://github.com/user-attachments/assets/75f3b101-236f-4f0a-9b78-bd73aa6eaa6d">


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
