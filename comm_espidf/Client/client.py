import socket
import sensor_pb2

HOST = '10.16.1.17'  # IP of ESP32
PORT = 3333           # Port ESP32 is listening on

# Create and populate protobuf message
data = sensor_pb2.SensorData()
data.temperature = 27.5
data.humidity = 65.2

# Serialize to bytes
serialized_data = data.SerializeToString()

# Send over TCP
with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as s:
    s.connect((HOST, PORT))
    s.sendall(serialized_data)
    print("Data sent to ESP32")
