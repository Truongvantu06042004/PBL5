import socket


HOST = "0.0.0.0"
PORT = 3333


server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

server.bind((HOST, PORT))

server.listen(1)


print("Waiting ESP32 connect...")


conn, addr = server.accept()

print("ESP32 connected:", addr)


while True:

    cmd = input("Command ON/OFF: ")

    conn.send(cmd.encode())
