import tkinter
from tkinter import ttk, messagebox
import threading
import uuid
import time
import paho.mqtt.client as mqtt

BROKER_HOST = "localhost"
BROKER_PORT = "1883"
AUTO_TOPIC = "auto_bot/control"
MANUAL_TOPIC = "manual_bot/control"

WINDOW_TITLE = "Renesas AutonomoCar"

class MqttApp:
    def __init__(self, root):
        self.root = root
        self.root.title(WINDOW_TITLE)
        self.root.geometry("1280x720")

        top_bar = ttk.Frame(self.root)
        top_bar.pack(side=tkinter.TOP, fill=tkinter.X)

        self.close_button = ttk.Button(top_bar, text="Close", command=self._on_close)
        self.close_button.pack(side=tkinter.RIGHT, padx=12, pady=8)

        paned = ttk.Panedwindow(self.root, orient=tkinter.HORIZONTAL)
        paned.pack(fill=tkinter.BOTH, expand=True)

        #Auto control pane
        self.left_frame = ttk.Frame(paned, padding=12)
        paned.add(self.left_frame, weight=1)

        #Manual Control pane
        self.right_frame = ttk.Frame(paned, padding=12)
        paned.add(self.right_frame, weight=1)

        self.root.protocol("WM_DELETE_WINDOW", self._on_close)

        self.mqtt = None

    def _on_close(self):
        if not messagebox.askokcancel("Quit", "Do you really want to quit?"):
            return
#        try:
#            if getattr(self, "mqtt", None) is not None and hasattr(self.mqtt, "disconnect"):
#                try:
#                    self.mqtt.disconnect()
#                except Exception:
#                    pass
#        except Exception:
#            pass
        try:
            self.root.destroy()
        except Exception:
            try:
                self.root.quit()
            except Exception:
                pass

    def run(self):
        self.root.mainloop()

if __name__ == "__main__":
    root = tkinter.Tk()
    app = MqttApp(root)
    app.run()
