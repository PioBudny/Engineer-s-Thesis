import sys
from pathlib import Path
import tkinter as tk
# from tkinter import ttk
import serial
import serial.tools.list_ports
import time
from Impedance_wave import Impedance_Wave as _Impedance_Wave
sys.path.append(str(Path(__file__).resolve().parent.parent))
from pico_control import (
    open_pico_control_window as open_pico_control_window_module,
    close_pico_control_window as close_pico_control_window_module,
)

APP_NAME = "TDR Control Software"
APP_VERSION = "1.0"
APP_PCB_COMPAT = "TDR 3.0"
APP_SOURCE_URL = "https://github.com/<placeholder>/TDR_software"
APP_AUTHOR = "Piotr Budny"

ser = None
current_port = None
log_window = None
log_text = None
serial_reader_started = False
Output_en = 0
Selected_Frequency = 0
impedance_window = None
info_window = None

# ===== SERIAL =====

def find_pico():
    ports = serial.tools.list_ports.comports()
    for port in ports:
        desc = port.description.lower()
        hwid = port.hwid.lower()
        if "2e8a" in hwid or "pico" in desc or "raspberry" in desc:
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
            if ser.in_waiting:
                line = ser.readline()
                if line:
                    add_log(line.decode(errors='ignore').strip())
        except Exception:
            disconnect()
            root.after(2000, try_reconnect)
            return
    root.after(100, read_serial)

def try_reconnect():
    if ser and ser.is_open: 
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
        status_label.itemconfig("led", fill="red")
        add_log("Disconnected - Pico not found")
        return

    if ser and ser.is_open and port == current_port:
        add_log("Already connected to " + port)
        return

    try:
        ser = serial.Serial(port, 115200, timeout=0.1)
        ser.reset_input_buffer()

        ser.write(b"PING\n")
        root.update_idletasks()
        response = ser.readline().decode(errors='ignore').strip()
        if response != "PONG":
            add_log(f"Handshake failed (got: '{response}')")
            ser.close()
            ser = None
            status_label.itemconfig("led", fill="red")
            return

        current_port = port
        status_label.itemconfig("led", fill="green")
        add_log("Connected to " + port)
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


def stop_impulse():
    if ser and ser.is_open:
        ser.write(b"IMPULSE_STOP\n")
        add_log("Stopped sending impulses")
    else:
        add_log("Not connected to Pico")

def read_regs():
    if not (ser and ser.is_open):
        add_log("Not connected to Pico")
        return
    ser.write(b"READ_REGS\n")
    
def Innitial_Config():
    if not (ser and ser.is_open):
        add_log("Not connected to Pico")
        return
    ser.write(b"Innital_Config\n")
    
def Calibrate_PLL():
    if not (ser and ser.is_open):
        add_log("Not connected to Pico")
        return
    ser.write(b"CALIBRATE_PLL\n")

# ===== IMPEDANCE WAVE WINDOW =====
 
def close_impedance_window():
    global impedance_window
    if impedance_window:
        impedance_window.destroy()
        impedance_window = None
 

def Impedance_Wave():
    global impedance_window
    if impedance_window and tk.Toplevel.winfo_exists(impedance_window):
        impedance_window.lift()
        return
    impedance_window = _Impedance_Wave(root, close_impedance_window)
    
# ===== CALCULATOR WINDOW =====

# ===== PICO CONTROL WINDOW =====

def open_pico_control_window():
    open_pico_control_window_module(root, add_log)


def close_pico_control_window():
    close_pico_control_window_module()

# ===== INFO WINDOW =====

def close_info_window():
    global info_window
    if info_window:
        info_window.destroy()
        info_window = None


def open_info_window():
    global info_window
    if info_window and tk.Toplevel.winfo_exists(info_window):
        info_window.lift()
        return

    info_window = tk.Toplevel(root)
    info_window.title("Info")
    info_window.geometry("650x500")

    main_frame = tk.Frame(info_window, padx=15, pady=15)
    main_frame.pack(fill=tk.BOTH, expand=True)

    info_frame = tk.LabelFrame(main_frame, text="Info", padx=10, pady=10)
    info_frame.pack(fill=tk.X, pady=(0, 10))

    tk.Label(
        info_frame,
        text=f"{APP_NAME} v{APP_VERSION}",
        font=("Arial", 10, "bold"),
        anchor="w",
        justify=tk.LEFT,
    ).pack(fill=tk.X)
    tk.Label(
        info_frame, text=f"Compatible with PCB: {APP_PCB_COMPAT}", anchor="w", justify=tk.LEFT
    ).pack(fill=tk.X, pady=(4, 0))
    tk.Label(
        info_frame,
        text=f"Source code: {APP_SOURCE_URL}",
        anchor="w",
        justify=tk.LEFT,
        wraplength=380,
    ).pack(fill=tk.X, pady=(4, 0))
    tk.Label(info_frame, text=f"Author: {APP_AUTHOR}", anchor="w", justify=tk.LEFT).pack(
        fill=tk.X, pady=(4, 0)
    )

    manual_frame = tk.LabelFrame(main_frame, text="User Manual", padx=10, pady=10)
    manual_frame.pack(fill=tk.BOTH, expand=True)

    manual_text_frame = tk.Frame(manual_frame)
    manual_text_frame.pack(fill=tk.BOTH, expand=True)

    manual_scrollbar = tk.Scrollbar(manual_text_frame)
    manual_scrollbar.pack(side=tk.RIGHT, fill=tk.Y)

    manual_text = tk.Text(manual_text_frame, wrap=tk.WORD, height=14)
    manual_text.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)
    manual_text.config(yscrollcommand=manual_scrollbar.set)
    manual_scrollbar.config(command=manual_text.yview)

    manual_content = (
        "Connect - establishes a serial connection with the Raspberry Pi Pico.\n\n"

        "Pico Control - opens the firmware build and programming window:\n"
        "  • Manual mode - copies the .uf2 file to the selected RPI-RP2 drive\n"
        "    using 'Program Pico'. The Pico must be in BOOTSEL mode and the\n"
        "    drive must be selected manually.\n"
        "  • Picotool mode - programs the Pico using the picotool utility.\n"
        "    The device is detected automatically over USB.\n"
        "    BOOTSEL mode is not required, but picotool must be installed.\n\n"

        "Open Log - opens a window containing the complete communication log.\n\n"

        "Initial Config - sends the default NLG9881 configuration to the Pico.\n\n"

        "Impedance Wave - opens the TDR impedance calculation and plotting window.\n\n"

        "  • Browse - opens a file dialog to select a .csv file containing\n"
        "    TDR measurement data.\n"
        "  • Load - loads the selected .csv file and displays the waveform.\n"
        "  • Calculate - calculates the impedance profile from the loaded\n"
        "    TDR measurement.\n"
        "  • Z0 - sets the reference characteristic impedance used for the\n"
        "    calculation.\n"
        "  • Vf / Er - specifies either the velocity factor (Vf) or the\n"
        "    relative permittivity (Er) and its value for the calculation.\n"
        "  • Segments with Different Vf or Er - allows different cable\n"
        "    sections to be defined with individual velocity factors or\n"
        "    relative permittivity values.\n\n"

        "Send Impulse - generates a pulse on the selected outputs (Q1/Q2).\n\n"
        "  • Single Impulse - when enabled, only a single pulse is generated\n"
        "    on the selected outputs.\n"
        "  • Enable Q1/Q2 - selects which outputs will generate the pulse.\n\n"

        "Stop - stops pulse generation on all outputs."
    )
    manual_text.insert(tk.END, manual_content)
    manual_text.config(state=tk.DISABLED)

    info_window.protocol("WM_DELETE_WINDOW", close_info_window)

# ===== LOG =====

def add_log(message):
    log_label.config(text=message)
    if log_text:
        append_log_text(message)


# ===== GUI =====

root = tk.Tk()
root.title("TDR App")
root.geometry("600x400")

# Top frame
top_frame = tk.Frame(root)
top_frame.pack(fill=tk.X, padx=10, pady=10)

status_label = tk.Canvas(top_frame, width=20, height=20, bg="white", highlightthickness=2)
status_label.pack(side=tk.RIGHT, padx=5)
status_label.create_oval(2, 2, 18, 18, fill="red", outline="black", tags="led")

tk.Button(top_frame, text="Connect",        command=connect).pack(side=tk.RIGHT, padx=5)
tk.Button(top_frame, text="Pico control", command=open_pico_control_window).pack(side=tk.RIGHT, padx=5)
tk.Button(top_frame, text="Open Log",       command=open_log_window).pack(side=tk.RIGHT, padx=5)
tk.Button(top_frame, text="Info",           command=open_info_window).pack(side=tk.RIGHT, padx=5)
#tk.Button(top_frame, text="Read Registers", command=read_regs).pack(side=tk.RIGHT, padx=5)
tk.Button(top_frame, text="Innitial config", command=Innitial_Config).pack(side=tk.RIGHT, padx=5)
#tk.Button(top_frame, text="Calibrate PLL", command=Calibrate_PLL).pack(side=tk.RIGHT, padx=5)
tk.Button(top_frame, text="Impedance Calculator", command=Impedance_Wave).pack(side=tk.RIGHT, padx=5)

q1_var                = tk.BooleanVar()
q2_var                = tk.BooleanVar()
q1_single_impulse_var = tk.BooleanVar()
q2_single_impulse_var = tk.BooleanVar()


def update_output():
    global Output_en
    Output_en = 0
    if q1_var.get():
        Output_en |= 0b0100
    if q2_var.get():
        Output_en |= 0b1000
    if Output_en == 0:
        add_log("No outputs selected")


instructions_frame = tk.LabelFrame(root, text="How to", padx=10, pady=8)
instructions_frame.pack(fill=tk.X, padx=10, pady=(0, 8))

instruction_text = (
    "1) Program Raspberry Pi Pico in the \"Pico control\" panel - required only first time\n"
    "2) Connect with Pico by clicking the \"Connect\" button\n"
    "3) Click the \"Initial configuration\" button - required after every power restart\n"
    "4) Choose outputs in the Q1 or Q2 configuration menu\n"
    "5) Start sending impulse by clicking the \"Send impulse\" button\n"
)

tk.Label(
    instructions_frame,
    text=instruction_text,
    justify=tk.LEFT,
    anchor="w",
    wraplength=560,
    font=("Arial", 10),
).pack(anchor="w")

# Container for config frames (side-by-side)
config_container = tk.Frame(root)
config_container.pack(fill=tk.BOTH, expand=True, padx=0, pady=0)

# ── Q1 Configuration Frame ───────────────────────────────────────────────────
q1_config_frame = tk.Frame(config_container, relief=tk.SUNKEN, borderwidth=1)
q1_config_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5, pady=5)

tk.Label(q1_config_frame, text="Q1 Configuration", font=("Arial", 10, "bold")).pack(pady=5)

q1_impulse_frame = tk.Frame(q1_config_frame)
q1_impulse_frame.pack(fill=tk.X, padx=5, pady=5)
tk.Checkbutton(q1_impulse_frame, text="Single impulse", variable=q1_single_impulse_var).pack(anchor="w", padx=5)
tk.Checkbutton(q1_impulse_frame, text="Enable Q1",      variable=q1_var, command=update_output).pack(anchor="w", padx=5)

# ── Q2 Configuration Frame ───────────────────────────────────────────────────
q2_config_frame = tk.Frame(config_container, relief=tk.SUNKEN, borderwidth=1)
q2_config_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=5, pady=5)

tk.Label(q2_config_frame, text="Q2 Configuration", font=("Arial", 10, "bold")).pack(pady=5)

q2_impulse_frame = tk.Frame(q2_config_frame)
q2_impulse_frame.pack(fill=tk.X, padx=5, pady=5)
tk.Checkbutton(q2_impulse_frame, text="Single impulse", variable=q2_single_impulse_var).pack(anchor="w", padx=5)
tk.Checkbutton(q2_impulse_frame, text="Enable Q2",      variable=q2_var, command=update_output).pack(anchor="w", padx=5)

# ── Buttons ──────────────────────────────────────────────────────────────────
buttons_frame = tk.Frame(root)
buttons_frame.pack(fill=tk.X, padx=10, pady=10)

tk.Button(buttons_frame, text="Send impulse",        command=send_impulse,       width=15).pack(pady=5)
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