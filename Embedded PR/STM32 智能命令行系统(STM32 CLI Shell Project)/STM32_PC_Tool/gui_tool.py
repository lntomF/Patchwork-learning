import serial
import serial.tools.list_ports
import tkinter as tk
from tkinter import ttk, scrolledtext
import threading
import time

class FanControlApp:
    def __init__(self, root):
        self.root = root
        self.root.title("STM32 智能温控上位机")
        self.root.geometry("600x450")
        
        self.ser = None
        self.receiving = False

        # --- 1. 串口设置区域 ---
        frame_config = ttk.LabelFrame(root, text="串口设置")
        frame_config.pack(pady=10, padx=10, fill="x")

        ttk.Label(frame_config, text="端口:").pack(side="left", padx=5)
        self.combo_port = ttk.Combobox(frame_config, width=15)
        self.combo_port.pack(side="left", padx=5)
        # 自动扫描端口
        self.scan_ports()
        
        ttk.Label(frame_config, text="波特率:").pack(side="left", padx=5)
        self.entry_baud = ttk.Entry(frame_config, width=10)
        self.entry_baud.insert(0, "115200") # 默认 115200
        self.entry_baud.pack(side="left", padx=5)

        self.btn_open = ttk.Button(frame_config, text="打开串口", command=self.toggle_serial)
        self.btn_open.pack(side="left", padx=10)
        
        # 刷新串口按钮
        ttk.Button(frame_config, text="刷新", command=self.scan_ports).pack(side="left", padx=5)

        # --- 2. 专属控制区域 ---
        frame_ctrl = ttk.LabelFrame(root, text="控制面板")
        frame_ctrl.pack(pady=10, padx=10, fill="x")

        # 第一排按钮
        ttk.Button(frame_ctrl, text="读取温度 (TEMP)", command=lambda: self.send_cmd("TEMP")).grid(row=0, column=0, padx=10, pady=10)
        ttk.Button(frame_ctrl, text="开启自动 (AUTO 1)", command=lambda: self.send_cmd("AUTO 1")).grid(row=0, column=1, padx=10, pady=10)
        ttk.Button(frame_ctrl, text="关闭自动 (AUTO 0)", command=lambda: self.send_cmd("AUTO 0")).grid(row=0, column=2, padx=10, pady=10)
        
        # 第二排按钮
        ttk.Button(frame_ctrl, text="LED 翻转", command=lambda: self.send_cmd("LED TOGGLE")).grid(row=1, column=0, padx=10, pady=10)
        ttk.Button(frame_ctrl, text="舵机归零", command=lambda: self.send_cmd("MOTOR 0")).grid(row=1, column=1, padx=10, pady=10)
        ttk.Button(frame_ctrl, text="舵机全开", command=lambda: self.send_cmd("MOTOR 180")).grid(row=1, column=2, padx=10, pady=10)

        # --- 3. 日志显示区域 ---
        frame_log = ttk.LabelFrame(root, text="接收日志")
        frame_log.pack(pady=10, padx=10, fill="both", expand=True)

        self.text_log = scrolledtext.ScrolledText(frame_log, height=10)
        self.text_log.pack(fill="both", expand=True)

    def scan_ports(self):
        ports = list(serial.tools.list_ports.comports())
        # 提取端口号，例如 "COM3 - USB Serial Device"
        port_list = [f"{p.device}" for p in ports]
        self.combo_port['values'] = port_list
        if port_list:
            self.combo_port.current(0)
        else:
            self.combo_port.set("未发现串口")

    def toggle_serial(self):
        if self.ser and self.ser.is_open:
            self.receiving = False
            self.ser.close()
            self.btn_open.config(text="打开串口")
            self.log("串口已关闭")
        else:
            try:
                # 只获取 COMx 部分
                port_str = self.combo_port.get().split(' ')[0]
                baud = int(self.entry_baud.get())
                self.ser = serial.Serial(port_str, baud, timeout=0.5)
                self.receiving = True
                self.btn_open.config(text="关闭串口")
                self.log(f"串口 {port_str} 已打开")
                # 开启接收线程
                threading.Thread(target=self.read_serial, daemon=True).start()
            except Exception as e:
                self.log(f"打开失败: {e}")

    def send_cmd(self, cmd):
        if self.ser and self.ser.is_open:
            # 关键：加上 \r\n，因为你的单片机是用回车判断命令结束的
            full_cmd = cmd + "\r\n"
            self.ser.write(full_cmd.encode('utf-8'))
            self.log(f">> 发送: {cmd}")
        else:
            self.log("错误: 请先打开串口")

    def read_serial(self):
        while self.receiving and self.ser.is_open:
            try:
                if self.ser.in_waiting:
                    # 读取一行数据，忽略解码错误
                    data = self.ser.readline().decode('utf-8', errors='ignore').strip()
                    if data:
                        self.log(f"[RX] {data}")
            except Exception as e:
                print(e)
                break
            time.sleep(0.01)

    def log(self, msg):
        self.text_log.insert(tk.END, msg + "\n")
        self.text_log.see(tk.END)

if __name__ == "__main__":
    root = tk.Tk()
    app = FanControlApp(root)
    root.mainloop()