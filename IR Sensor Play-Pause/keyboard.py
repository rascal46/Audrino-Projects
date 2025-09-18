import serial
import keyboard

PORT = "COM16"   # Change to your ESP32 COM port
BAUD = 115200

ser = serial.Serial(PORT, BAUD)

print("Listening for ESP32 signals...")

while True:
    line = ser.readline().decode().strip()
    print("ESP32 says:", line)
    if "PLAY_PAUSE" in line:
        keyboard.press_and_release("space")  # YouTube play/pause