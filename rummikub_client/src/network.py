# network.py
import socket
import threading

class NetworkClient:
    def __init__(self, host, port, msg_queue):
        self.sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
        self.msg_queue = msg_queue
        self.connected = False
        self.host = host
        self.port = port

    def connect(self):
        try:
            self.sock.connect((self.host, self.port))
            self.connected = True
            print(f"--- POŁĄCZONO Z SERWEREM {self.host}:{self.port} ---")
            # Uruchomienie wątku nasłuchującego
            threading.Thread(target=self.receive_loop, daemon=True).start()
            return True
        except Exception as e:
            print(f"!!! BŁĄD POŁĄCZENIA: {e}")
            return False

    def receive_loop(self):
        buffer = ""
        while self.connected:
            try:
                data = self.sock.recv(1024).decode('utf-8')
                if not data:
                    print("--- POŁĄCZENIE ZAMKNIĘTE PRZEZ SERWER ---")
                    self.connected = False
                    break
                
                buffer += data
                while '\n' in buffer:
                    msg, buffer = buffer.split('\n', 1)
                    msg = msg.strip()
                    if msg:
                        print(f"[RECV] {msg}") # Logowanie odbioru
                        self.msg_queue.put(msg)
            except Exception as e:
                print(f"!!! BŁĄD ODBIORU: {e}")
                self.connected = False
                break

    def send(self, msg):
        if self.connected:
            try:
                print(f"[SENT] {msg}") # Logowanie wysyłki
                self.sock.sendall((msg + '\n').encode('utf-8'))
            except Exception as e:
                print(f"!!! BŁĄD WYSYŁANIA: {e}")
                self.connected = False