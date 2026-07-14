import tkinter as tk
import csv
import bisect
from tkinter import filedialog, messagebox
from tkinter import ttk


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

    csv_frame = tk.Frame(top_frame)
    csv_frame.pack(side=tk.LEFT, fill=tk.X, expand=True)

    button_frame = tk.Frame(top_frame)
    button_frame.pack(side=tk.RIGHT)

    csv_path = tk.StringVar()
    z0_var = tk.StringVar(value="50")
    param_type_var = tk.StringVar(value="VF")  # "VF" lub "ER"
    param_value_var = tk.StringVar(value="")

    tk.Label(csv_frame, text="CSV file:").pack(side=tk.LEFT)

    tk.Entry(
        csv_frame,
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

    # Wybór między ER a VF z listy
    tk.Label(settings_frame, text="Parameter:").pack(side=tk.LEFT, padx=(12, 4))

    def on_param_type_change(event=None):
        param_value_var.set("")

    param_combo = ttk.Combobox(
        settings_frame,
        textvariable=param_type_var,
        values=["VF", "ER"],
        state="readonly",
        width=6
    )
    param_combo.pack(side=tk.LEFT, padx=(0, 8))
    param_combo.bind("<<ComboboxSelected>>", on_param_type_change)

    tk.Entry(settings_frame, textvariable=param_value_var, width=8).pack(side=tk.LEFT, padx=(0, 12))

    variables_text = (
        "Z0 - reference impedance, VF - velocity factor (0-1), ER - relative permittivity (>1)"
    )
    tk.Label(
        settings_frame,
        text=variables_text,
        anchor="w",
        justify=tk.LEFT,
        wraplength=520
    ).pack(side=tk.LEFT, fill=tk.X, expand=True)

    # ────────────────────────────────────────────────────────────────
    # Info o obliczeniach (info_var jest ustawiane w on_calculate_zd)
    # ────────────────────────────────────────────────────────────────

    info_var = tk.StringVar(value="Brak obliczeń — wciśnij Calculate Z(d)")

    info_label = tk.Label(
        impedance_window,
        textvariable=info_var,
        anchor="w",
        justify=tk.LEFT,
        font=("Consolas", 9)
    )
    info_label.pack(fill=tk.X, padx=10, pady=(0, 8))

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

    raw_data = {
        "x_values": [],
        "y_values": [],
        "x_label": "Time (s)",
        "y_label": "Voltage (V)",
        "title": "Raw CSV data",
        "x_min": None,
        "x_max": None,
        "y_min": None,
        "y_max": None,
        "pan_start": None,
        "redraw_job": None
    }

    impedance_data = {
        "x_values": [],
        "y_values": [],
        "x_label": "Distance (m)",
        "y_label": "Impedance (Ohm)",
        "title": "Charakterystyka impedancji od odleglosci",
        "x_min": None,
        "x_max": None,
        "y_min": None,
        "y_max": None,
        "pan_start": None,
        "redraw_job": None
    }

    def bind_mouse_wheel(canvas, chart_data):
        canvas.bind("<MouseWheel>", lambda event: on_mouse_wheel(event, canvas, chart_data))
        canvas.bind("<Button-4>", lambda event: on_mouse_wheel(event, canvas, chart_data))
        canvas.bind("<Button-5>", lambda event: on_mouse_wheel(event, canvas, chart_data))
        canvas.bind("<ButtonPress-1>", lambda event: on_pan_start(event, canvas, chart_data))
        canvas.bind("<B1-Motion>", lambda event: on_pan_move(event, canvas, chart_data))
        canvas.bind("<ButtonRelease-1>", lambda event: on_pan_end(event, canvas, chart_data))

    bind_mouse_wheel(raw_canvas, raw_data)
    bind_mouse_wheel(impedance_canvas, impedance_data)

    PLOT_PAD_L, PLOT_PAD_R, PLOT_PAD_T, PLOT_PAD_B = 60, 20, 30, 50
    _NO_REFERENCE_LINE = object()

    def update_chart_bounds(chart_data, x_values, y_values, reference_line, reset_bounds=False):
        if reset_bounds or chart_data.get("x_min") is None:
            if not x_values or not y_values:
                chart_data["x_min"] = 0.0
                chart_data["x_max"] = 1.0
                if reference_line is not None:
                    chart_data["y_min"] = reference_line - max(abs(reference_line) * 0.1, 1.0)
                    chart_data["y_max"] = reference_line + max(abs(reference_line) * 0.1, 1.0)
                else:
                    chart_data["y_min"] = 0.0
                    chart_data["y_max"] = 1.0
                return

            min_x = min(x_values)
            max_x = max(x_values)
            min_y = min(y_values)
            max_y = max(y_values)

            if reference_line is not None:
                min_y = min(min_y, reference_line)
                max_y = max(max_y, reference_line)

            if min_x == max_x:
                min_x -= 1
                max_x += 1

            margin_y = (max_y - min_y) * 0.1 or 5
            min_y -= margin_y
            max_y += margin_y

            chart_data["x_min"] = min_x
            chart_data["x_max"] = max_x
            chart_data["y_min"] = min_y
            chart_data["y_max"] = max_y

    def draw_chart(canvas, chart_data, x_values, y_values, title,
                   x_label, y_label, empty_text,
                   reference_line=_NO_REFERENCE_LINE,
                   reference_line_label=None,
                   reset_bounds=False):
        if reference_line is _NO_REFERENCE_LINE:
            reference_line = chart_data.get("reference_line")
            reference_line_label = chart_data.get("reference_line_label")
        elif reference_line is None:
            chart_data.pop("reference_line", None)
            chart_data.pop("reference_line_label", None)
        else:
            chart_data["reference_line"] = reference_line
            chart_data["reference_line_label"] = reference_line_label
        chart_data["x_values"] = x_values
        chart_data["y_values"] = y_values
        chart_data["title"] = title
        chart_data["x_label"] = x_label
        chart_data["y_label"] = y_label

        update_chart_bounds(chart_data, x_values, y_values, reference_line, reset_bounds=reset_bounds)

        # WAŻNE: update_idletasks() musi być PRZED delete("all"). Wywołane po
        # czyszczeniu wymuszało natychmiastowe odrysowanie pustego (białego)
        # canvasu na ekranie, zanim dorysowana została nowa zawartość - stąd
        # widoczne "miganie" na biało przy każdym przerysowaniu.
        canvas.update_idletasks()
        canvas.delete("all")

        W = canvas.winfo_width()
        H = canvas.winfo_height()

        if W < 10 or H < 10:
            return

        pad_l, pad_r, pad_t, pad_b = PLOT_PAD_L, PLOT_PAD_R, PLOT_PAD_T, PLOT_PAD_B

        min_x = chart_data.get("x_min", 0.0)
        max_x = chart_data.get("x_max", 1.0)
        min_y = chart_data.get("y_min", 0.0)
        max_y = chart_data.get("y_max", 1.0)

        if (not x_values or not y_values) and reference_line is None:
            canvas.create_text(
                W // 2,
                H // 2,
                text=empty_text,
                fill="gray",
                font=("Arial", 12)
            )
            return

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

        # ── 1) Zawartość danych (linia pomiarowa, linia referencyjna) ──
        # Tag "content". Rysowana jako pierwsza (najniżej), dzięki czemu
        # fragmenty, które przy zoomie/panie wystają poza obszar wykresu,
        # zostaną w kolejnym kroku przykryte białymi maskami.
        if reference_line is not None:
            ref_y = ty(reference_line)
            # Tag "refline" (a nie "content"): to pozioma linia o stałej
            # szerokości (od pad_l do W-pad_r), więc przy przeciąganiu wykresu
            # wolno ją przesuwać tylko w pionie (zmiana wartości Y), nigdy w
            # poziomie - inaczej "ucieka" w bok i część wykresu zostaje bez niej.
            canvas.create_line(
                pad_l,
                ref_y,
                W - pad_r,
                ref_y,
                fill="#cc2222",
                dash=(4, 4),
                width=2,
                tags="refline"
            )

            label_text = reference_line_label or "Z0"
            canvas.create_text(
                W - pad_r - 10,
                ref_y - 10,
                text=f"{label_text} = {reference_line:.2f}",
                anchor="e",
                fill="#cc2222",
                font=("Arial", 8, "bold"),
                tags="refline"
            )

        # Aby wykres pozostal plynny nawet przy duzym przyblizeniu, rysujemy
        # tylko probki z aktualnie widocznego zakresu X (+ jedna probka
        # zapasu z kazdej strony, zeby linia byla ciagla na krawedzi), a nie
        # zawsze caly zbior danych. Zaklada rosnaco posortowane x_values
        # (tak sa tu zawsze wczytywane/liczone - czas albo odleglosc).
        if x_values:
            x_range = (max_x - min_x) or 1.0
            # Bufor: dodatkowa "szerokosc ekranu" danych z kazdej strony.
            # Dzieki temu podczas plynnego przeciagania (canvas.move) brzeg,
            # ktory dopiero wjezdza w kadr, jest juz narysowany, zanim
            # zdazy sie pelne przerysowanie (throttled) - inaczej widac
            # pusty/bialy pasek do momentu odswiezenia.
            margin = x_range
            start_idx = bisect.bisect_left(x_values, min_x - margin)
            end_idx = bisect.bisect_right(x_values, max_x + margin)
            visible_x = x_values[start_idx:end_idx]
            visible_y = y_values[start_idx:end_idx]
        else:
            visible_x = x_values
            visible_y = y_values

        points = []
        last_px = None

        for x, y in zip(visible_x, visible_y):
            px = tx(x)

            # Pomijamy probki, ktore i tak wypadlyby w tym samym pikselu -
            # przy malym przyblizeniu/gestych danych mocno redukuje liczbe
            # punktow przekazywanych do canvasa.
            if last_px is not None and abs(px - last_px) < 1.0:
                continue

            last_px = px
            points.extend([px, ty(y)])

        if len(points) >= 4:
            canvas.create_line(
                points,
                fill="#2266cc",
                width=2,
                smooth=True,
                tags="content"
            )

        # ── 2) Maski (białe prostokąty poza obszarem wykresu) ──
        # Tag "mask". Zakrywają wszystko, co narysowane w kroku 1, a leży
        # poza prostokątem [pad_l, W-pad_r] x [pad_t, H-pad_b].
        canvas.create_rectangle(0, 0, W, pad_t, fill="white", outline="", tags="mask")
        canvas.create_rectangle(0, H - pad_b, W, H, fill="white", outline="", tags="mask")
        canvas.create_rectangle(0, 0, pad_l, H, fill="white", outline="", tags="mask")
        canvas.create_rectangle(W - pad_r, 0, W, H, fill="white", outline="", tags="mask")

        # ── 3) Osie, siatka, opisy, tytuł ──
        # Tag "chrome". Rysowane na samej górze, więc zawsze widoczne
        # ponad maskami.
        canvas.create_text(
            W // 2,
            14,
            text=title,
            font=("Arial", 10, "bold"),
            tags="chrome"
        )

        canvas.create_line(
            pad_l,
            pad_t,
            pad_l,
            H - pad_b,
            width=2,
            tags="chrome"
        )

        canvas.create_line(
            pad_l,
            H - pad_b,
            W - pad_r,
            H - pad_b,
            width=2,
            tags="chrome"
        )

        canvas.create_text(
            W // 2,
            H - 12,
            text=x_label,
            tags="chrome"
        )

        canvas.create_text(
            14,
            H // 2,
            text=y_label,
            angle=90,
            tags="chrome"
        )

        steps = 5

        for i in range(steps + 1):
            y_val = min_y + i * (max_y - min_y) / steps
            y_pix = ty(y_val)

            canvas.create_line(
                pad_l - 4,
                y_pix,
                pad_l,
                y_pix,
                tags="chrome"
            )

            canvas.create_text(
                pad_l - 8,
                y_pix,
                text=format_axis_value(y_val),
                anchor="e",
                font=("Arial", 8),
                tags="chrome"
            )

            canvas.create_line(
                pad_l,
                y_pix,
                W - pad_r,
                y_pix,
                fill="#e0e0e0",
                dash=(2, 4),
                tags="chrome"
            )

        for i in range(steps + 1):
            x_val = min_x + i * (max_x - min_x) / steps
            x_pix = tx(x_val)

            canvas.create_line(
                x_pix,
                H - pad_b,
                x_pix,
                H - pad_b + 4,
                tags="chrome"
            )

            canvas.create_text(
                x_pix,
                H - pad_b + 14,
                text=format_axis_value(x_val),
                font=("Arial", 8),
                tags="chrome"
            )

    def redraw_from_state(canvas, chart_data, empty_text=""):
        """Przerysowuje wykres na podstawie danych już zapisanych w chart_data
        (bez konieczności ponownego przekazywania x/y/tytułu itd.)."""
        draw_chart(
            canvas,
            chart_data,
            chart_data["x_values"],
            chart_data["y_values"],
            chart_data["title"],
            chart_data["x_label"],
            chart_data["y_label"],
            empty_text,
            reference_line=chart_data.get("reference_line"),
            reference_line_label=chart_data.get("reference_line_label")
        )

    def schedule_redraw(canvas, chart_data, delay=30):
        """Odkłada pełne przerysowanie (z przeliczeniem siatki/osi) o kilka
        milisekund i anuluje poprzednio zaplanowane odświeżenie. Dzięki temu
        przy szybkich zdarzeniach (scroll kółkiem, przeciąganie) nie robimy
        pełnego canvas.delete('all') + redraw za każdym pojedynczym zdarzeniem,
        co wcześniej powodowało miganie na biało i spadki płynności."""
        job = chart_data.get("redraw_job")
        if job is not None:
            canvas.after_cancel(job)

        def _do_redraw():
            chart_data["redraw_job"] = None
            redraw_from_state(canvas, chart_data)

        chart_data["redraw_job"] = canvas.after(delay, _do_redraw)

    def show_point_at_pixel(canvas, chart_data, px, py):
        """Znajduje najblizszy punkt danych do klikniecia (px, py w pikselach
        canvasa) i rysuje przy nim znacznik ze wspolrzednymi (np. Distance /
        Impedance albo Time / Voltage - w zaleznosci od wykresu)."""
        W = canvas.winfo_width()
        H = canvas.winfo_height()
        pad_l, pad_r, pad_t, pad_b = PLOT_PAD_L, PLOT_PAD_R, PLOT_PAD_T, PLOT_PAD_B

        # Klikniecie poza samym obszarem wykresu (np. na opisie osi) - nic
        # nie pokazujemy.
        if not (pad_l <= px <= W - pad_r and pad_t <= py <= H - pad_b):
            return

        min_x = chart_data.get("x_min")
        max_x = chart_data.get("x_max")
        min_y = chart_data.get("y_min")
        max_y = chart_data.get("y_max")

        if min_x is None or max_x is None or min_y is None or max_y is None:
            return

        plot_w = W - pad_l - pad_r
        plot_h = H - pad_t - pad_b
        if plot_w <= 0 or plot_h <= 0:
            return

        x_values = chart_data.get("x_values") or []
        y_values = chart_data.get("y_values") or []

        if not x_values:
            return

        clicked_x = min_x + (px - pad_l) / plot_w * (max_x - min_x)

        idx = bisect.bisect_left(x_values, clicked_x)
        candidates = [i for i in (idx - 1, idx) if 0 <= i < len(x_values)]
        if not candidates:
            return

        best_i = min(candidates, key=lambda i: abs(x_values[i] - clicked_x))
        x_val = x_values[best_i]
        y_val = y_values[best_i]

        def tx(x):
            return pad_l + (x - min_x) / (max_x - min_x) * plot_w

        def ty(y):
            return pad_t + (max_y - y) / (max_y - min_y) * plot_h

        marker_px = tx(x_val)
        marker_py = ty(y_val)

        canvas.delete("marker")

        r = 4
        canvas.create_oval(
            marker_px - r, marker_py - r, marker_px + r, marker_py + r,
            outline="#009933", width=2, fill="white", tags="marker"
        )

        x_label = chart_data.get("x_label", "x")
        y_label = chart_data.get("y_label", "y")
        label = f"{x_label}: {x_val:.4g}\n{y_label}: {y_val:.4g}"

        is_left_half = marker_px < pad_l + plot_w / 2
        anchor = "nw" if is_left_half else "ne"
        offset_x = 8 if is_left_half else -8
        offset_y = 8 if marker_py < pad_t + plot_h / 2 else -34

        canvas.create_text(
            marker_px + offset_x, marker_py + offset_y,
            text=label,
            anchor=anchor,
            fill="#009933",
            font=("Arial", 8, "bold"),
            justify=tk.LEFT,
            tags="marker"
        )

    def on_mouse_wheel(event, canvas, chart_data):
        if chart_data.get("x_min") is None or chart_data.get("x_max") is None:
            return

        delta = getattr(event, "delta", 0)
        if delta == 0:
            if getattr(event, "num", None) == 4:
                delta = 120
            elif getattr(event, "num", None) == 5:
                delta = -120

        if delta == 0:
            return

        factor = 0.9 if delta > 0 else 1.1
        x_min = chart_data["x_min"]
        x_max = chart_data["x_max"]
        y_min = chart_data["y_min"]
        y_max = chart_data["y_max"]

        W = canvas.winfo_width()
        H = canvas.winfo_height()
        plot_w = W - PLOT_PAD_L - PLOT_PAD_R
        plot_h = H - PLOT_PAD_T - PLOT_PAD_B
        if plot_w <= 0 or plot_h <= 0:
            return

        px = min(max(event.x, PLOT_PAD_L), W - PLOT_PAD_R)
        py = min(max(event.y, PLOT_PAD_T), H - PLOT_PAD_B)
        center_x = x_min + (px - PLOT_PAD_L) / plot_w * (x_max - x_min)
        center_y = y_max - (py - PLOT_PAD_T) / plot_h * (y_max - y_min)

        new_width = max((x_max - x_min) * factor, 1e-12)
        new_height = max((y_max - y_min) * factor, 1e-12)

        chart_data["x_min"] = center_x - new_width / 2
        chart_data["x_max"] = center_x + new_width / 2
        chart_data["y_min"] = center_y - new_height / 2
        chart_data["y_max"] = center_y + new_height / 2

        schedule_redraw(canvas, chart_data, delay=16)

    def on_pan_start(event, canvas, chart_data):
        chart_data["pan_start"] = (event.x, event.y)
        chart_data["press_pos"] = (event.x, event.y)
        canvas.delete("marker")

    def on_pan_move(event, canvas, chart_data):
        if not chart_data.get("pan_start"):
            return

        x_start, y_start = chart_data["pan_start"]
        dx = event.x - x_start
        dy = event.y - y_start

        W = canvas.winfo_width()
        H = canvas.winfo_height()
        plot_w = W - PLOT_PAD_L - PLOT_PAD_R
        plot_h = H - PLOT_PAD_T - PLOT_PAD_B
        if plot_w <= 0 or plot_h <= 0:
            return

        x_range = chart_data["x_max"] - chart_data["x_min"]
        y_range = chart_data["y_max"] - chart_data["y_min"]

        chart_data["x_min"] -= dx / plot_w * x_range
        chart_data["x_max"] -= dx / plot_w * x_range
        chart_data["y_min"] += dy / plot_h * y_range
        chart_data["y_max"] += dy / plot_h * y_range

        chart_data["pan_start"] = (event.x, event.y)

        # Natychmiastowa, płynna informacja zwrotna: przesuwamy tylko
        # elementy z tagiem "content" (linię danych, linię referencyjną),
        # zamiast czyścić i przerysowywać cały canvas przy każdym ruchu myszy
        # — to właśnie robiło "biały" flash przy przeciąganiu.
        canvas.move("content", dx, dy)
        canvas.move("refline", 0, dy)

        # Pełne, dokładne przerysowanie (poprawne skale osi/siatki i
        # przycięcie do obszaru wykresu) z niewielkim opóźnieniem.
        schedule_redraw(canvas, chart_data, delay=40)

    def on_pan_end(event, canvas, chart_data):
        press_pos = chart_data.get("press_pos")
        chart_data["pan_start"] = None

        job = chart_data.get("redraw_job")
        if job is not None:
            canvas.after_cancel(job)
            chart_data["redraw_job"] = None

        redraw_from_state(canvas, chart_data)

        # Jesli mysz prawie sie nie poruszyla miedzy wcisnieciem a
        # puszczeniem przycisku, to bylo klikniecie (a nie przeciagniecie) -
        # pokaz najblizszy punkt danych wraz z jego wspolrzednymi.
        if press_pos is not None:
            moved = abs(event.x - press_pos[0]) + abs(event.y - press_pos[1])
            if moved <= 3:
                show_point_at_pixel(canvas, chart_data, event.x, event.y)

    def redraw_charts():
        draw_chart(
            raw_canvas,
            raw_data,
            raw_data["x_values"],
            raw_data["y_values"],
            raw_data["title"],
            raw_data["x_label"],
            raw_data["y_label"],
            "Press Load .csv"
        )
        draw_chart(
            impedance_canvas,
            impedance_data,
            impedance_data["x_values"],
            impedance_data["y_values"],
            impedance_data["title"],
            impedance_data["x_label"],
            impedance_data["y_label"],
            "Press Calculate Z(d)",
            reference_line=impedance_data.get("reference_line"),
            reference_line_label=impedance_data.get("reference_line_label")
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

        # Przelicz czas z sekund na nanosekundy (tylko dla wyświetlania surowych danych)
        if x_label == "Time (s)":
            x_values = [x * 1e9 for x in x_values]
            x_label = "Time (ns)"

        draw_chart(
            raw_canvas,
            raw_data,
            x_values,
            y_values,
            title,
            x_label,
            y_label,
            "Press Load .csv",
            reset_bounds=True
        )

    def prepare_tdr_data(times, voltages):
        c = 299792458.0

        try:
            z0 = float(z0_var.get())
            param_value = float(param_value_var.get())
            param_type = param_type_var.get()
        except ValueError:
            raise ValueError("Z0 and parameter value must be numeric.")

        if z0 <= 0:
            raise ValueError("Z0 must be greater than zero.")

        # Konwersja ER i VF
        if param_type == "VF":
            vf = param_value
            if vf > 1:
                vf /= 100
            if not (0 < vf <= 1):
                raise ValueError("VF must be between 0 and 1, or percent value like 66.")
            er = 1.0 / (vf ** 2)  # er = 1 / vf^2 dla non-magnetic materials
        else:  # ER
            er = param_value
            if er <= 1:
                raise ValueError("ER must be greater than 1.")
            vf = 1.0 / (er ** 0.5)  # vf = 1 / sqrt(er)

        if len(times) < 2:
            raise ValueError("CSV file must contain at least two samples.")

        def average(values):
            return sum(values) / len(values)

        def find_first_above_threshold(values, start_index, threshold):
            """Zwraca indeks pierwszej próbki (od start_index), której |wartość|
            przekracza threshold, albo None, jeśli żadna nie przekracza."""
            for i in range(start_index, len(values)):
                if abs(values[i]) >= threshold:
                    return i
            return None

        def find_pulse_peak_and_end(values, start_index, noise_threshold):
            """Od start_index znajduje indeks szczytu (maks. |wartość|),
            a potem pierwszy indeks po szczycie, gdzie sygnał znów spada
            poniżej noise_threshold. Używane zarówno dla impulsu wysłanego,
            jak i dla odbicia - stąd wspólna funkcja."""
            peak_index = start_index + max(
                range(len(values) - start_index),
                key=lambda i: abs(values[start_index + i])
            )

            end_index = len(values) - 1
            for i in range(peak_index, len(values)):
                if abs(values[i]) < noise_threshold:
                    end_index = i
                    break

            return peak_index, end_index

        # ── Krok 1: znajdź impulse_index (start impulsu #1 - wysłanego) ──
        # Jeśli czas przechodzi przez t=0, to jest to najpewniejszy wyznacznik
        # startu impulsu (moment wyzwolenia oscyloskopu). W przeciwnym razie
        # dopiero wtedy liczymy prowizoryczny offset/próg na potrzeby detekcji
        # amplitudowej - nie robimy tego niepotrzebnie za każdym razem.
        def find_impulse_index():
            if min(times) < 0 < max(times):
                for i, t in enumerate(times):
                    if t >= 0:
                        return i

            prelim_count = max(3, int(len(voltages) * 0.05))
            prelim_offset = average(voltages[:prelim_count])
            prelim_corrected = [v - prelim_offset for v in voltages]
            prelim_noise = average([abs(v) for v in prelim_corrected[:prelim_count]])
            prelim_peak = max(abs(v) for v in prelim_corrected)
            prelim_threshold = max(prelim_noise * 6, prelim_peak * 0.1, 1e-12)

            index = find_first_above_threshold(prelim_corrected, 0, prelim_threshold)
            return index if index is not None else prelim_count

        impulse_index = find_impulse_index()

        # ── Krok 2: właściwy offset, liczony od początku danych
        # do faktycznie znalezionego początku impulsu ──
        baseline_samples = voltages[:impulse_index] if impulse_index >= 1 else voltages[:max(3, int(len(voltages) * 0.05))]
        voltage_offset = average(baseline_samples)
        corrected_voltages = [v - voltage_offset for v in voltages]
        noise_level = average([abs(v) for v in corrected_voltages[:len(baseline_samples)]])

        peak = max(abs(v) for v in corrected_voltages)
        noise_threshold = max(noise_level * 4, peak * 0.02, 1e-12)

        # ── Krok 3: znajdź szczyt i koniec impulsu #1 (wysłanego) ──
        peak_index, pulse_end_index = find_pulse_peak_and_end(
            corrected_voltages, impulse_index, noise_threshold
        )

        calculation_start = min(pulse_end_index + 1, len(voltages) - 1)

        # ── Krok 4: napięcie padające (szczyt impulsu #1) ──
        incident_voltage = corrected_voltages[peak_index]

        if abs(incident_voltage) < 1e-12:
            raise ValueError("Incident pulse amplitude is too close to zero.")

        # ── Krok 5: znajdź impuls #2 (odbicie) i miejsce jego zakończenia ──
        reflection_start_index = find_first_above_threshold(
            corrected_voltages, calculation_start, noise_threshold
        )

        if reflection_start_index is not None:
            reflection_peak_index, _ = find_pulse_peak_and_end(
                corrected_voltages, reflection_start_index, noise_threshold
            )
            # Przycinaj wykres dokładnie na szczycie odbicia, bez dodatkowego marginesu
            plot_end_index = min(reflection_peak_index, len(corrected_voltages) - 1)
        else:
            # Nie znaleziono żadnego odbicia powyżej progu szumu - pokaż wszystko, co jest
            reflection_peak_index = calculation_start
            plot_end_index = len(corrected_voltages) - 1

        return {
            "c": c,
            "z0": z0,
            "vf": vf,
            "er": er,
            "param_type": param_type,
            "times": times,
            "impulse_index": impulse_index,
            "peak_index": peak_index,
            "calculation_start": calculation_start,
            "reflection_peak_index": reflection_peak_index,
            "plot_end_index": plot_end_index,
            "voltage_offset": voltage_offset,
            "corrected_voltages": corrected_voltages,
            "incident_voltage": incident_voltage,
            "t0": times[peak_index]
        }

    def calculate_impedance_vs_distance(data):

        # ── Zmienne potrzebne do obliczeń ──
        # c                  - prędkość światła w próżni [m/s]
        # z0                 - impedancja odniesienia (linii/generatora) [Ohm]
        # vf                 - współczynnik skrócenia (velocity factor) [-]
        # times              - lista czasów próbek [s]
        # calculation_start  - indeks pierwszej próbki liczonej jako odbicie
        #                      (czyli tuż po impulsie #1 - wysłanym)
        # plot_end_index     - indeks końca impulsu #2 (odbicia) + margines;
        #                      wszystko po nim jest odcinane (to echa/szum,
        #                      nie nowa informacja o linii)
        # corrected_voltages - napięcia po odjęciu offsetu (zera) [V]
        # incident_voltage   - napięcie padające Vi, czyli szczyt
        #                      impulsu #1 (wysłanego) [V]
        # t0                 - czas odniesienia (x=0), czas szczytu
        #                      impulsu #1 [s]
        #
        # Wzory (pulse-TDR: brak drugiego odjęcia Vi, patrz dyskusja):
        #   Gamma(t) = Vcorr(t) / Vi                 (współczynnik odbicia)
        #   x(t)     = (t - t0) * c * vf / 2          (odległość, droga w obie strony)
        #   Z(t)     = Z0 * (1 + Gamma) / (1 - Gamma) (impedancja lokalna)

        c = data["c"]
        z0 = data["z0"]
        vf = data["vf"]
        er = data["er"]
        param_type = data["param_type"]
        times = data["times"]
        calculation_start = data["calculation_start"]
        plot_end_index = data["plot_end_index"]
        corrected_voltages = data["corrected_voltages"]
        incident_voltage = data["incident_voltage"]
        t0 = data["t0"]

        # Jeśli został wybrany ER, przelicz VF z ER
        if param_type == "ER":
            vf = 1.0 / (er ** 0.5)

        distances = []
        impedances = []

        reflection_times = times[calculation_start:plot_end_index + 1]
        reflection_voltages = corrected_voltages[calculation_start:plot_end_index + 1]

        for t, voltage in zip(reflection_times, reflection_voltages):
            gamma = voltage / incident_voltage
            gamma = max(min(gamma, 0.99), -0.99)  # zabezpieczenie przed dzieleniem przez 0

            distance = (t - t0) * c * vf / 2
            impedance = z0 * (1 + gamma) / (1 - gamma)

            distances.append(distance)
            impedances.append(impedance)

        if not distances:
            raise ValueError("Could not calculate impedance from this CSV data.")

        return distances, impedances

    def on_calculate_zd():
        loaded = get_loaded_csv()
        if not loaded:
            return

        times, voltages, _, _, _ = loaded

        try:
            data = prepare_tdr_data(times, voltages)
        except ValueError as e:
            messagebox.showerror("Calculation error", str(e))
            return

        info_text = (
            f"impulse_index={data['impulse_index']}  "
            f"peak_index={data['peak_index']}  "
            f"calc_start={data['calculation_start']}  "
            f"reflection_peak={data['reflection_peak_index']}  "
            f"plot_end={data['plot_end_index']}  "
            f"t0={data['t0']:.4e}s  "
            f"Voffset={data['voltage_offset']:.5f}V  "
            f"Vi={data['incident_voltage']:.5f}V"
        )
        info_var.set(info_text)
        print(info_text)

        try:
            distances, impedances = calculate_impedance_vs_distance(data)
        except ValueError as e:
            messagebox.showerror("Calculation error", str(e))
            return

        draw_chart(
            impedance_canvas,
            impedance_data,
            distances,
            impedances,
            "Charakterystyka impedancji od odleglosci",
            "Distance (m)",
            "Impedance (Ohm)",
            "Press Calculate Z(d)",
            reference_line=data["z0"],
            reference_line_label="Z0",
            reset_bounds=True
        )

    tk.Button(
        button_frame,
        text="Browse",
        width=10,
        command=browse_csv
    ).pack(side=tk.LEFT, padx=5)

    tk.Button(
        button_frame,
        text="Load .csv",
        width=10,
        command=on_load_csv
    ).pack(side=tk.LEFT, padx=(0, 5))

    tk.Button(
        button_frame,
        text="Calculate Z(d)",
        width=14,
        command=on_calculate_zd
    ).pack(side=tk.LEFT, padx=(0, 5))

    impedance_window.after(50, redraw_charts)

    impedance_window.bind(
        "<Configure>",
        lambda e: impedance_window.after(10, redraw_charts)
    )

    return impedance_window