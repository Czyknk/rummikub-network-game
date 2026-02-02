# ui.py
import pygame
from settings import *

class GameRenderer:
    def __init__(self, screen):
        self.screen = screen
        self.font = pygame.font.SysFont('Arial', 24, bold=True)
        self.font_title = pygame.font.SysFont('Arial', 72, bold=True)
        self.font_ui = pygame.font.SysFont('Arial', 32)
        
        # Zmienne układu (Layout)
        self.table_area_h = 0
        self.hand_start_y = 0
        self.cell_w = 0
        self.cell_h = 0
        self.grid_start_x = 0
        self.table_grid_start_y = 0
        self.hand_grid_start_y = 0
        
        # Elementy UI Lobby
        self.input_rect = pygame.Rect(0, 0, 0, 0)
        self.buttons = []
        self.rect_back_btn = pygame.Rect(0, 0, 0, 0)
        
        # Przeliczamy raz na start
        self.recalc_dimensions(screen.get_width(), screen.get_height())

    def recalc_dimensions(self, w, h):
        # --- GAME LAYOUT ---
        self.table_area_h = int(h * 0.70)
        self.hand_area_h = h - self.table_area_h
        self.hand_start_y = self.table_area_h
        
        margin = 20
        usable_w = w - 2 * margin
        self.cell_w = usable_w // GRID_COLS
        self.cell_h = int(self.cell_w * 1.5)
        
        self.grid_start_x = margin + (usable_w - (self.cell_w * GRID_COLS)) // 2
        
        total_table_grid_h = GRID_ROWS_TABLE * self.cell_h
        self.table_grid_start_y = (self.table_area_h - total_table_grid_h) // 2
        if self.table_grid_start_y < margin: self.table_grid_start_y = margin

        total_hand_grid_h = GRID_ROWS_HAND * self.cell_h
        self.hand_grid_start_y = self.hand_start_y + (self.hand_area_h - total_hand_grid_h) // 2
        
        # Skalowanie czcionki klocków
        font_size = int(self.cell_h * 0.5)
        if font_size < 10: font_size = 10
        self.font = pygame.font.SysFont('Arial', font_size, bold=True)

        # --- LOBBY LAYOUT ---
        cx, cy = w // 2, h // 2
        self.input_rect = pygame.Rect(cx - 150, cy - 50, 300, 50)
        
        btn_w, btn_h = 200, 60
        start_y = cy + 50
        gap = 20
        self.buttons = []
        for i, num in enumerate([2, 3, 4]):
            rect = pygame.Rect(cx - btn_w//2, start_y + i*(btn_h+gap), btn_w, btn_h)
            self.buttons.append((rect, f"{num} Graczy", num))
            
        self.rect_back_btn = pygame.Rect(cx - 100, h - 100, 200, 60)

    # --- RYSOWANIE LOBBY ---
    def draw_lobby(self, nickname, input_active):
        self.screen.fill(COLOR_BG)
        mx, my = pygame.mouse.get_pos()
        
        # Tytuł
        title = self.font_title.render("Rummikub", True, COLOR_TITLE)
        self.screen.blit(title, title.get_rect(center=(self.screen.get_width()//2, self.screen.get_height()//2 - 150)))
        
        # Input
        lbl = self.font_ui.render("Twój Nick:", True, (200, 200, 200))
        self.screen.blit(lbl, (self.input_rect.x, self.input_rect.y - 40))
        
        col = COLOR_INPUT_ACTIVE if input_active else COLOR_INPUT_BORDER
        pygame.draw.rect(self.screen, COLOR_INPUT_BG, self.input_rect)
        pygame.draw.rect(self.screen, col, self.input_rect, 2)
        self.screen.blit(self.font_ui.render(nickname, True, (255,255,255)), (self.input_rect.x + 10, self.input_rect.y + 10))
        
        # Przyciski
        for rect, txt, num in self.buttons:
            col = COLOR_BTN_HOVER if rect.collidepoint(mx, my) else COLOR_BTN
            pygame.draw.rect(self.screen, col, rect, border_radius=10)
            pygame.draw.rect(self.screen, (0,0,0), rect, 2, border_radius=10)
            t_surf = self.font_ui.render(txt, True, COLOR_BTN_TEXT)
            self.screen.blit(t_surf, t_surf.get_rect(center=rect.center))

    # --- RYSOWANIE GRY ---
    def draw_game(self, game_state):
        self.screen.fill(COLOR_BG)
        # Tło ręki
        pygame.draw.rect(self.screen, COLOR_HAND_BG, (0, self.table_area_h, self.screen.get_width(), self.hand_area_h))
        
        # Siatki
        self._draw_grid(GRID_ROWS_TABLE, self.table_grid_start_y, (40, 80, 40))
        self._draw_grid(GRID_ROWS_HAND, self.hand_grid_start_y, (70, 70, 70))

        # Info nagłówek (Używamy nicku jeśli dostępny)
        if game_state.my_turn:
            turn_txt = "TWOJA TURA"
            col = (255, 255, 0)
        else:
            # Pobieramy nick aktualnego gracza
            curr_pid = game_state.current_turn_pid
            curr_name = game_state.player_names.get(curr_pid, f"Gracz {curr_pid}")
            turn_txt = f"TURA: {curr_name}"
            col = (200, 200, 200)
            
        self.screen.blit(self.font.render(turn_txt, True, col), (10, 10))

        # Lista graczy (Liczniki)
        y_off = 10
        # Sortujemy po ID, żeby lista nie skakała
        for pid in sorted(game_state.player_counts.keys()):
            if pid == game_state.my_player_id: continue # Siebie nie wyświetlamy w licznikach
            
            count = game_state.player_counts[pid]
            name = game_state.player_names.get(pid, f"P{pid}") # Pobranie nicku
            
            is_active = (pid == game_state.current_turn_pid)
            col_txt = (255, 100, 100) if is_active else (255, 255, 255)
            
            # Format: "> Batman: 14"
            display_str = f"{'> ' if is_active else ''}{name}: {count}"
            
            lbl = self.font.render(display_str, True, col_txt)
            self.screen.blit(lbl, lbl.get_rect(topright=(self.screen.get_width()-20, y_off)))
            y_off += 30

        # Klocki
        for t in game_state.table_tiles + game_state.hand_tiles:
            if t.location != 'dragging': self.draw_tile(t, game_state)
        if game_state.dragging_tile:
            self.draw_tile(game_state.dragging_tile, game_state)

    def _draw_grid(self, rows, start_y, color):
        for y in range(rows):
            for x in range(GRID_COLS):
                r = pygame.Rect(self.grid_start_x + x*self.cell_w, start_y + y*self.cell_h, self.cell_w, self.cell_h)
                pygame.draw.rect(self.screen, color, r, 1)

    def draw_tile(self, t, game_state):
        if t.location == 'dragging': x, y = t.rect.x, t.rect.y
        elif t.location == 'table': x = self.grid_start_x + t.grid_x*self.cell_w; y = self.table_grid_start_y + t.grid_y*self.cell_h
        else: x = self.grid_start_x + t.grid_x*self.cell_w; y = self.hand_grid_start_y + t.grid_y*self.cell_h
        
        t.rect = pygame.Rect(x, y, self.cell_w, self.cell_h)
        
        bg = COLOR_TILE_NORMAL
        if t.location == 'table':
            if t.id in game_state.locked_ids: bg = COLOR_TILE_LOCKED
            elif not game_state.my_turn: bg = COLOR_TILE_OPP
        
        pygame.draw.rect(self.screen, (50, 50, 50), (x+3, y+3, self.cell_w, self.cell_h), border_radius=4)
        pygame.draw.rect(self.screen, bg, (x, y, self.cell_w, self.cell_h), border_radius=4)
        pygame.draw.rect(self.screen, (0,0,0), (x, y, self.cell_w, self.cell_h), 1, border_radius=4)
        
        val_s = str(t.value) if t.value > 0 else "J"
        col = RUMMI_COLORS.get(t.color_code, (0,0,0))
        txt = self.font.render(val_s, True, col)
        self.screen.blit(txt, txt.get_rect(center=(x+self.cell_w//2, y+self.cell_h//2)))

    # --- RYSOWANIE GAME OVER ---
    def draw_game_over(self, result_msg):
        self.screen.fill((20, 20, 20))
        title = self.font_title.render("KONIEC GRY", True, (255, 50, 50))
        self.screen.blit(title, title.get_rect(center=(self.screen.get_width()//2, self.screen.get_height()//2 - 100)))
        
        res_font = pygame.font.SysFont('Arial', 36)
        for i, line in enumerate(result_msg.split('|')):
            l_surf = res_font.render(line.strip(), True, (255, 255, 255))
            self.screen.blit(l_surf, l_surf.get_rect(center=(self.screen.get_width()//2, self.screen.get_height()//2 + i*40)))
            
        col = COLOR_BTN_HOVER if self.rect_back_btn.collidepoint(pygame.mouse.get_pos()) else COLOR_BTN
        pygame.draw.rect(self.screen, col, self.rect_back_btn, border_radius=10)
        pygame.draw.rect(self.screen, (255, 255, 255), self.rect_back_btn, 2, border_radius=10)
        b_txt = self.font_ui.render("Wróć do Lobby", True, COLOR_BTN_TEXT)
        self.screen.blit(b_txt, b_txt.get_rect(center=self.rect_back_btn.center))