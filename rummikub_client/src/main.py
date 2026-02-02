# main.py
import pygame
import queue
import sys
from settings import *
from network import NetworkClient
from tile import Tile
from ui import GameRenderer

class RummikubGame:
    def __init__(self):
        #Inicjalizacja Pygame
        pygame.init()
        self.screen = pygame.display.set_mode((WINDOW_WIDTH, WINDOW_HEIGHT), pygame.RESIZABLE)
        pygame.display.set_caption("Rummikub Client Python")
        self.clock = pygame.time.Clock()
        
        # UI MODULE
        self.ui = GameRenderer(self.screen)
        
        # Sieć
        self.msg_queue = queue.Queue()
        self.net = NetworkClient(HOST, PORT, self.msg_queue)
        self.net.connect()
        
        # Stan gry
        self.state = STATE_LOBBY
        self.my_player_id = -1
        self.current_turn_pid = -1
        self.my_turn = False
        
        self.player_counts = {}
        self.player_names = {}
        
        self.game_result_msg = ""
        
        # Dane gry
        self.hand_tiles = []
        self.table_tiles = []
        self.locked_ids = set()
        self.dragging_tile = None
        self.drag_offset_x = 0
        self.drag_offset_y = 0
        
        # Dane Lobby
        self.nickname = "Gracz"
        self.input_active = False

    def run(self):
        running = True
        while running:
            # Sieć
            while not self.msg_queue.empty():
                self.handle_network_message(self.msg_queue.get())

            # eventy Pygame
            for event in pygame.event.get():
                if event.type == pygame.QUIT:
                    running = False
                elif event.type == pygame.VIDEORESIZE:
                    # Delegownie do przeliczanie do UI
                    self.ui.recalc_dimensions(event.w, event.h)
                
                if self.state == STATE_LOBBY:
                    self.handle_lobby_events(event)
                elif self.state == STATE_GAME:
                    self.handle_game_events(event)
                elif self.state == STATE_GAME_OVER:
                    self.handle_gameover_events(event)

            # Drag - aktualizacja pozycji
            if self.state == STATE_GAME and self.dragging_tile:
                mx, my = pygame.mouse.get_pos()
                self.dragging_tile.rect.x = mx - self.drag_offset_x
                self.dragging_tile.rect.y = my - self.drag_offset_y

            # Draw - Delegacja do UI
            if self.state == STATE_LOBBY:
                self.ui.draw_lobby(self.nickname, self.input_active)
            elif self.state == STATE_GAME:
                self.ui.draw_game(self) # Przekazujemy 'self' jako obiekt stanu
            elif self.state == STATE_GAME_OVER:
                self.ui.draw_game_over(self.game_result_msg)
            
            pygame.display.flip()
            self.clock.tick(60)
        
        pygame.quit()
        sys.exit()

    # Obsluga zdarzeń Lobby - łączenie, wpisywanie nicku, dołączanie do gry
    def handle_lobby_events(self, event):
        if event.type == pygame.MOUSEBUTTONDOWN:
            if event.button == 1:
                mx, my = event.pos
                if self.ui.input_rect.collidepoint(mx, my):
                    self.input_active = True
                else:
                    self.input_active = False
                
                for rect, txt, num in self.ui.buttons:
                    if rect.collidepoint(mx, my):
                        safe_nick = self.nickname.replace(" ", "_")
                        self.net.send(f"JOIN {num} {safe_nick}") # Wysłanie JOIN

        elif event.type == pygame.KEYDOWN:
            if self.input_active:
                if event.key == pygame.K_RETURN: self.input_active = False
                elif event.key == pygame.K_BACKSPACE: self.nickname = self.nickname[:-1]
                elif event.key == pygame.K_SPACE: pass # ignorowanie spacji
                else:
                    if len(self.nickname) < 12: self.nickname += event.unicode
                    
    def handle_game_events(self, event):
        if event.type == pygame.MOUSEBUTTONDOWN:
            if event.button == 1: self.handle_mouse_down(event.pos)
        elif event.type == pygame.MOUSEBUTTONUP: 
            if event.button == 1: self.handle_mouse_up(event.pos) 
        elif event.type == pygame.KEYDOWN:
            if event.key == pygame.K_d: self.net.send("DRAW") # Dobierz klocek D
            if event.key == pygame.K_c: self.commit_turn()      # Zatwierdź turę C  

    def handle_gameover_events(self, event):
        if event.type == pygame.MOUSEBUTTONDOWN:
            if self.ui.rect_back_btn.collidepoint(event.pos): # Wróć do lobby
                self.state = STATE_LOBBY
                self.hand_tiles = []
                self.table_tiles = []
                self.player_counts = {}

    # OBSŁUGA KLOCKÓW I TUR
    def handle_mouse_down(self, pos):
        for t in reversed(self.hand_tiles + self.table_tiles):
            if t.rect.collidepoint(pos):
                if t.location == 'table' and not self.my_turn: return
                self.dragging_tile = t
                self.drag_offset_x = pos[0] - t.rect.x
                self.drag_offset_y = pos[1] - t.rect.y
                t.orig_loc = t.location
                t.location = 'dragging'
                return

    def handle_mouse_up(self, pos):
        t = self.dragging_tile
        if not t: return
        self.dragging_tile = None
        mx, my = pos
        
        # Używamy wymiarów z UI
        if my < self.ui.hand_start_y: # Table
            if not self.my_turn:
                self.return_tile(t)
                return
            gx = (mx - self.ui.grid_start_x) // self.ui.cell_w
            gy = (my - self.ui.table_grid_start_y) // self.ui.cell_h
            if 0 <= gx < GRID_COLS and 0 <= gy < GRID_ROWS_TABLE:
                self.place_tile(t, 'table', gx, gy)
            else: self.return_tile(t)
        else: # Hand
            gx = (mx - self.ui.grid_start_x) // self.ui.cell_w
            gy = (my - self.ui.hand_grid_start_y) // self.ui.cell_h
            if t.id in self.locked_ids:
                self.return_tile(t)
                return
            if 0 <= gx < GRID_COLS and 0 <= gy < GRID_ROWS_HAND:
                self.place_tile(t, 'hand', gx, gy)
            else: self.return_tile(t)

    def place_tile(self, tile, target_loc, gx, gy):
        target_list = self.table_tiles if target_loc == 'table' else self.hand_tiles
        
        # Sprawdzamy czy miejsce jest zajęte (Insert Mode)
        occupant = next((x for x in target_list if x.grid_x == gx and x.grid_y == gy and x != tile), None)
        
        if occupant:
            if target_loc == 'table':
                # NA STOLE: Zabronione Insert Mode
                self.return_tile(tile)
                return
            else:
                # W RĘCE: Dozwolone Insert Mode
                max_rows = GRID_ROWS_HAND
                # Próbujemy rozsunąć klocki w ręce
                if not self.shift_right(target_list, gx, gy, max_rows, ignore_tile=tile):
                    self.return_tile(tile)
                    return

        # Logika przenoszenia między listami
        if tile in self.table_tiles and target_loc == 'hand':
            self.table_tiles.remove(tile)
            self.hand_tiles.append(tile)
            self.net.send(f"PULL {tile.id}")
        elif tile in self.hand_tiles and target_loc == 'table':
            self.hand_tiles.remove(tile)
            self.table_tiles.append(tile)
        
        # Aktualizacja pozycji wstawianego klocka
        tile.location = target_loc
        tile.grid_x, tile.grid_y = gx, gy
        
        # Wysłanie POS dla wstawionego klocka (jako ostatni komunikat)
        if target_loc == 'table':
            self.net.send(f"POS {tile.id} {gx} {gy} {tile.color_code} {tile.value}")

    def shift_right(self, tile_list, start_x, y, max_rows, ignore_tile=None):
        # 1. Zbieramy klocki w danym wierszu od start_x w prawo (z wyłączeniem ignore_tile) 
        row_tiles = [t for t in tile_list if t.grid_y == y and t.grid_x >= start_x and t != ignore_tile]
        
        # 2. Sortujemy je malejąco po grid_x (żeby przesuwać od końca)
        row_tiles.sort(key=lambda t: t.grid_x, reverse=True)
        
        if not row_tiles: return True
        
        # 3. Sprawdzamy czy skrajny klocek nie wypadnie poza planszę
        if row_tiles[0].grid_x >= GRID_COLS - 1:
            return False # Brak miejsca na przesunięcie
            
        # 4. Wykonujemy przesunięcie i wysyłamy komunikaty
        for t in row_tiles:
            t.grid_x += 1
            if t.location == 'table':
                # Format: POS <id> <x> <y> <col> <val>
                self.net.send(f"POS {t.id} {t.grid_x} {t.grid_y} {t.color_code} {t.value}")
                
        return True

    def return_tile(self, t):
        t.location = t.orig_loc # Przywrócenie oryginalnej lokalizacji

    def commit_turn(self):
        # Budowanie komunikatu COMMIT
        grid = [[None]*GRID_COLS for _ in range(GRID_ROWS_TABLE)]
        for t in self.table_tiles: grid[t.grid_y][t.grid_x] = t
        groups = []
        for y in range(GRID_ROWS_TABLE):
            curr = []
            for x in range(GRID_COLS):
                if grid[y][x]: curr.append(str(grid[y][x].id))
                else:
                    if curr: groups.append(",".join(curr)); curr = []
            if curr: groups.append(",".join(curr))
        self.net.send("COMMIT " + "|".join(groups))
        self.my_turn = False

    def add_tile_to_hand(self, t_str):
        tid, col, val = self.parse_tile_str(t_str)
        gx, gy = self.find_free_spot(self.hand_tiles, GRID_COLS, GRID_ROWS_HAND)
        t = Tile(tid, col, val, location='hand')
        t.grid_x, t.grid_y = gx, gy
        self.hand_tiles.append(t)

    def find_free_spot(self, tile_list, cols, rows):
        # Szukanie wolnego miejsca w siatce do Draw
        occ = {(t.grid_x, t.grid_y) for t in tile_list}
        for y in range(rows):
            for x in range(cols):
                if (x, y) not in occ: return x, y
        return 0, 0

    def parse_tile_str(self, t_str):
        p = t_str.split(':')
        return int(p[0]), int(p[1]), int(p[2])
    
    def get_tile_by_id(self, tid):
        for t in self.table_tiles + self.hand_tiles:
            if t.id == tid: return t
        return None

    # Obsługa komunikatów sieciowych
    def handle_network_message(self, msg):
        parts = msg.split(' ')
        cmd = parts[0]
        args = parts[1:]

        if cmd == "GAME_START":
            # Format: GAME_START <id1> <nick1> <id2> <nick2> ...
            self.state = STATE_GAME
            self.table_tiles = []
            self.player_counts = {}
            self.player_names = {} # Reset nazw
            
            print(f"[GAME] Start gry. Otrzymano listę graczy: {args}")
            
            # Parsowanie listy graczy
            if len(args) >= 2:
                for i in range(0, len(args), 2):
                    try:
                        pid = int(args[i])
                        nick = args[i+1]
                        self.player_names[pid] = nick
                        self.player_counts[pid] = 14 
                    except IndexError:
                        pass
                    except ValueError:
                        print(f"Błąd parsowania gracza w GAME_START: {args}")

        elif cmd == "GAME_OVER":
            self.state = STATE_GAME_OVER
            content = msg.replace("GAME_OVER", "").strip()
            if not content:
                # Brak argumentów -> Ktoś wyszedł
                self.game_result_msg = "Ktoś opuścił grę.|Rozgrywka przerwana."
                print("[GAME] Ktoś się rozłączył. Powrót do Lobby.")
            else:
                # Normalny koniec z wynikami
                # Format: Winner: X | Scores...
                self.game_result_msg = content
                print("[GAME] Koniec gry (Wyniki).")
            
        elif cmd == "WELCOME_HAND":
            # Format: WELCOME_HAND <MY_ID> <t1> <t2> ...
            if len(args) > 0:
                self.my_player_id = int(args[0])
                
                # 1. Parsujemy listę klocków od serwera do słownika {id: (col, val)}
                server_hand_data = {}
                for t_str in args[1:]:
                    try:
                        tid, col, val = self.parse_tile_str(t_str)
                        server_hand_data[tid] = (col, val)
                    except: pass
                
                # 2. Usuwamy z naszej ręki klocki, których nie ma na liście serwera
                self.hand_tiles = [t for t in self.hand_tiles if t.id in server_hand_data]
                
                # 3. Aktualizujemy istniejące i dodajemy nowe
                for tid, (col, val) in server_hand_data.items():
                    existing_tile = next((t for t in self.hand_tiles if t.id == tid), None)
                    
                    if existing_tile:
                        # Klocek już jest - aktualizujemy tylko dane (nie ruszamy pozycji X,Y)
                        existing_tile.color_code = col
                        existing_tile.value = val
                    else:
                        # Znajdź wolne miejsce i dodaj nowy klocek
                        gx, gy = self.find_free_spot(self.hand_tiles, GRID_COLS, GRID_ROWS_HAND)
                        new_t = Tile(tid, col, val, location='hand')
                        new_t.grid_x = gx
                        new_t.grid_y = gy
                        self.hand_tiles.append(new_t)

                # Aktualizacja licznika (dla spójności UI)
                self.player_counts[self.my_player_id] = len(self.hand_tiles)

        elif cmd == "HAND_COUNTS":
            # Format: HAND_COUNTS <count1> <count2> ...
            for pid, count_str in enumerate(args):
                try: self.player_counts[pid] = int(count_str)
                except: pass

        elif cmd == "TURN_INFO":
            try:
                self.current_turn_pid = int(args[0])
                self.my_turn = (self.current_turn_pid == self.my_player_id)
                if self.my_turn: self.locked_ids = {t.id for t in self.table_tiles}
                else: self.locked_ids.clear()
            except: pass

        elif cmd == "POS":
            tid, gx, gy, col, val = map(int, args[:5])
            tile = self.get_tile_by_id(tid)
            if tile:
                tile.grid_x, tile.grid_y = gx, gy
                tile.color_code, tile.value = col, val
                if tile.location == 'hand':
                    self.hand_tiles.remove(tile)
                    self.table_tiles.append(tile)
                tile.location = 'table'
            else:
                new_tile = Tile(tid, col, val, location='table')
                new_tile.grid_x, new_tile.grid_y = gx, gy
                self.table_tiles.append(new_tile)
                if not self.my_turn and self.current_turn_pid in self.player_counts:
                    self.player_counts[self.current_turn_pid] -= 1

        elif cmd == "PULL":
            tid = int(args[0])
            tile = self.get_tile_by_id(tid)
            if tile:
                if tile in self.table_tiles: self.table_tiles.remove(tile)
                if tile in self.hand_tiles: self.hand_tiles.remove(tile)
                if not self.my_turn and self.current_turn_pid in self.player_counts:
                    self.player_counts[self.current_turn_pid] += 1
        
        elif cmd == "OPP_DRAWN":
            if self.current_turn_pid in self.player_counts:
                self.player_counts[self.current_turn_pid] += 1

        elif cmd == "TILE_DRAWN":
            self.add_tile_to_hand(args[0])
            if self.my_player_id in self.player_counts:
                self.player_counts[self.my_player_id] += 1

        elif cmd == "BOARD_UPDATE":
            self.table_tiles = []
            for t_str in args:
                try:
                    parts = t_str.split(':')
                    if len(parts) >= 5:
                        tid, gx, gy, col, val = map(int, parts[:5])
                        in_hand = next((t for t in self.hand_tiles if t.id == tid), None)
                        if in_hand: self.hand_tiles.remove(in_hand)
                        new_tile = Tile(tid, col, val, location='table')
                        new_tile.grid_x, new_tile.grid_y = gx, gy
                        self.table_tiles.append(new_tile)
                except: pass
            self.locked_ids = {t.id for t in self.table_tiles}

if __name__ == "__main__":
    RummikubGame().run()