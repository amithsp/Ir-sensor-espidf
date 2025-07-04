#!/usr/bin/env python3
"""
Python client to test the ESP32 protobuf server.
Sends ControlCommand messages to the ESP32 over TCP.
"""

import socket
import struct
import time
import sys

# Simple protobuf-like message structure for testing
class ControlCommand:
    def __init__(self, id=0, speed=0.0, steering=0.0, enable=False):
        self.id = id
        self.speed = speed
        self.steering = steering
        self.enable = enable
    
    def serialize(self):
        """
        Simple serialization that matches the nanopb format.
        This is a simplified version - in production you'd use the actual protobuf library.
        """
        # For nanopb, we'll create a simple binary format
        # Field 1 (id): varint
        # Field 2 (speed): 4-byte float
        # Field 3 (steering): 4-byte float  
        # Field 4 (enable): 1-byte bool
        
        data = bytearray()
        
        # Field 1: id (tag=1, varint)
        data.extend(self._encode_varint(1 << 3 | 0))  # tag 1, wire type 0 (varint)
        data.extend(self._encode_varint(self.id))
        
        # Field 2: speed (tag=2, fixed32)
        data.extend(self._encode_varint(2 << 3 | 5))  # tag 2, wire type 5 (fixed32)
        data.extend(struct.pack('<f', self.speed))
        
        # Field 3: steering (tag=3, fixed32)
        data.extend(self._encode_varint(3 << 3 | 5))  # tag 3, wire type 5 (fixed32)
        data.extend(struct.pack('<f', self.steering))
        
        # Field 4: enable (tag=4, varint)
        data.extend(self._encode_varint(4 << 3 | 0))  # tag 4, wire type 0 (varint)
        data.extend(self._encode_varint(1 if self.enable else 0))
        
        return bytes(data)
    
    def _encode_varint(self, value):
        """Encode a varint (variable-length integer)"""
        result = bytearray()
        while value >= 0x80:
            result.append((value & 0x7F) | 0x80)
            value >>= 7
        result.append(value & 0x7F)
        return result

def send_command(host, port, command):
    """Send a ControlCommand to the ESP32 server"""
    try:
        # Create socket
        sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        sock.settimeout(5.0)  # 5 second timeout
        
        # Connect to ESP32
        print(f"Connecting to {host}:{port}...")
        sock.connect((host, port))
        print("Connected!")
        
        # Serialize the command
        message_data = command.serialize()
        message_length = len(message_data)
        
        print(f"Sending message (length: {message_length} bytes)")
        print(f"Command: ID={command.id}, Speed={command.speed}, Steering={command.steering}, Enable={command.enable}")
        
        # Send length header (2 bytes, big-endian)
        length_header = struct.pack('>H', message_length)
        sock.send(length_header)
        
        # Send the protobuf message
        sock.send(message_data)
        
        print("Message sent successfully!")
        
    except socket.timeout:
        print("ERROR: Connection timeout!")
    except ConnectionRefusedError:
        print("ERROR: Connection refused! Make sure ESP32 is running and connected to WiFi.")
    except Exception as e:
        print(f"ERROR: {e}")
    finally:
        sock.close()

def main():
    # Default ESP32 IP (you'll need to update this with your ESP32's actual IP)
    ESP32_IP = "10.16.1.17"  # Update this with your ESP32's IP address
    ESP32_PORT = 3333
    
    if len(sys.argv) > 1:
        ESP32_IP = sys.argv[1]
    
    print("ESP32 Protobuf Client Test")
    print("=" * 30)
    print(f"Target: {ESP32_IP}:{ESP32_PORT}")
    print()
    
    # Test commands to send
    test_commands = [
        ControlCommand(id=1, speed=0.5, steering=0.0, enable=True),
        ControlCommand(id=2, speed=1.0, steering=0.25, enable=True),
        ControlCommand(id=3, speed=0.0, steering=-0.5, enable=False),
        ControlCommand(id=4, speed=-0.8, steering=0.1, enable=True),
    ]
    
    for i, cmd in enumerate(test_commands, 1):
        print(f"\n--- Test {i}/{len(test_commands)} ---")
        send_command(ESP32_IP, ESP32_PORT, cmd)
        time.sleep(1)  # Wait 1 second between commands
    
    print("\nAll tests completed!")
    print("\nTo use with custom IP: python test_client.py <ESP32_IP>")

if __name__ == "__main__":
    main()
