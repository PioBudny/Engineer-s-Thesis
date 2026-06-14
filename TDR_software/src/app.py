import tkinter as tk
from tkinter import ttk
import serial
import serial.tools.list_ports
import time

ser = None
current_port = None
log_window = None
log_text = None
serial_reader_started = False
Output_en = 0
Selected_Frequency = 0

# ===== SERIAL =====

def find_pico():
    ports = serial.tools.list_ports.comports()
    for port in ports:
        if "USB" in port.description or "Pico" in port.description:
            return port.device
    return None

def disconnect():
    global ser, current_port, serial_reader_started
    ser = None
    current_port = None
    serial_reader_started = False
    status_label.itemconfig("led", fill="red")
    add_log("Connection lost - Pico disconnected")
    
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
            disconnect()
            root.after(2000, try_reconnect)
            return
    root.after(100, read_serial)

def try_reconnect():
    if ser and ser.is_open:  # już połączony, nie rób nic
        return
    port = find_pico()
    if port:
        connect()
    else:
        root.after(2000, try_reconnect)
        
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
        
def send_impulse():
    global Output_en

    if not (ser and ser.is_open):
        add_log("Not connected to Pico")
        return

    if Output_en == 0:
        add_log("Select Q1 or Q2 first")
        return

    q1_single = int(q1_single_impulse_var.get())
    q2_single = int(q2_single_impulse_var.get())

    cmd = f"IMPULSE_START,{Output_en},{q1_single},{q2_single}\n"
    ser.write(cmd.encode())
    add_log(f"Sent: {cmd.strip()}")

def send_GPIO_impulse():
    global Output_en

    if not (ser and ser.is_open):
        add_log("Not connected to Pico")
        return

    if Output_en == 0:
        add_log("Select Q1 or Q2 first")
        return

    q1_single = int(q1_single_impulse_var.get())
    q2_single = int(q2_single_impulse_var.get())

    cmd = f"IMPULSE_GPIO_START,{Output_en},{q1_single},{q2_single}\n"
    ser.write(cmd.encode())
    add_log(f"Sent: {cmd.strip()}")

def stop_impulse():
    if ser and ser.is_open:
        ser.write(b"IMPULSE_STOP\n")
        add_log("Stopped sending impulses")
    else:
        add_log("Not connected to Pico")


def load_configuration():
    global jitter_ant

    if not (ser and ser.is_open):
        add_log("Not connected to Pico")
        return

    source_map = {"Crystal": 0, "External": 1, "PLL": 2}

    q1_source = source_map[q1_source_var.get()]
    q2_source = source_map[q2_source_var.get()]

    q1_freq = q1_selected_frequency
    q2_freq = q2_selected_frequency
    
    jitter_ant = int(
    q1_source_var.get() == "External" or
    q2_source_var.get() == "External"
)

    cmd = (
        f"LOAD_CONFIG,"
        f"{jitter_ant},"
        f"{Output_en},"
        f"{q1_source},{q2_source},"
        f"{q1_freq},{q2_freq}\n"
    )

    ser.write(cmd.encode())
    add_log(f"Sent: {cmd.strip()}")


def read_regs():
    if not (ser and ser.is_open):
        add_log("Not connected to Pico")
        return
    ser.write(b"READ_REGS\n")
    add_log("Requested device register dump")


def default_config():
    if not (ser and ser.is_open):
        add_log("Not connected to Pico")
        return
    ser.write(b"DEFAULT_CONFIG\n")
    
def Innitial_Config():
    if not (ser and ser.is_open):
        add_log("Not connected to Pico")
        return
    ser.write(b"Innital_Config\n")
    add_log("Loaded innital configuration")
    
def Calibrate_PLL():
    if not (ser and ser.is_open):
        add_log("Not connected to Pico")
        return
    ser.write(b"CALIBRATE_PLL\n")


# ===== LOG =====

def add_log(message):
    log_label.config(text=message)
    if log_text:
        append_log_text(message)


# ===== GUI =====

root = tk.Tk()
root.title("TDR App")
root.geometry("600x450")

# Top frame
top_frame = tk.Frame(root)
top_frame.pack(fill=tk.X, padx=10, pady=10)

status_label = tk.Canvas(top_frame, width=20, height=20, bg="white", highlightthickness=2)
status_label.pack(side=tk.RIGHT, padx=5)
status_label.create_oval(2, 2, 18, 18, fill="red", outline="black", tags="led")

tk.Button(top_frame, text="Connect",        command=connect).pack(side=tk.RIGHT, padx=5)
tk.Button(top_frame, text="Open Log",       command=open_log_window).pack(side=tk.RIGHT, padx=5)
tk.Button(top_frame, text="Read Registers", command=read_regs).pack(side=tk.RIGHT, padx=5)
tk.Button(top_frame, text="Default config", command=default_config).pack(side=tk.RIGHT, padx=5)
tk.Button(top_frame, text="Innitial Config", command=Innitial_Config).pack(side=tk.RIGHT, padx=5)
tk.Button(top_frame, text="Calibrate PLL", command=Calibrate_PLL).pack(side=tk.RIGHT, padx=5)

q1_var                = tk.BooleanVar()
q2_var                = tk.BooleanVar()
q1_source_var         = tk.StringVar(value="Crystal")
q2_source_var         = tk.StringVar(value="Crystal")
q1_single_impulse_var = tk.BooleanVar()
q2_single_impulse_var = tk.BooleanVar()
q1_frequency_var      = tk.StringVar()
q2_frequency_var      = tk.StringVar()
q1_selected_frequency = 0
q2_selected_frequency = 0


def get_frequency_options(source):
    if source == "Crystal":
        return ["1","4", "10k"]
    elif source == "External":
        return ["1", "16"]
    else:  # PLL
        return ["25", "50", "100", "200"]


def update_q1_frequency_options():
    global q1_selected_frequency
    options = get_frequency_options(q1_source_var.get())
    q1_frequency_combo['values'] = options
    if options:
        q1_frequency_var.set(options[0])
        update_q1_selected_frequency()


def update_q2_frequency_options():
    global q2_selected_frequency
    options = get_frequency_options(q2_source_var.get())
    q2_frequency_combo['values'] = options
    if options:
        q2_frequency_var.set(options[0])
        update_q2_selected_frequency()


def update_q1_selected_frequency():
    global q1_selected_frequency
    if q1_frequency_var.get() == "10k":
        q1_selected_frequency = 10000
    else:
        q1_selected_frequency = int(q1_frequency_var.get())


def update_q2_selected_frequency():
    global q2_selected_frequency
    if q2_frequency_var.get() == "10k":
        q2_selected_frequency = 10000
    else:
        q2_selected_frequency = int(q2_frequency_var.get())


def update_output():
    global Output_en
    Output_en = 0
    if q1_var.get():
        Output_en |= 0b0100
    if q2_var.get():
        Output_en |= 0b1000
    if Output_en == 0:
        add_log("No outputs selected")


# Container for config frames (side-by-side)
config_container = tk.Frame(root)
config_container.pack(fill=tk.BOTH, expand=True, padx=0, pady=0)

# ── Q1 Configuration Frame ───────────────────────────────────────────────────
q1_config_frame = tk.Frame(config_container, relief=tk.SUNKEN, borderwidth=1)
q1_config_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5, pady=5)

tk.Label(q1_config_frame, text="Q1 Configuration", font=("Arial", 10, "bold")).pack(pady=5)

q1_source_frame = tk.Frame(q1_config_frame)
q1_source_frame.pack(fill=tk.X, padx=5, pady=5)
tk.Label(q1_source_frame, text="Source:").pack(side=tk.LEFT, padx=5)
tk.Radiobutton(q1_source_frame, text="Crystal",  variable=q1_source_var, value="Crystal",  command=update_q1_frequency_options).pack(side=tk.LEFT, padx=5)
tk.Radiobutton(q1_source_frame, text="External", variable=q1_source_var, value="External", command=update_q1_frequency_options).pack(side=tk.LEFT, padx=5)
tk.Radiobutton(q1_source_frame, text="PLL",      variable=q1_source_var, value="PLL",      command=update_q1_frequency_options).pack(side=tk.LEFT, padx=5)

q1_freq_frame = tk.Frame(q1_config_frame)
q1_freq_frame.pack(fill=tk.X, padx=5, pady=5)
tk.Label(q1_freq_frame, text="Frequency (MHz):").pack(side=tk.LEFT, padx=5)
q1_frequency_combo = ttk.Combobox(q1_freq_frame, textvariable=q1_frequency_var,
                                   values=get_frequency_options("Crystal"),
                                   state='readonly', width=10)
q1_frequency_combo.pack(side=tk.LEFT, padx=5)
q1_frequency_combo.bind('<<ComboboxSelected>>', lambda e: update_q1_selected_frequency())
q1_frequency_var.set(get_frequency_options("Crystal")[0])
update_q1_selected_frequency()

q1_impulse_frame = tk.Frame(q1_config_frame)
q1_impulse_frame.pack(fill=tk.X, padx=5, pady=5)
tk.Checkbutton(q1_impulse_frame, text="Single impulse", variable=q1_single_impulse_var).pack(anchor="w", padx=5)
tk.Checkbutton(q1_impulse_frame, text="Enable Q1",      variable=q1_var, command=update_output).pack(anchor="w", padx=5)

# ── Q2 Configuration Frame ───────────────────────────────────────────────────
q2_config_frame = tk.Frame(config_container, relief=tk.SUNKEN, borderwidth=1)
q2_config_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5, pady=5)

tk.Label(q2_config_frame, text="Q2 Configuration", font=("Arial", 10, "bold")).pack(pady=5)

q2_source_frame = tk.Frame(q2_config_frame)
q2_source_frame.pack(fill=tk.X, padx=5, pady=5)
tk.Label(q2_source_frame, text="Source:").pack(side=tk.LEFT, padx=5)
tk.Radiobutton(q2_source_frame, text="Crystal",  variable=q2_source_var, value="Crystal",  command=update_q2_frequency_options).pack(side=tk.LEFT, padx=5)
tk.Radiobutton(q2_source_frame, text="External", variable=q2_source_var, value="External", command=update_q2_frequency_options).pack(side=tk.LEFT, padx=5)
tk.Radiobutton(q2_source_frame, text="PLL",      variable=q2_source_var, value="PLL",      command=update_q2_frequency_options).pack(side=tk.LEFT, padx=5)

q2_freq_frame = tk.Frame(q2_config_frame)
q2_freq_frame.pack(fill=tk.X, padx=10, pady=5)
tk.Label(q2_freq_frame, text="Frequency (MHz):").pack(side=tk.LEFT, padx=5)
q2_frequency_combo = ttk.Combobox(q2_freq_frame, textvariable=q2_frequency_var,
                                   values=get_frequency_options("Crystal"),
                                   state='readonly', width=10)
q2_frequency_combo.pack(side=tk.LEFT, padx=5)
q2_frequency_combo.bind('<<ComboboxSelected>>', lambda e: update_q2_selected_frequency())
q2_frequency_var.set(get_frequency_options("Crystal")[0])
update_q2_selected_frequency()

q2_impulse_frame = tk.Frame(q2_config_frame)
q2_impulse_frame.pack(fill=tk.X, padx=5, pady=5)
tk.Checkbutton(q2_impulse_frame, text="Single impulse", variable=q2_single_impulse_var).pack(anchor="w", padx=5)
tk.Checkbutton(q2_impulse_frame, text="Enable Q2",      variable=q2_var, command=update_output).pack(anchor="w", padx=5)

# ── Buttons ──────────────────────────────────────────────────────────────────
buttons_frame = tk.Frame(root)
buttons_frame.pack(fill=tk.X, padx=10, pady=10)

tk.Button(buttons_frame, text="Load Configuration", command=load_configuration, width=15).pack(pady=5)
tk.Button(buttons_frame, text="Send impulse by i2C",        command=send_impulse,       width=15).pack(pady=5)
tk.Button(buttons_frame, text="Send impulse by GPIO",        command=send_GPIO_impulse,       width=15).pack(pady=5)
tk.Button(buttons_frame, text="Stop",                command=stop_impulse,       width=15).pack(pady=5)

# ── LOG ───────────────────────────────────────────────────────────────────────
log_frame = tk.Frame(root)
log_frame.pack(fill=tk.X, padx=10, pady=10, side=tk.BOTTOM)

tk.Label(log_frame, text="LOG:").pack(side=tk.LEFT, padx=5)
log_label = tk.Label(log_frame, text="Waiting for events...", fg="gray", justify=tk.LEFT)
log_label.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)


def on_closing():
    root.destroy()

root.protocol("WM_DELETE_WINDOW", on_closing)
root.mainloop()