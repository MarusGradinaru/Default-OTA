import socket

server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.bind(('0.0.0.0', 9000))
server.listen(1)
print("Server started and listening on 9000 port...")

while True:
  client, addr = server.accept()
  while True:
    data = client.recv(1024)
    if not data: break
    print(data.decode('utf-8'), end='')
  client.close()