#!/bin/bash

# Raspberry Pi LoRa Receiver Setup Script
# This script sets up the Python environment and systemd service

set -e  # Exit on any error

# Configuration
PROJECT_DIR="/home/pi/lora_receiver"
VENV_DIR="$PROJECT_DIR/venv"
SERVICE_NAME="lora-receiver"
PYTHON_SCRIPT="lora_receiver.py"
REQUIREMENTS_FILE="requirements.txt"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}=== Raspberry Pi LoRa Receiver Setup ===${NC}"

# Check if running as root
if [[ $EUID -eq 0 ]]; then
   echo -e "${RED}This script should not be run as root. Run as pi user.${NC}"
   exit 1
fi

# Create project directory
echo -e "${YELLOW}Creating project directory...${NC}"
mkdir -p "$PROJECT_DIR"
cd "$PROJECT_DIR"

# Create requirements.txt if it doesn't exist
if [ ! -f "$REQUIREMENTS_FILE" ]; then
    echo -e "${YELLOW}Creating requirements.txt...${NC}"
    cat > "$REQUIREMENTS_FILE" << EOF
pyserial==3.5
pygame==2.1.3
EOF
fi

# Create virtual environment if it doesn't exist
if [ ! -d "$VENV_DIR" ]; then
    echo -e "${YELLOW}Creating virtual environment...${NC}"
    python3 -m venv "$VENV_DIR"
else
    echo -e "${GREEN}Virtual environment already exists.${NC}"
fi

# Activate virtual environment and install requirements
echo -e "${YELLOW}Installing Python requirements...${NC}"
source "$VENV_DIR/bin/activate"
pip install --upgrade pip
pip install -r "$REQUIREMENTS_FILE"

# Create the Python script if it doesn't exist
if [ ! -f "$PYTHON_SCRIPT" ]; then
    echo -e "${YELLOW}Creating Python receiver script...${NC}"
    cat > "$PYTHON_SCRIPT" << 'EOF'
import serial
import time
import pygame
import os
import sys

# Serial connection settings
port = '/dev/ttyUSB0'
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
EOF
fi

# Create sounds directory
mkdir -p "$PROJECT_DIR/sounds"

# Create systemd service file
echo -e "${YELLOW}Creating systemd service...${NC}"
sudo tee /etc/systemd/system/${SERVICE_NAME}.service > /dev/null << EOF
[Unit]
Description=LoRa Receiver Service
After=network.target

[Service]
Type=simple
User=pi
Group=pi
WorkingDirectory=$PROJECT_DIR
Environment=PATH=$VENV_DIR/bin
ExecStart=$VENV_DIR/bin/python $PROJECT_DIR/$PYTHON_SCRIPT
Restart=always
RestartSec=10

[Install]
WantedBy=multi-user.target
EOF

# Add pi user to dialout group for serial access
echo -e "${YELLOW}Adding pi user to dialout group...${NC}"
sudo usermod -a -G dialout pi

# Reload systemd and enable service
echo -e "${YELLOW}Enabling systemd service...${NC}"
sudo systemctl daemon-reload
sudo systemctl enable ${SERVICE_NAME}.service

echo -e "${GREEN}=== Setup Complete! ===${NC}"
echo ""
echo -e "${YELLOW}Next steps:${NC}"
echo "1. Copy your .wav sound files to: $PROJECT_DIR/sounds/"
echo "2. Connect your Arduino to USB port"
echo "3. Check the serial port in the Python script (currently set to /dev/ttyUSB0)"
echo "4. Start the service: sudo systemctl start ${SERVICE_NAME}"
echo "5. Check service status: sudo systemctl status ${SERVICE_NAME}"
echo "6. View logs: sudo journalctl -u ${SERVICE_NAME} -f"
echo ""
echo -e "${YELLOW}Manual testing:${NC}"
echo "cd $PROJECT_DIR && source venv/bin/activate && python $PYTHON_SCRIPT"
echo ""
echo -e "${GREEN}Remember to reboot or log out/in for group changes to take effect!${NC}"