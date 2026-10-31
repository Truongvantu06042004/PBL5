import socket


ESP_IP = "172.20.10.2"
PORT = 3333


sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)


sock.connect((ESP_IP, PORT))


print("Connected to ESP32")


while True:

    cmd = input("Command ON/OFF: ")

    sock.send(cmd.encode())
