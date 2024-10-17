import socket

sfd = socket.socket(socket.AF_INET)
message = bytes("GET / HTTP/1.1\r\n\r\n", "utf-8")
sfd.connect(("localhost", 8000))
sfd.sendall(message)
sfd.recv(4096)
sfd.recv(4096)
sfd.close()
