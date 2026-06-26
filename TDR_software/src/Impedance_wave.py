import tkinter as tk
import random


def Impedance_Wave(root, close_impedance_window):
    root.update_idletasks()
    root_x = root.winfo_x()
    root_y = root.winfo_y()
    root_w = root.winfo_width()

    impedance_window = tk.Toplevel(root)
    impedance_window.title("Impedance Wave")
    impedance_window.geometry(f"600x500+{root_x + root_w + 10}+{root_y}")
    impedance_window.protocol("WM_DELETE_WINDOW", close_impedance_window)

    # ── Górny pasek przycisków ────────────────────────────────────────────────
    btn_frame = tk.Frame(impedance_window)
    btn_frame.pack(fill=tk.X, padx=10, pady=8)

    def on_connect():
        pass  # TODO

    def on_download_csv():
        pass  # TODO

    def on_calculate():
        base  = random.uniform(40, 70)
        noise = random.uniform(2, 8)
        lengths    = [i * 0.1 for i in range(50)]
        impedances = [base + noise * (i % 7 - 3) + random.uniform(-1.5, 1.5)
                      for i in range(50)]
        draw_chart(lengths, impedances)

    tk.Button(btn_frame, text="Connect",       width=12, command=on_connect).pack(side=tk.LEFT, padx=5)
    tk.Button(btn_frame, text="Download .csv", width=12, command=on_download_csv).pack(side=tk.LEFT, padx=5)
    tk.Button(btn_frame, text="Calculate",     width=12, command=on_calculate).pack(side=tk.LEFT, padx=5)

    # ── Canvas na wykres ──────────────────────────────────────────────────────
    chart_frame = tk.Frame(impedance_window, relief=tk.SUNKEN, borderwidth=1)
    chart_frame.pack(fill=tk.BOTH, expand=True, padx=10, pady=(0, 10))

    canvas = tk.Canvas(chart_frame, bg="white")
    canvas.pack(fill=tk.BOTH, expand=True)

    current_data = {"lengths": [], "impedances": []}

    def draw_chart(lengths, impedances, title="Impedance vs Length"):
        current_data["lengths"]    = lengths
        current_data["impedances"] = impedances

        canvas.delete("all")
        canvas.update_idletasks()

        W = canvas.winfo_width()
        H = canvas.winfo_height()

        if W < 10 or H < 10:
            return

        pad_l, pad_r, pad_t, pad_b = 60, 20, 30, 50

        if not lengths or not impedances:
            canvas.create_text(W // 2, H // 2, text="Press Calculate",
                               fill="gray", font=("Arial", 12))
            return

        # Autorange
        min_x, max_x = min(lengths), max(lengths)
        min_y, max_y = min(impedances), max(impedances)
        margin_y = (max_y - min_y) * 0.1 or 5
        min_y -= margin_y
        max_y += margin_y

        def tx(x):
            return pad_l + (x - min_x) / (max_x - min_x) * (W - pad_l - pad_r)

        def ty(y):
            return pad_t + (max_y - y) / (max_y - min_y) * (H - pad_t - pad_b)

        canvas.create_text(W // 2, 14, text=title, font=("Arial", 10, "bold"))

        canvas.create_line(pad_l, pad_t, pad_l, H - pad_b, fill="black", width=2)
        canvas.create_line(pad_l, H - pad_b, W - pad_r, H - pad_b, fill="black", width=2)

        canvas.create_text(W // 2, H - 12, text="Length (m)", font=("Arial", 9))
        canvas.create_text(14, H // 2, text="Z (Ω)", font=("Arial", 9), angle=90)

        steps = 5
        for i in range(steps + 1):
            y_val = min_y + i * (max_y - min_y) / steps
            y_pix = ty(y_val)
            canvas.create_line(pad_l - 4, y_pix, pad_l, y_pix, fill="black")
            canvas.create_text(pad_l - 8, y_pix, text=f"{y_val:.1f}",
                               anchor="e", font=("Arial", 8))
            canvas.create_line(pad_l, y_pix, W - pad_r, y_pix,
                               fill="#e0e0e0", dash=(2, 4))

        for i in range(steps + 1):
            x_val = min_x + i * (max_x - min_x) / steps
            x_pix = tx(x_val)
            canvas.create_line(x_pix, H - pad_b, x_pix, H - pad_b + 4, fill="black")
            canvas.create_text(x_pix, H - pad_b + 14, text=f"{x_val:.1f}",
                               font=("Arial", 8))

        points = [c for pair in zip([tx(x) for x in lengths],
                                    [ty(y) for y in impedances]) for c in pair]
        if len(points) >= 4:
            canvas.create_line(points, fill="#2266cc", width=2, smooth=True)

    impedance_window.after(50, lambda: draw_chart([], []))

    impedance_window.bind("<Configure>", lambda e: canvas.after(
        10, lambda: draw_chart(current_data["lengths"], current_data["impedances"])
    ))

    return impedance_window