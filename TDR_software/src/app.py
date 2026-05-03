import tkinter as tk
import serial
import serial.tools.list_ports
import time

ser = None
current_port = None

# ===== SERIAL =====

def find_pico():
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if "USB" in port.description or "Pico" in port.description:
            return port.device
    return None

def connect():
    global ser, current_port

    port = find_pico()

    if not port:
        ser = None
        current_port = None
        status_label.itemconfig("led", fill="red")
        add_log("Disconnected - Pico not found")
        return

    if ser and ser.is_open and port == current_port:
        status_label.itemconfig("led", fill="green")
        add_log("Already connected to " + port)
        return

    try:
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2)
        current_port = port
        status_label.itemconfig("led", fill="green")
        add_log("Connected to " + port)
    except Exception as e:
        ser = None
        current_port = None
        status_label.itemconfig("led", fill="red")
        add_log(f"Connection failed: {e}")

# ===== CHANNEL SELECTION =====

def get_selected_channel():
    ch1 = channel1_var.get()
    ch2 = channel2_var.get()

    if ch1 and ch2:
        return "12", "Channel 1 and 2"
    elif ch1:
        return "1", "Channel 1"
    elif ch2:
        return "2", "Channel 2"
    else:
        add_log("No channel selected!")
        return None, None

# ===== KOMENDY =====

def send_impulse():
    if not (ser and ser.is_open):
        add_log("Not connected to Pico")
        return

    channel, channel_text = get_selected_channel()
    if not channel:
        return

    if single_impulse_var.get():
        ser.write(f"IMPULSE_SINGLE:{channel}\n".encode())
        add_log(f"Single impulse on {channel_text}")

    elif continuous_signal_var.get():
        try:
            freq = float(frequency_entry.get())

            if 8 <= freq <= 250:
                scaled_freq = freq * 1000  # konwersja z kHz na Hz
                ser.write(f"IMPULSE_CONTINUOUS:{int(scaled_freq)}:{channel}\n".encode())
                add_log(f"Sending continous impulses {freq}kHz on {channel_text}")
            else:
                frequency_label.config(text="Frequency out of range (8-250 kHz)")
                add_log("Frequency out of range")

        except ValueError:
            frequency_label.config(text="Invalid frequency value")
            add_log("Invalid frequency value")

def stop_impulse():
    if ser and ser.is_open:
        ser.write(b"IMPULSE_STOP\n")
        add_log("Stopped sending impulses")
    else:
        add_log("Not connected to Pico")

# ===== LOG =====

def add_log(message):
    log_label.config(text=message)

# ===== TRYBY =====

def on_single_impulse_change():
    if single_impulse_var.get():
        continuous_signal_var.set(False)
        send_impulse_button.config(state=tk.NORMAL)
    else:
        send_impulse_button.config(state=tk.DISABLED)

def on_continuous_signal_change():
    if continuous_signal_var.get():
        single_impulse_var.set(False)
        send_impulse_button.config(state=tk.NORMAL)
    else:
        send_impulse_button.config(state=tk.DISABLED)

# ===== GUI =====

root = tk.Tk()
root.title("TDR App")
root.geometry("400x350")

# Top frame
top_frame = tk.Frame(root)
top_frame.pack(fill=tk.X, padx=10, pady=10)

status_label = tk.Canvas(top_frame, width=20, height=20, bg="white", highlightthickness=2)
status_label.pack(side=tk.RIGHT, padx=5)
status_label.create_oval(2, 2, 18, 18, fill="red", outline="black", tags="led")

tk.Button(top_frame, text="Connect", command=connect).pack(side=tk.RIGHT, padx=5)

# TRYBY
checkbox_frame = tk.Frame(root)
checkbox_frame.pack(pady=10)

single_impulse_var = tk.BooleanVar()
continuous_signal_var = tk.BooleanVar()

tk.Checkbutton(
    checkbox_frame,
    text="Single Impulse",
    variable=single_impulse_var,
    command=on_single_impulse_change
).pack(side=tk.LEFT, padx=20)

tk.Checkbutton(
    checkbox_frame,
    text="Continuous",
    variable=continuous_signal_var,
    command=on_continuous_signal_change
).pack(side=tk.LEFT, padx=20)

# CHANNELS
channel_frame = tk.Frame(root)
channel_frame.pack(pady=10)

channel1_var = tk.BooleanVar()
channel2_var = tk.BooleanVar()

tk.Label(channel_frame, text="Channel:").pack(side=tk.LEFT, padx=10)

tk.Checkbutton(channel_frame, text="Channel 1", variable=channel1_var).pack(side=tk.LEFT, padx=5)
tk.Checkbutton(channel_frame, text="Channel 2", variable=channel2_var).pack(side=tk.LEFT, padx=5)

# FREQUENCY
frequency_frame = tk.Frame(root)
frequency_frame.pack(pady=5)

frequency_label = tk.Label(frequency_frame, text="Frequency (8-250 kHz):")
frequency_label.pack(side=tk.LEFT, padx=5)

frequency_entry = tk.Entry(frequency_frame, width=10)
frequency_entry.insert(0, "100")
frequency_entry.pack(side=tk.LEFT, padx=5)

# BUTTONS
buttons_frame = tk.Frame(root)
buttons_frame.pack(pady=20)

send_impulse_button = tk.Button(
    buttons_frame,
    text="Send Impulse",
    command=send_impulse,
    state=tk.DISABLED
)
send_impulse_button.pack(pady=10)

tk.Button(buttons_frame, text="Stop", command=stop_impulse).pack(pady=5)

# LOG
log_messages = []

log_frame = tk.Frame(root)
log_frame.pack(fill=tk.X, padx=10, pady=10)

tk.Label(log_frame, text="LOG:").pack(side=tk.LEFT, padx=5)

log_label = tk.Label(log_frame, text="Waiting for events...", fg="gray", justify=tk.LEFT)
log_label.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

def on_closing():
    root.destroy()

root.protocol("WM_DELETE_WINDOW", on_closing)
root.mainloop()