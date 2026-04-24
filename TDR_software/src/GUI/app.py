import tkinter as tk
import serial
import serial.tools.list_ports
import time

ser = None
current_port = None

def find_pico():
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if "USB" in port.description or "Pico" in port.description:
            return port.device
    return None

def connect():
    global ser, current_port

    port = find_pico()

    # no device → Disconnected
    if not port:
        status_label.config(text="Disconnected")
        ser = None
        current_port = None
        return

    # already connected → do nothing
    if ser and ser.is_open and port == current_port:
        status_label.config(text="Connected")
        return

    try:
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2)
        current_port = port
        status_label.config(text="Connected")
    except:
        ser = None
        current_port = None
        status_label.config(text="Disconnected")

def send_led_on():
    if ser and ser.is_open:
        ser.write(b"LED_ON\n")

def send_led_off():
    if ser and ser.is_open:
        ser.write(b"LED_OFF\n")

# GUI
root = tk.Tk()
root.title("Pico Control")

status_label = tk.Label(root, text="Disconnected", font=("Arial", 12))
status_label.pack(pady=10)

tk.Button(root, text="Connect", command=connect).pack(pady=5)
tk.Button(root, text="LED ON", command=send_led_on).pack(pady=5)
tk.Button(root, text="LED OFF", command=send_led_off).pack(pady=5)

root.mainloop()