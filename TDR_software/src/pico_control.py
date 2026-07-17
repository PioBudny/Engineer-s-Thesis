import os
import shutil
import subprocess
import sys
import threading
import tkinter as tk
from pathlib import Path
from tkinter import filedialog


_pico_control_window = None


def _is_frozen():
    """True when running from a PyInstaller (or similar) packaged executable."""
    return bool(getattr(sys, "frozen", False))


# This file lives in src/, while CMakeLists.txt / build/ are one level up.
# Only meaningful in dev mode - a packaged exe has no CMakeLists.txt/build/ tree.
PROJECT_ROOT = Path(__file__).resolve().parent.parent
BUILD_DIR = PROJECT_ROOT / "build"
UF2_NAME = "TDR_software.uf2"


def _exe_name(base):
    """Windows binaries have a .exe extension, Linux/macOS ones don't."""
    return base + ".exe" if os.name == "nt" else base


# Local Pico SDK install (Raspberry Pi Pico VS Code Extension) - same
# directory layout on Windows, Linux and macOS.
PICO_SDK_HOME = Path.home() / ".pico-sdk"
PICO_SDK_CMAKE = PICO_SDK_HOME / "cmake" / "v3.31.5" / "bin" / _exe_name("cmake")
PICO_SDK_NINJA_DIR = PICO_SDK_HOME / "ninja" / "v1.12.1"
PICO_SDK_TOOLCHAIN_DIR = PICO_SDK_HOME / "toolchain" / "14_2_Rel1"
PICO_SDK_TOOLCHAIN_BIN = PICO_SDK_TOOLCHAIN_DIR / "bin"
PICO_SDK_PICOTOOL_DIR = PICO_SDK_HOME / "picotool" / "2.2.0-a4" / "picotool"
PICO_SDK_PICOTOOL_EXE = PICO_SDK_PICOTOOL_DIR / _exe_name("picotool")
PICO_SDK_SDK_DIR = PICO_SDK_HOME / "sdk" / "2.2.0"

# Remembers the last selected drive path between window openings (within a session)
_selected_drive_path = ""
_building = False
_flashing = False


def get_uf2_path():
    """Where to look for the firmware. In a packaged app there is no
    build/ tree, so the .uf2 is expected right next to the executable
    (ship it there manually). In dev mode it's build/TDR_software.uf2,
    next to CMakeLists.txt, where 'Package' writes it."""
    if _is_frozen():
        return Path(sys.executable).resolve().parent / UF2_NAME
    return BUILD_DIR / UF2_NAME


def close_pico_control_window():
    global _pico_control_window
    if _pico_control_window:
        _pico_control_window.destroy()
        _pico_control_window = None


def browse_drive_path(path_var, log_action=None):

    global _selected_drive_path
    folder = filedialog.askdirectory(title="Select RPI-RP2 drive (Pico in BOOTSEL mode)")
    if folder:
        _selected_drive_path = folder
        path_var.set(folder)
        if log_action:
            log_action(f"Selected drive: {folder}")


def flash_pico(uf2_path, drive_path, log_action=None):


    def log(message):
        if log_action:
            log_action(message)

    if not uf2_path or not os.path.exists(uf2_path):
        log(f"Firmware file not found: {uf2_path}")
        return

    if not drive_path or not os.path.isdir(drive_path):
        log(f"Invalid Pico drive path: {drive_path}")
        return

    try:
        dest = os.path.join(drive_path, os.path.basename(uf2_path))
        shutil.copy(uf2_path, dest)
        log(f"Firmware uploaded ({dest}). Pico will restart shortly.")
    except Exception as e:
        log(f"Upload error: {e}")
        log("Copy the .uf2 file manually to the RPI-RP2 drive.")


def handle_program_pico(drive_path, log_action=None):
    uf2_path = get_uf2_path()
    if not uf2_path.exists():
        if log_action:
            log_action(f"Firmware not found ({uf2_path}). Click 'Package' first.")
        return
    flash_pico(str(uf2_path), drive_path, log_action)


def _resolve_cmake_exe():
    if PICO_SDK_CMAKE.exists():
        return str(PICO_SDK_CMAKE)
    system_cmake = shutil.which("cmake")
    if system_cmake:
        return system_cmake
    return None


def _resolve_picotool_exe():
    if PICO_SDK_PICOTOOL_EXE.exists():
        return str(PICO_SDK_PICOTOOL_EXE)
    system_picotool = shutil.which("picotool")
    if system_picotool:
        return system_picotool
    return None


def _build_env():
    env = os.environ.copy()
    if PICO_SDK_SDK_DIR.exists():
        env["PICO_SDK_PATH"] = str(PICO_SDK_SDK_DIR)
    if PICO_SDK_TOOLCHAIN_DIR.exists():
        env["PICO_TOOLCHAIN_PATH"] = str(PICO_SDK_TOOLCHAIN_DIR)

    extra_path_dirs = [
        PICO_SDK_TOOLCHAIN_BIN,
        PICO_SDK_NINJA_DIR,
        PICO_SDK_PICOTOOL_DIR,
    ]
    existing_extra = [str(p) for p in extra_path_dirs if p.exists()]
    if existing_extra:
        env["PATH"] = os.pathsep.join(existing_extra) + os.pathsep + env.get("PATH", "")
    return env


def _run_build_step(cmake_exe, args, cwd, env, log):
    process = subprocess.Popen(
        [cmake_exe, *args],
        cwd=cwd,
        env=env,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
        text=True,
        bufsize=1,
    )
    for line in process.stdout:
        line = line.rstrip()
        if line:
            log(line)
    process.wait()
    return process.returncode


def build_firmware(log_action=None, on_complete=None, on_error=None, root=None):

    def log(message):
        if log_action:
            log_action(message)

    def notify(callback, *args):
        if not callback:
            return
        if root:
            root.after(0, lambda: callback(*args))
        else:
            callback(*args)

    def worker():
        if _is_frozen():
            log("Building is not available in the packaged app (no .c sources/toolchain "
                f"bundled). Ship a prebuilt {UF2_NAME} next to the executable instead.")
            notify(on_error, "frozen")
            return

        cmake_exe = _resolve_cmake_exe()
        if not cmake_exe:
            log("CMake not found. Install the Pico SDK (VS Code extension) or CMake.")
            notify(on_error, "CMake not found")
            return

        env = _build_env()
        cmake_cache = BUILD_DIR / "CMakeCache.txt"

        if not cmake_cache.exists():
            log("Configuring CMake project (first run)...")
            BUILD_DIR.mkdir(parents=True, exist_ok=True)
            rc = _run_build_step(
                cmake_exe,
                ["-G", "Ninja", "-S", str(PROJECT_ROOT), "-B", str(BUILD_DIR), "-DPICO_BOARD=pico2"],
                PROJECT_ROOT,
                env,
                log,
            )
            if rc != 0:
                log(f"CMake configuration failed (code {rc}).")
                notify(on_error, "configure failed")
                return

        log("Building firmware...")
        rc = _run_build_step(cmake_exe, ["--build", str(BUILD_DIR)], PROJECT_ROOT, env, log)
        if rc != 0:
            log(f"Build failed (code {rc}).")
            notify(on_error, "build failed")
            return

        uf2_path = BUILD_DIR / UF2_NAME
        if not uf2_path.exists():
            log(f"Build finished, but file {uf2_path} was not found.")
            notify(on_error, "uf2 not found")
            return

        log(f"Firmware ready: {uf2_path}")
        notify(on_complete, str(uf2_path))

    threading.Thread(target=worker, daemon=True).start()


def handle_package(package_button, log_action=None, root=None):
    global _building

    if _building:
        return

    _building = True
    package_button.config(state="disabled")

    def finish():
        global _building
        _building = False
        package_button.config(state="normal")

    def on_complete(_uf2_path):
        finish()

    def on_error(_reason):
        finish()

    build_firmware(
        log_action=log_action,
        on_complete=on_complete,
        on_error=on_error,
        root=root,
    )


def flash_with_picotool(picotool_button, log_action=None, root=None):
    global _flashing

    def log(message):
        if log_action:
            log_action(message)

    if _flashing:
        return

    uf2_path = get_uf2_path()
    if not uf2_path.exists():
        log(f"Firmware not found ({uf2_path}). Click 'Package' first.")
        return

    picotool_exe = _resolve_picotool_exe()
    if not picotool_exe:
        log("picotool not found.")
        return

    _flashing = True
    picotool_button.config(state="disabled")

    def notify_done():
        global _flashing
        _flashing = False
        picotool_button.config(state="normal")

    def finish():
        if root:
            root.after(0, notify_done)
        else:
            notify_done()

    def worker():
        env = _build_env()
        try:
            process = subprocess.Popen(
                [picotool_exe, "load", "-f", "-x", str(uf2_path)],
                env=env,
                stdout=subprocess.PIPE,
                stderr=subprocess.STDOUT,
                text=True,
                bufsize=1,
            )
            for line in process.stdout:
                line = line.rstrip()
                if line:
                    log(line)
            process.wait()
        except Exception as e:
            log(f"Error running picotool: {e}")
            finish()
            return

        if process.returncode == 0:
            log("Firmware uploaded via picotool. Pico will restart shortly.")
        else:
            log(f"picotool exited with an error (code {process.returncode}).")
            if os.name != "nt":
                log("On Linux, USB permissions may be missing - check udev rules for picotool.")

        finish()

    threading.Thread(target=worker, daemon=True).start()


def open_pico_control_window(root, log_action=None):
    global _pico_control_window
    if _pico_control_window and tk.Toplevel.winfo_exists(_pico_control_window):
        _pico_control_window.lift()
        return

    _pico_control_window = tk.Toplevel(root)
    _pico_control_window.title("Pico control")
    _pico_control_window.geometry("440x420")

    main_frame = tk.Frame(_pico_control_window, padx=15, pady=15)
    main_frame.pack(fill=tk.BOTH, expand=True)

    # ── Manual (copy the .uf2 file to the RPI-RP2 drive) ──────────────
    manual_frame = tk.LabelFrame(main_frame, text="Manual", padx=10, pady=10)
    manual_frame.pack(fill=tk.X, pady=(0, 10))

    tk.Label(manual_frame, text="1. Connect Pico in BOOTSEL mode", anchor="w").pack(fill=tk.X)

    tk.Label(manual_frame, text="2. Pico drive (RPI-RP2):", anchor="w").pack(fill=tk.X, pady=(8, 0))
    drive_path_var = tk.StringVar(value=_selected_drive_path)
    drive_frame = tk.Frame(manual_frame)
    drive_frame.pack(fill=tk.X, pady=(0, 5))
    tk.Entry(drive_frame, textvariable=drive_path_var).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=(0, 5))
    tk.Button(drive_frame, text="Browse...", command=lambda: browse_drive_path(drive_path_var, log_action)).pack(
        side=tk.RIGHT
    )

    manual_action_frame = tk.Frame(manual_frame)
    manual_action_frame.pack(fill=tk.X, pady=(8, 0))
    tk.Button(
        manual_action_frame,
        text="Program pico",
        width=14,
        command=lambda: handle_program_pico(drive_path_var.get(), log_action),
    ).pack(side=tk.LEFT)
  #  package_button = tk.Button(manual_action_frame, text="Package", width=14)
   # package_button.config(command=lambda: handle_package(package_button, log_action, root))
    #package_button.pack(side=tk.LEFT, padx=(5, 0))

    tk.Label(
        manual_frame,
        text="3. If automatic copying doesn't work, copy the\nbuild/TDR_software.uf2 file manually to the\nRPI-RP2 drive.",
        fg="gray",
        justify=tk.LEFT,
        anchor="w",
    ).pack(fill=tk.X, pady=(8, 0))

    # ── picotool (programming with the picotool utility, over USB) ────
    picotool_frame = tk.LabelFrame(main_frame, text="picotool", padx=10, pady=10)
    picotool_frame.pack(fill=tk.X, pady=(0, 10))

    tk.Label(picotool_frame, text="1. Connect Pico.", anchor="w").pack(fill=tk.X)

    tk.Label(picotool_frame, text="2. Build the firmware with the 'Package' button above.", anchor="w").pack(
        fill=tk.X, pady=(4, 0)
    )

    picotool_action_frame = tk.Frame(picotool_frame)
    picotool_action_frame.pack(fill=tk.X, pady=(8, 0))
    picotool_button = tk.Button(picotool_action_frame, text="Program with Picotool", width=20)
    picotool_button.config(command=lambda: flash_with_picotool(picotool_button, log_action, root))
    picotool_button.pack(side=tk.LEFT)

    _pico_control_window.protocol("WM_DELETE_WINDOW", close_pico_control_window)
