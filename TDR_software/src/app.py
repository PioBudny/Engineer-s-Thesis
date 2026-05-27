import tkinter as tk
import serial
import serial.tools.list_ports
import time

ser = None
current_port = None
log_window = None
log_text = None
serial_reader_started = False

# ===== SERIAL =====

def find_pico():
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if "USB" in port.description or "Pico" in port.description:
            return port.device
    return None


def append_log_text(message):
    global log_text
    if log_text:
        log_text.configure(state=tk.NORMAL)
        log_text.insert(tk.END, message + "\n")
        log_text.see(tk.END)
        log_text.configure(state=tk.DISABLED)


def close_log_window():
    global log_window, log_text
    if log_window:
        log_window.destroy()
        log_window = None
        log_text = None


def open_log_window():
    global log_window, log_text
    if log_window and tk.Toplevel.winfo_exists(log_window):
        log_window.lift()
        return

    log_window = tk.Toplevel(root)
    log_window.title("Pico Log")
    log_window.geometry("500x300")

    log_frame = tk.Frame(log_window)
    log_frame.pack(fill=tk.BOTH, expand=True)

    scrollbar = tk.Scrollbar(log_frame)
    scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

    log_text = tk.Text(log_frame, wrap=tk.NONE, state=tk.DISABLED)
    log_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
    log_text.config(yscrollcommand=scrollbar.set)
    scrollbar.config(command=log_text.yview)

    log_window.protocol("WM_DELETE_WINDOW", close_log_window)


def read_serial():
    global ser
    if ser and ser.is_open:
        try:
            line = ser.readline().decode(errors='ignore').strip()
            if line:
                add_log(line)
        except Exception:
            pass
    root.after(100, read_serial)


def start_serial_reader():
    global serial_reader_started
    if not serial_reader_started:
        serial_reader_started = True
        root.after(100, read_serial)


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
        ser = serial.Serial(port, 115200, timeout=0.1)
        time.sleep(2)
        current_port = port
        status_label.itemconfig("led", fill="green")
        add_log("Connected to " + port)
        open_log_window()
        start_serial_reader()
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
                add_log(f"Sending continuous impulses {freq}kHz on {channel_text}")
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

# ===== DEBUG LED =====

def debug_led_on():
    if not (ser and ser.is_open):
        add_log("Not connected to Pico")
        return

    ser.write(b"DEBUG_LED_ON\n")
    add_log("Debug LED turned ON")


def debug_led_off():
    if not (ser and ser.is_open):
        add_log("Not connected to Pico")
        return

    ser.write(b"DEBUG_LED_OFF\n")
    add_log("Debug LED turned OFF")


def read_device_id():
    if not (ser and ser.is_open):
        add_log("Not connected to Pico")
        return
    ser.write(b"READ_ID\n")
    add_log("Requested device ID")

# ===== LOG =====

def add_log(message):
    log_label.config(text=message)
    if log_text:
        append_log_text(message)

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
root.geometry("400x420")

# Top frame
top_frame = tk.Frame(root)
top_frame.pack(fill=tk.X, padx=10, pady=10)

status_label = tk.Canvas(top_frame, width=20, height=20, bg="white", highlightthickness=2)
status_label.pack(side=tk.RIGHT, padx=5)
status_label.create_oval(2, 2, 18, 18, fill="red", outline="black", tags="led")

tk.Button(top_frame, text="Connect", command=connect).pack(side=tk.RIGHT, padx=5)
tk.Button(top_frame, text="Open Log", command=open_log_window).pack(side=tk.RIGHT, padx=5)
tk.Button(top_frame, text="Read Device ID", command=read_device_id).pack(side=tk.RIGHT, padx=5)

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

# DEBUG LED BUTTONS
debug_frame = tk.Frame(root)
debug_frame.pack(pady=10)

tk.Button(debug_frame, text="Debug LED On", command=debug_led_on).pack(side=tk.LEFT, padx=5)
tk.Button(debug_frame, text="Debug LED Off", command=debug_led_off).pack(side=tk.LEFT, padx=5)

# LOG
log_frame = tk.Frame(root)
log_frame.pack(fill=tk.X, padx=10, pady=10)

tk.Label(log_frame, text="LOG:").pack(side=tk.LEFT, padx=5)

log_label = tk.Label(log_frame, text="Waiting for events...", fg="gray", justify=tk.LEFT)
log_label.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)


def on_closing():
    root.destroy()

root.protocol("WM_DELETE_WINDOW", on_closing)
root.mainloop()