import subprocess
import time
import sys
import signal


class ProcessMonitor:
    def __init__(self, command):
        """
        初始化进程监控器

        参数:
            command (list): 要执行的命令，以字符串列表形式提供
        """
        self.command = command
        self.process = None
        self.running = False

    def start_process(self):
        """启动被监控的进程"""
        try:
            print(f"启动进程: {' '.join(self.command)}")
            self.process = subprocess.Popen(self.command)
            print(f"进程已启动，PID: {self.process.pid}")
            return True
        except Exception as e:
            print(f"启动进程失败: {e}")
            return False

    def is_process_running(self):
        """检查进程是否仍在运行"""
        if self.process is None:
            return False

        try:
            # 检查进程是否仍然存活
            self.process.poll()
            return self.process.returncode is None
        except Exception as e:
            print(f"检查进程状态时出错: {e}")
            return False

    def stop_process(self):
        """停止被监控的进程"""
        if self.process and self.is_process_running():
            try:
                # 首先尝试优雅终止
                self.process.terminate()
                self.process.wait(timeout=5)
            except subprocess.TimeoutExpired:
                # 如果优雅终止失败，强制杀死进程
                self.process.kill()
                self.process.wait()
            except Exception as e:
                print(f"停止进程时出错: {e}")

    def monitor(self):
        """主监控循环"""
        self.running = True

        # 初始启动进程
        if not self.start_process():
            return

        while self.running:
            if not self.is_process_running():
                print(f"进程意外终止，退出代码: {self.process.returncode}")
                print("正在重启进程...")
                time.sleep(1)  # 重启前短暂延迟
                if not self.start_process():
                    time.sleep(5)  # 重试前等待更长时间
            else:
                time.sleep(2)  # 每2秒检查一次

    def stop_monitoring(self):
        """停止监控并终止进程"""
        print("正在停止监控器...")
        self.running = False
        self.stop_process()


def main():
    if len(sys.argv) < 2:
        print("用法: python keep_alive.py <命令> [参数...]")
        print("示例: python keep_alive.py python my_script.py")
        sys.exit(1)

    # 从参数中提取命令
    command = sys.argv[1:]

    # 创建监控器实例
    monitor = ProcessMonitor(command)

    def signal_handler(sig, frame):
        """处理中断信号 (Ctrl+C)"""
        print("\n接收到中断信号。正在停止...")
        monitor.stop_monitoring()
        sys.exit(0)

    # 注册信号处理器以实现优雅关闭
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)

    try:
        # 开始监控
        print("启动进程监控器...")
        monitor.monitor()
    except KeyboardInterrupt:
        print("接收到键盘中断")
    finally:
        monitor.stop_monitoring()


if __name__ == "__main__":
    main()
