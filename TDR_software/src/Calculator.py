import tkinter as tk


def Calculator(root, close_callback):

    root.update_idletasks()
    root_x = root.winfo_x()
    root_y = root.winfo_y()
    root_w = root.winfo_width()

    calculator_window = tk.Toplevel(root)
    calculator_window.title("TDR Calculator")
    calculator_window.geometry(f"400x400+{root_x + root_w + 620}+{root_y}")

    calculator_window.protocol("WM_DELETE_WINDOW", close_callback)

    # ------------------------------------------------------------------
    # Diagram
    # ------------------------------------------------------------------

    diagram_frame = tk.Frame(
        calculator_window,
        relief=tk.SUNKEN,
        borderwidth=1,
        bg="white",
        height=150
    )
    diagram_frame.pack(fill=tk.X, padx=10, pady=10)
    diagram_frame.pack_propagate(False)

    tk.Label(
        diagram_frame,
        text="[ diagram ]",
        bg="white",
        fg="gray",
        font=("Arial", 10)
    ).place(relx=0.5, rely=0.5, anchor="center")

    # ------------------------------------------------------------------
    # Input
    # ------------------------------------------------------------------

    fields_frame = tk.Frame(
        calculator_window,
        relief=tk.GROOVE,
        borderwidth=1
    )
    fields_frame.pack(fill=tk.X, padx=10, pady=5)

    t_var = tk.StringVar()
    material_var = tk.StringVar()
    result_var = tk.StringVar()

    input_mode = "vf"

    # ---------------- Time ----------------

    tk.Label(
        fields_frame,
        text="Time between impulses (ns):",
        width=25,
        anchor="w"
    ).grid(row=0, column=0, padx=8, pady=6, sticky="w")

    ent_time = tk.Entry(
        fields_frame,
        textvariable=t_var,
        width=20
    )
    ent_time.grid(row=0, column=1, padx=8, pady=6, sticky="w")

    # ---------------- Material ----------------

    lbl_material = tk.Label(
        fields_frame,
        text="Velocity factor:",
        width=25,
        anchor="w"
    )

    lbl_material.grid(row=1, column=0, padx=8, pady=6, sticky="w")

    material_frame = tk.Frame(fields_frame)
    material_frame.grid(row=1, column=1, padx=8, pady=6, sticky="w")

    ent_material = tk.Entry(
        material_frame,
        textvariable=material_var,
        width=20
    )

    ent_material.pack(side=tk.LEFT)

    # ------------------------------------------------------------------
    # Toggle button
    # ------------------------------------------------------------------

    def toggle_mode():
        nonlocal input_mode

        if input_mode == "vf":
            input_mode = "er"
            mode_button.config(text="εᵣ")
            lbl_material.config(text="Relative permittivity:")
        else:
            input_mode = "vf"
            mode_button.config(text="VF")
            lbl_material.config(text="Velocity factor:")

        material_var.set("")
        result_var.set("")

    mode_button = tk.Button(
        material_frame,
        text="VF",
        width=4,
        command=toggle_mode
    )

    mode_button.pack(side=tk.LEFT, padx=(5, 0))

    # ------------------------------------------------------------------
    # Calculate
    # ------------------------------------------------------------------

    def on_calculate():

        c = 299792458.0

        try:

            t = float(t_var.get()) * 1e-9

            if input_mode == "vf":

                vf = float(material_var.get())

                # można wpisać 66 zamiast 0.66
                if vf > 1:
                    vf /= 100

                if not (0 < vf <= 1):
                    raise ValueError("Velocity factor must be between 0 and 1.")

                distance = vf * c * t / 2

            else:

                er = float(material_var.get())

                if er <= 0:
                    raise ValueError("Relative permittivity must be greater than zero.")

                distance = c * t / (2 * er ** 0.5)

            result_var.set(
                f"Distance = {distance:.4f} m ({distance*100:.2f} cm)"
            )

        except ValueError as e:
            result_var.set(str(e))

    tk.Button(
        calculator_window,
        text="Calculate",
        width=15,
        command=on_calculate
    ).pack(pady=10)

    # ------------------------------------------------------------------
    # Result
    # ------------------------------------------------------------------

    result_frame = tk.Frame(
        calculator_window,
        relief=tk.SUNKEN,
        borderwidth=1
    )

    result_frame.pack(fill=tk.X, padx=10, pady=(0, 10))

    tk.Label(
        result_frame,
        text="Result:",
        font=("Arial", 9, "bold")
    ).pack(anchor="w", padx=8, pady=(5, 0))

    tk.Label(
        result_frame,
        textvariable=result_var,
        font=("Arial", 11),
        fg="#2266cc",
        justify=tk.LEFT
    ).pack(anchor="w", padx=8, pady=8)

    return calculator_window