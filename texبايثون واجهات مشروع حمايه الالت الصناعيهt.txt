import tkinter as tk
from tkinter import messagebox
import serial
from matplotlib.figure import Figure
from matplotlib.backends.backend_tkagg import FigureCanvasTkAgg
from collections import deque

class SCADADashboard:
    def __init__(self, root):
        self.root = root
        self.root.title("SCADA System - Vibration Monitor")
        self.root.geometry("850x750")
        self.root.configure(bg="#1e1e1e")
        
        # --- المتغيرات التشغيلية ---
        self.is_reading = False
        self.alarm_latched = False  # قفل لتثبيت شاشة الخطر
        self.serial_port = 'COM3'   # تأكد من رقم المنفذ الخاص بك
        self.baud_rate = 9600
        self.arduino = None
        self.y_data = deque([0] * 100, maxlen=100)
        self.danger_marker = None   # نقطة تحديد قمة الخطر على الرسم
        
        self.setup_ui()
        
        # الاتصال وبدء المراقبة تلقائياً عند الإقلاع
        self.connect_serial()
        if self.arduino and self.arduino.is_open:
            self.start_reading()

    def setup_ui(self):
        # 1. إطار العنوان
        header_frame = tk.Frame(self.root, bg="#252526", pady=15)
        header_frame.pack(fill=tk.X)
        
        tk.Label(header_frame, text="نظام المراقبة والتشخيص اللحظي للمحرك", 
                 font=("Segoe UI", 20, "bold"), bg="#252526", fg="#ffffff").pack()

        # 2. إطار التحكم والبيانات
        control_frame = tk.Frame(self.root, bg="#1e1e1e")
        control_frame.pack(pady=15, fill=tk.X, padx=40)
        
        # أزرار التحكم (يمين)
        btn_frame = tk.Frame(control_frame, bg="#1e1e1e")
        btn_frame.pack(side=tk.RIGHT)

        self.reset_btn = tk.Button(btn_frame, text="🔄 إعادة تشغيل وتصفير", font=("Segoe UI", 11, "bold"), 
                                   bg="#007acc", fg="white", width=18, relief="flat", 
                                   activebackground="#005f9e", activeforeground="white", cursor="hand2",
                                   command=self.restart_system)
        self.reset_btn.pack(side=tk.RIGHT, padx=5)

        self.stop_btn = tk.Button(btn_frame, text="⏹ إيقاف مؤقت", font=("Segoe UI", 11, "bold"), 
                                  bg="#444444", fg="white", width=12, relief="flat", 
                                  activebackground="#333333", activeforeground="white", cursor="hand2",
                                  command=self.stop_reading)
        self.stop_btn.pack(side=tk.RIGHT, padx=5)

        # عرض البيانات اللحظية (يسار)
        data_frame = tk.Frame(control_frame, bg="#252526", padx=15, pady=10)
        data_frame.pack(side=tk.LEFT, fill=tk.X, expand=True)

        self.vib_label = tk.Label(data_frame, text="الاهتزاز: 0", font=("Segoe UI", 16, "bold"), 
                                  bg="#252526", fg="#00ffcc")
        self.vib_label.pack(side=tk.LEFT, padx=15)
        
        self.status_label = tk.Label(data_frame, text="الحالة: جاري البدء...", font=("Segoe UI", 14, "bold"), 
                                     bg="#252526", fg="#cccccc")
        self.status_label.pack(side=tk.RIGHT, padx=15)

        # 3. إعداد الرسم البياني
        graph_frame = tk.Frame(self.root, bg="#1e1e1e", bd=0)
        graph_frame.pack(fill=tk.BOTH, expand=True, padx=40, pady=10)

        self.fig = Figure(figsize=(8, 4), dpi=100)
        self.fig.patch.set_facecolor('#1e1e1e')
        
        self.ax = self.fig.add_subplot(111)
        self.ax.set_facecolor('#252526')
        self.ax.set_title("Vibration Signal Waveform (Auto-Capture on Fault)", color='white', fontsize=12, pad=12)
        self.ax.set_xlabel("Samples (Window = 100)", color='gray')
        self.ax.set_ylabel("Amplitude", color='gray')
        
        self.ax.tick_params(colors='gray')
        self.ax.spines['bottom'].set_color('#444444')
        self.ax.spines['left'].set_color('#444444')
        self.ax.spines['top'].set_visible(False)
        self.ax.spines['right'].set_visible(False)
        self.ax.grid(color='#333333', linestyle='--', linewidth=1, alpha=0.7)

        # خط الإشارة الافتراضي
        self.line, = self.ax.plot(range(100), self.y_data, color='#00ffcc', linewidth=2)

        self.canvas = FigureCanvasTkAgg(self.fig, master=graph_frame)
        self.canvas.get_tk_widget().pack(fill=tk.BOTH, expand=True)

    def connect_serial(self):
        try:
            if self.arduino and self.arduino.is_open:
                self.arduino.close()
            self.arduino = serial.Serial(self.serial_port, self.baud_rate, timeout=0.1)
            self.status_label.config(text="الحالة: متصل وجاري المراقبة", fg="#00ca4e")
        except Exception:
            self.status_label.config(text="الحالة: خطأ في منفذ التوصيل", fg="#ff605c")

    def start_reading(self):
        if not self.is_reading:
            self.is_reading = True
            self.read_data()

    def stop_reading(self):
        self.is_reading = False
        self.status_label.config(text="الحالة: متوقف مؤقتاً", fg="#cccccc")

    def restart_system(self):
        """إعادة تهيئة كاملة دون فصل الكابل: مسح الشاشة، فتح المنفذ، واستئناف المراقبة"""
        self.is_reading = False
        self.alarm_latched = False
        
        # إزالة علامة الخطر إن وجدت
        if self.danger_marker:
            self.danger_marker.remove()
            self.danger_marker = None

        # إعادة الألوان وتصفير المنحنى
        self.line.set_color('#00ffcc')
        self.y_data = deque([0] * 100, maxlen=100)
        self.line.set_ydata(self.y_data)
        self.ax.relim()
        self.ax.autoscale_view(scalex=False, scaley=True)
        
        self.root.configure(bg="#1e1e1e")
        self.fig.patch.set_facecolor('#1e1e1e')
        self.vib_label.config(text="الاهتزاز: 0", fg="#00ffcc")
        self.canvas.draw_idle()

        # إعادة فتح منفذ السيريال وتفريغ الـ Buffer
        self.connect_serial()
        if self.arduino and self.arduino.is_open:
            self.arduino.reset_input_buffer()
            self.start_reading()
        else:
            messagebox.showerror("تنبيه", f"تعذر الاتصال بـ {self.serial_port}. تأكد من توصيل الجهاز.")

    def read_data(self):
        if not self.is_reading or self.alarm_latched:
            return

        try:
            if self.arduino and self.arduino.in_waiting > 0:
                data = self.arduino.readline().decode('utf-8', errors='ignore').strip()
                
                if "," in data:
                    vib_str, status_str = data.split(",")
                    vib_val = int(vib_str)
                    
                    self.vib_label.config(text=f"الاهتزاز: {vib_val}")
                    self.y_data.append(vib_val)
                    self.line.set_ydata(self.y_data)
                    self.ax.relim()
                    self.ax.autoscale_view(scalex=False, scaley=True)
                    
                    # فحص حالة الخطر
                    if status_str == "1":
                        # تثبيت الإشارة وتجميد القراءة لمنع تخطي موجة الخطر
                        self.alarm_latched = True
                        self.is_reading = False
                        
                        # تلوين المنحنى بالأحمر ورصد أعلى نقطة سببت الخطر
                        self.line.set_color('#ff3333')
                        max_val = max(self.y_data)
                        max_idx = list(self.y_data).index(max_val)
                        
                        # وضع علامة حمراء واضحة مكان لقطة الخطر
                        self.danger_marker = self.ax.scatter([max_idx], [max_val], color='#ff0055', s=120, zorder=5)
                        
                        self.status_label.config(text="⚠️ خطر! تم التقاط العطل وتجميد الإشارة", fg="#ff605c")
                        self.vib_label.config(fg="#ff605c")
                        self.root.configure(bg="#3a1111")
                        self.fig.patch.set_facecolor('#3a1111')
                    else:
                        self.status_label.config(text="الحالة: طبيعي ومستقر", fg="#00ca4e")
                        self.vib_label.config(fg="#00ffcc")
                    
                    self.canvas.draw_idle()
                        
        except Exception as e:
            print(f"Error reading serial: {e}")
            self.status_label.config(text="الحالة: انقطع الاتصال", fg="#ff605c")
            self.is_reading = False
            
        if self.is_reading and not self.alarm_latched:
            self.root.after(40, self.read_data)

if __name__ == "__main__":
    root = tk.Tk()
    app = SCADADashboard(root)
    root.mainloop()
