This is a HTTP Web Server which accepts multiple connections by creating a child process after every request.
This Web Server to support HTTP 1.1 persistent connection the socket closes after every 10 seconds
The Webserver is implemented for GET and POST. The POST accepts only field from the web page.

For Invalid HTTP Version
400 Bad Request
For Invalid File
404 File not Found
For requests other than POST and GET
501 Not Implemented

