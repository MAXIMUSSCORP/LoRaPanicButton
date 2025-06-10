import serial
import time
import pygame
import os
import sys

# Serial connection settings
port = '/dev/ttyUSB0'  # Common for USB-Serial adapters on Pi
baud_rate = 9600
timeout = 1

# Device ID to location mapping
device_locations = {
    1: "Bar1",
    2: "Bar2"
    # Add more devices as needed: 3: "Bar3", 4: "Bar4", etc.
}

# Sound files mapping (relative to script directory)
script_dir = os.path.dirname(os.path.abspath(__file__))
sound_files = {
    "Bar1": {
        "HELP": os.path.join(script_dir, "sounds", "HelpBar1.wav"),
        "EMERGENCY": os.path.join(script_dir, "sounds", "EmergencyBar1.wav")
    },
    "Bar2": {
        "HELP": os.path.join(script_dir, "sounds", "HelpBar2.wav"),
        "EMERGENCY": os.path.join(script_dir, "sounds", "EmergencyBar2.wav")
    }
    # Add more locations as needed
}

def init_audio():
    """Initialize pygame mixer for audio playback"""
    pygame.mixer.init()

def play_sound_for_message(device_id, message_type):
    """Play appropriate sound based on device ID and message type"""
    if device_id in device_locations:
        location = device_locations[device_id]

        if location in sound_files and message_type in sound_files[location]:
            sound_file = sound_files[location][message_type]
            if os.path.exists(sound_file):
                print(f"Playing {location} {message_type.lower()} sound...")
                try:
                    pygame.mixer.music.load(sound_file)
                    pygame.mixer.music.play()
                    # Wait for sound to finish
                    while pygame.mixer.music.get_busy():
                        time.sleep(0.1)
                except pygame.error as e:
                    print(f"Error playing sound: {e}")
            else:
                print(f"Sound file not found: {sound_file}")
        else:
            print(f"No sound file configured for {location} {message_type}")
    else:
        print(f"Unknown device ID: {device_id}")

def main():
    print("Initializing audio system...")
    init_audio()

    try:
        # Initialize serial connection
        ser = serial.Serial(port, baud_rate, timeout=timeout)
        print(f"Connected to {port} at {baud_rate} baud.")
        print("Waiting for LoRa messages...")

        # Infinite loop to read data
        while True:
            if ser.in_waiting > 0:
                line = ser.readline().decode('utf-8').strip()
                print(f"Received: {line}")

                # Parse the message format: "DEVICE_ID:MESSAGE_TYPE"
                if ':' in line:
                    try:
                        device_id_str, message_type = line.split(':')
                        device_id = int(device_id_str)

                        # Play appropriate sound
                        play_sound_for_message(device_id, message_type)

                    except ValueError:
                        print(f"Invalid message format: {line}")
                else:
                    # Handle other serial messages (like status messages)
                    if "parity error" not in line.lower():
                        print(f"Status: {line}")

            time.sleep(0.1)

    except serial.SerialException as e:
        print(f"Serial Error: {e}")
        print("Make sure Arduino is connected and check the port")
    except KeyboardInterrupt:
        print("\nMonitoring stopped.")
    except Exception as e:
        print(f"Unexpected error: {e}")
    finally:
        if 'ser' in locals() and ser.is_open:
            ser.close()
        print("Serial connection closed.")
        pygame.mixer.quit()

if __name__ == "__main__":
    main()