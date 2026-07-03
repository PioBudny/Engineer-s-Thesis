import tkinter as tk
import csv
from tkinter import filedialog, messagebox


def Impedance_Wave(root, close_impedance_window):
    root.update_idletasks()
    root_x = root.winfo_x()
    root_y = root.winfo_y()
    root_w = root.winfo_width()

    impedance_window = tk.Toplevel(root)
    impedance_window.title("Impedance Wave")
    impedance_window.geometry(f"1100x620+{root_x + root_w + 10}+{root_y}")
    impedance_window.protocol("WM_DELETE_WINDOW", close_impedance_window)

    # ────────────────────────────────────────────────────────────────
    # Górny pasek
    # ────────────────────────────────────────────────────────────────

    top_frame = tk.Frame(impedance_window)
    top_frame.pack(fill=tk.X, padx=10, pady=8)

    csv_path = tk.StringVar()
    z0_var = tk.StringVar(value="50")
    vf_var = tk.StringVar(value="0.66")

    tk.Label(top_frame, text="CSV file:").pack(side=tk.LEFT)

    tk.Entry(
        top_frame,
        textvariable=csv_path
    ).pack(side=tk.LEFT, fill=tk.X, expand=True, padx=5)

    def browse_csv():
        filename = filedialog.askopenfilename(
            title="Select CSV file",
            filetypes=[
                ("CSV files", "*.csv"),
                ("All files", "*.*")
            ]
        )

        if filename:
            csv_path.set(filename)

    settings_frame = tk.Frame(impedance_window)
    settings_frame.pack(fill=tk.X, padx=10, pady=(0, 8))

    tk.Label(settings_frame, text="Z0 (Ohm):").pack(side=tk.LEFT)
    tk.Entry(settings_frame, textvariable=z0_var, width=8).pack(side=tk.LEFT, padx=(4, 12))

    tk.Label(settings_frame, text="VF:").pack(side=tk.LEFT)
    tk.Entry(settings_frame, textvariable=vf_var, width=8).pack(side=tk.LEFT, padx=(4, 12))

    variables_text = (
        "Z0 - reference impedance, VF - velocity factor, "
        "Voffset - average before impulse, Vcorr - V - Voffset, "
        "Vimpuls - incident pulse amplitude, Gamma - reflection coefficient, "
        "d - distance"
    )
    tk.Label(
        settings_frame,
        text=variables_text,
        anchor="w",
        justify=tk.LEFT,
        wraplength=520
    ).pack(side=tk.LEFT, fill=tk.X, expand=True)

    # ────────────────────────────────────────────────────────────────
    # Wykres
    # ────────────────────────────────────────────────────────────────

    content_frame = tk.Frame(impedance_window)
    content_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=(0, 10))

    charts_frame = tk.Frame(content_frame)
    charts_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True)

    raw_chart_frame = tk.Frame(charts_frame, relief=tk.SUNKEN, borderwidth=1)
    raw_chart_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(0, 5))

    impedance_chart_frame = tk.Frame(charts_frame, relief=tk.SUNKEN, borderwidth=1)
    impedance_chart_frame.pack(side=tk.LEFT, fill=tk.BOTH, expand=True, padx=(5, 0))

    raw_canvas = tk.Canvas(raw_chart_frame, bg="white")
    raw_canvas.pack(fill=tk.BOTH, expand=True)

    impedance_canvas = tk.Canvas(impedance_chart_frame, bg="white")
    impedance_canvas.pack(fill=tk.BOTH, expand=True)

    math_frame = tk.Frame(content_frame, width=250)
    math_frame.pack(side=tk.RIGHT, fill=tk.Y, padx=(10, 0))
    math_frame.pack_propagate(False)

    tk.Label(
        math_frame,
        text="Math",
        font=("Arial", 10, "bold"),
        anchor="w"
    ).pack(fill=tk.X, pady=(0, 6))

    math_canvas = tk.Canvas(math_frame, width=240, height=350, bg="white", highlightthickness=0)
    math_canvas.pack(fill=tk.X)

    def draw_fraction(canvas, x, y, numerator, denominator, prefix="", suffix=""):
        canvas.create_text(x, y + 12, text=prefix, anchor="w", font=("Arial", 10))
        frac_x = x + 70
        canvas.create_text(frac_x, y, text=numerator, anchor="center", font=("Arial", 10))
        canvas.create_line(frac_x - 48, y + 12, frac_x + 48, y + 12, width=1)
        canvas.create_text(frac_x, y + 26, text=denominator, anchor="center", font=("Arial", 10))
        canvas.create_text(frac_x + 56, y + 12, text=suffix, anchor="w", font=("Arial", 10))

    def draw_math_formulas():
        math_canvas.delete("all")
        draw_fraction(math_canvas, 2, 8, "1", "N", "Voffset =", "* sum(Vpre)")
        math_canvas.create_text(
            2,
            58,
            text="Vcorr = V - Voffset",
            anchor="w",
            font=("Arial", 10)
        )
        math_canvas.create_text(
            2,
            84,
            text="Vimpuls = max(Vcorr)",
            anchor="w",
            font=("Arial", 10)
        )
        draw_fraction(math_canvas, 2, 112, "Vcorr", "Vimpuls", "Gamma =")
        math_canvas.create_text(
            2,
            156,
            text="-0.99 <= Gamma <= 0.99",
            anchor="w",
            font=("Arial", 10)
        )
        draw_fraction(math_canvas, 2, 184, "1 + Gamma", "1 - Gamma", "Z = Z0 *")
        draw_fraction(math_canvas, 2, 238, "(t - t0) * c * VF", "2", "d =")
        math_canvas.create_text(
            2,
            286,
            text="Peeling: remove earlier layer reflections",
            anchor="w",
            font=("Arial", 9)
        )
        math_canvas.create_text(2, 326, text="Gamma_local =", anchor="w", font=("Arial", 10))
        math_canvas.create_text(170, 312, text="Gamma_meas", anchor="center", font=("Arial", 10))
        math_canvas.create_line(112, 326, 228, 326, width=1)
        math_canvas.create_text(170, 340, text="prod(1 - Gamma_prev^2)", anchor="center", font=("Arial", 9))

    math_canvas.after(50, draw_math_formulas)

    raw_data = {
        "x_values": [],
        "y_values": [],
        "x_label": "Time (s)",
        "y_label": "Voltage (V)",
        "title": "Raw CSV data"
    }

    impedance_data = {
        "x_values": [],
        "y_values": [],
        "x_label": "Distance (m)",
        "y_label": "Impedance (Ohm)",
        "title": "Charakterystyka impedancji od odleglosci"
    }

    def draw_chart(canvas, chart_data, x_values, y_values, title,
                   x_label, y_label, empty_text):
        chart_data["x_values"] = x_values
        chart_data["y_values"] = y_values
        chart_data["title"] = title
        chart_data["x_label"] = x_label
        chart_data["y_label"] = y_label

        canvas.delete("all")
        canvas.update_idletasks()

        W = canvas.winfo_width()
        H = canvas.winfo_height()

        if W < 10 or H < 10:
            return

        pad_l, pad_r, pad_t, pad_b = 60, 20, 30, 50

        if not x_values or not y_values:
            canvas.create_text(
                W // 2,
                H // 2,
                text=empty_text,
                fill="gray",
                font=("Arial", 12)
            )
            return

        min_x = min(x_values)
        max_x = max(x_values)

        min_y = min(y_values)
        max_y = max(y_values)

        if min_x == max_x:
            min_x -= 1
            max_x += 1

        margin_y = (max_y - min_y) * 0.1 or 5

        min_y -= margin_y
        max_y += margin_y

        def tx(x):
            return pad_l + (x - min_x) / (max_x - min_x) * (W - pad_l - pad_r)

        def ty(y):
            return pad_t + (max_y - y) / (max_y - min_y) * (H - pad_t - pad_b)

        def format_axis_value(value):
            abs_value = abs(value)

            if value == 0:
                return "0"

            if abs_value < 0.01 or abs_value >= 10000:
                return f"{value:.2e}"

            return f"{value:.2f}"

        canvas.create_text(
            W // 2,
            14,
            text=title,
            font=("Arial", 10, "bold")
        )

        canvas.create_line(
            pad_l,
            pad_t,
            pad_l,
            H - pad_b,
            width=2
        )

        canvas.create_line(
            pad_l,
            H - pad_b,
            W - pad_r,
            H - pad_b,
            width=2
        )

        canvas.create_text(
            W // 2,
            H - 12,
            text=x_label
        )

        canvas.create_text(
            14,
            H // 2,
            text=y_label,
            angle=90
        )

        steps = 5

        for i in range(steps + 1):
            y_val = min_y + i * (max_y - min_y) / steps
            y_pix = ty(y_val)

            canvas.create_line(
                pad_l - 4,
                y_pix,
                pad_l,
                y_pix
            )

            canvas.create_text(
                pad_l - 8,
                y_pix,
                text=format_axis_value(y_val),
                anchor="e",
                font=("Arial", 8)
            )

            canvas.create_line(
                pad_l,
                y_pix,
                W - pad_r,
                y_pix,
                fill="#e0e0e0",
                dash=(2, 4)
            )

        for i in range(steps + 1):
            x_val = min_x + i * (max_x - min_x) / steps
            x_pix = tx(x_val)

            canvas.create_line(
                x_pix,
                H - pad_b,
                x_pix,
                H - pad_b + 4
            )

            canvas.create_text(
                x_pix,
                H - pad_b + 14,
                text=format_axis_value(x_val),
                font=("Arial", 8)
            )

        points = []

        for x, y in zip(x_values, y_values):
            points.extend([tx(x), ty(y)])

        if len(points) >= 4:
            canvas.create_line(
                points,
                fill="#2266cc",
                width=2,
                smooth=True
            )

    def load_csv_data(filename):
        with open(filename, newline="") as csv_file:
            rows = list(csv.reader(csv_file))

        if len(rows) < 3:
            raise ValueError("CSV file has too few rows.")

        first_row = [cell.strip() for cell in rows[0]]
        second_row = [cell.strip() for cell in rows[1]]

        signal_name = first_row[1] if len(first_row) > 1 and first_row[1] else "Signal"
        x_label = second_row[0] if second_row and second_row[0] else "Sequence"
        y_label = second_row[1] if len(second_row) > 1 and second_row[1] else "Value"

        try:
            start_index = first_row.index("Start")
            increment_index = first_row.index("Increment")
            start = float(second_row[start_index])
            increment = float(second_row[increment_index])
            use_generated_x = True
            x_label = "Time (s)"
        except (ValueError, IndexError):
            start = 0.0
            increment = 1.0
            use_generated_x = False

        x_values = []
        y_values = []

        for row in rows[2:]:
            if len(row) < 2:
                continue

            try:
                sequence = float(row[0])
                value = float(row[1])
            except ValueError:
                continue

            x_values.append(start + sequence * increment if use_generated_x else sequence)
            y_values.append(value)

        if not x_values:
            raise ValueError("CSV file does not contain numeric data.")

        return x_values, y_values, f"{signal_name} vs {x_label}", x_label, y_label

    def get_loaded_csv():
        filename = csv_path.get()

        if not filename:
            messagebox.showwarning("No CSV selected", "Select a CSV file first.")
            return None

        print("Selected:", filename)

        try:
            return load_csv_data(filename)
        except (OSError, ValueError) as e:
            messagebox.showerror("CSV error", str(e))
            return None

    def on_load_csv():
        loaded = get_loaded_csv()
        if not loaded:
            return

        x_values, y_values, title, x_label, y_label = loaded
        draw_chart(
            raw_canvas,
            raw_data,
            x_values,
            y_values,
            title,
            x_label,
            y_label,
            "Press Load .csv"
        )

    def prepare_tdr_data(times, voltages):
        c = 299792458.0

        try:
            z0 = float(z0_var.get())
            vf = float(vf_var.get())
        except ValueError:
            raise ValueError("Z0 and VF must be numeric.")

        if z0 <= 0:
            raise ValueError("Z0 must be greater than zero.")

        if vf > 1:
            vf /= 100

        if not (0 < vf <= 1):
            raise ValueError("VF must be between 0 and 1, or percent value like 66.")

        if len(times) < 2:
            raise ValueError("CSV file must contain at least two samples.")

        def average(values):
            return sum(values) / len(values)

        def find_impulse_index():
            if min(times) < 0 < max(times):
                for i, t in enumerate(times):
                    if t >= 0:
                        return i

            initial_count = max(3, int(len(voltages) * 0.1))
            initial_offset = average(voltages[:initial_count])
            initial_noise = average([abs(v - initial_offset) for v in voltages[:initial_count]])
            corrected_preview = [v - initial_offset for v in voltages]
            peak = max(abs(v) for v in corrected_preview)
            threshold = max(initial_noise * 6, peak * 0.1, 1e-12)

            for i, value in enumerate(corrected_preview):
                if abs(value) >= threshold:
                    return i

            return initial_count

        impulse_index = find_impulse_index()

        if impulse_index <= 0:
            offset_count = max(1, int(len(voltages) * 0.05))
        else:
            offset_count = impulse_index

        voltage_offset = average(voltages[:offset_count])
        corrected_voltages = [v - voltage_offset for v in voltages]
        skip_samples = max(10, int(len(voltages) * 0.01))
        calculation_start = min(impulse_index + skip_samples, len(voltages) - 1)
        impulse_samples = corrected_voltages[calculation_start:]

        if not impulse_samples:
            raise ValueError("Could not find samples after impulse start.")

        incident_voltage = max(impulse_samples)

        if abs(incident_voltage) < 1e-12:
            raise ValueError("Incident pulse amplitude is too close to zero.")

        if incident_voltage < 0:
            raise ValueError("Incident pulse amplitude must be positive after offset removal.")

        return {
            "c": c,
            "z0": z0,
            "vf": vf,
            "impulse_index": impulse_index,
            "calculation_start": calculation_start,
            "voltage_offset": voltage_offset,
            "corrected_voltages": corrected_voltages,
            "incident_voltage": incident_voltage,
            "t0": times[impulse_index]
        }

    def calculate_impedance_vs_distance(times, voltages):
        data = prepare_tdr_data(times, voltages)
        c = data["c"]
        z0 = data["z0"]
        vf = data["vf"]
        calculation_start = data["calculation_start"]
        corrected_voltages = data["corrected_voltages"]
        incident_voltage = data["incident_voltage"]
        t0 = data["t0"]

        distances = []
        impedances = []

        for t, voltage in zip(times[calculation_start:], corrected_voltages[calculation_start:]):
            gamma = voltage / incident_voltage
            gamma = max(min(gamma, 0.99), -0.99)
            denominator = 1 - gamma

            if abs(denominator) < 1e-12:
                continue

            distance = (t - t0) * c * vf / 2
            impedance = z0 * (1 + gamma) / denominator

            distances.append(distance)
            impedances.append(impedance)

        if not distances:
            raise ValueError("Could not calculate impedance from this CSV data.")

        return distances, impedances, data["voltage_offset"], incident_voltage

    def calculate_impedance_layer_stripping(times, voltages):
        data = prepare_tdr_data(times, voltages)
        c = data["c"]
        z0 = data["z0"]
        vf = data["vf"]
        calculation_start = data["calculation_start"]
        corrected_voltages = data["corrected_voltages"]
        incident_voltage = data["incident_voltage"]
        t0 = data["t0"]

        distances = []
        impedances = []
        transmission_product = 1.0

        for t, voltage in zip(times[calculation_start:], corrected_voltages[calculation_start:]):
            measured_gamma = voltage / incident_voltage

            if abs(transmission_product) < 1e-6:
                break

            local_gamma = measured_gamma / transmission_product
            local_gamma = max(min(local_gamma, 0.99), -0.99)

            distance = (t - t0) * c * vf / 2
            impedance = z0 * (1 + local_gamma) / (1 - local_gamma)

            distances.append(distance)
            impedances.append(impedance)

            transmission_product *= max(1 - local_gamma * local_gamma, 1e-6)

        if not distances:
            raise ValueError("Could not calculate layer stripping impedance from this CSV data.")

        return distances, impedances

    def on_calculate_zd():
        loaded = get_loaded_csv()
        if not loaded:
            return

        times, voltages, _, _, _ = loaded

        try:
            distances, impedances, _, _ = calculate_impedance_vs_distance(times, voltages)
        except ValueError as e:
            messagebox.showerror("Calculation error", str(e))
            return

        title = "Charakterystyka impedancji od odleglosci"
        draw_chart(
            impedance_canvas,
            impedance_data,
            distances,
            impedances,
            title,
            "Distance (m)",
            "Impedance (Ohm)",
            "Press Calculate Z(d)"
        )

    def on_calculate_peeling_zd():
        loaded = get_loaded_csv()
        if not loaded:
            return

        times, voltages, _, _, _ = loaded

        try:
            distances, impedances = calculate_impedance_layer_stripping(times, voltages)
        except ValueError as e:
            messagebox.showerror("Calculation error", str(e))
            return

        title = "Charakterystyka impedancji od odleglosci - peeling"
        draw_chart(
            impedance_canvas,
            impedance_data,
            distances,
            impedances,
            title,
            "Distance (m)",
            "Impedance (Ohm)",
            "Press Peeling Z(d)"
        )

    tk.Button(
        top_frame,
        text="Browse",
        width=10,
        command=browse_csv
    ).pack(side=tk.LEFT, padx=5)

    tk.Button(
        top_frame,
        text="Load .csv",
        width=10,
        command=on_load_csv
    ).pack(side=tk.LEFT, padx=(0, 5))

    tk.Button(
        top_frame,
        text="Calculate Z(d)",
        width=14,
        command=on_calculate_zd
    ).pack(side=tk.LEFT, padx=(0, 5))

    tk.Button(
        top_frame,
        text="Peeling Z(d)",
        width=14,
        command=on_calculate_peeling_zd
    ).pack(side=tk.LEFT)

    impedance_window.after(
        50,
        lambda: (
            draw_chart(
                raw_canvas,
                raw_data,
                [],
                [],
                raw_data["title"],
                raw_data["x_label"],
                raw_data["y_label"],
                "Press Load .csv"
            ),
            draw_chart(
                impedance_canvas,
                impedance_data,
                [],
                [],
                impedance_data["title"],
                impedance_data["x_label"],
                impedance_data["y_label"],
                "Press Calculate Z(d)"
            )
        )
    )

    impedance_window.bind(
        "<Configure>",
        lambda e: raw_canvas.after(
            10,
            lambda: (
                draw_chart(
                    raw_canvas,
                    raw_data,
                    raw_data["x_values"],
                    raw_data["y_values"],
                    raw_data["title"],
                    raw_data["x_label"],
                    raw_data["y_label"],
                    "Press Load .csv"
                ),
                draw_chart(
                    impedance_canvas,
                    impedance_data,
                    impedance_data["x_values"],
                    impedance_data["y_values"],
                    impedance_data["title"],
                    impedance_data["x_label"],
                    impedance_data["y_label"],
                    "Press Calculate Z(d)"
                )
            )
        )
    )

    return impedance_window
