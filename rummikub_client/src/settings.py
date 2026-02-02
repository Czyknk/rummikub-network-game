# settings.py

# --- KONFIGURACJA SIECI ---
HOST = '127.0.0.1'
PORT = 1100

# --- WYMIARY SIATKI ---
GRID_COLS = 24
GRID_ROWS_TABLE = 7
GRID_ROWS_HAND = 2 

# --- OKNO ---
WINDOW_WIDTH = 1280
WINDOW_HEIGHT = 800

# --- STANY GRY ---
STATE_LOBBY = "LOBBY"
STATE_GAME = "GAME"
STATE_GAME_OVER = "GAME_OVER"

# --- KOLORY INTERFEJSU ---
COLOR_BG_UI = (15, 15, 15) 
COLOR_BTN = (70, 70, 70)
COLOR_BTN_HOVER = (90, 90, 90)
COLOR_BTN_TEXT = (255, 255, 255)
COLOR_INPUT_BG = (20, 20, 20)
COLOR_INPUT_BORDER = (100, 100, 100)
COLOR_INPUT_ACTIVE = (0, 200, 0) # Zielona ramka gdy aktywne
COLOR_TITLE = (255, 165, 0)      # Pomarańczowy

# --- KOLORY STOŁU ---
COLOR_BG = (30, 100, 30)         # Tło stołu 
COLOR_HAND_BG = (50, 50, 50)     # Tło obszaru ręki (dla odróżnienia)
COLOR_GRID_LINE = (5, 5, 5)     # Kolor linii siatki na stole
COLOR_GRID_LINE_HAND = (70, 70, 70) # Kolor linii siatki w ręce

# --- KOLORY KLOCKÓW ---
COLOR_TILE_NORMAL = (245, 245, 220)
COLOR_TILE_LOCKED = (200, 200, 180)
COLOR_TILE_OPP = (220, 220, 255)
COLOR_TILE_UNKNOWN = (150, 150, 150)

# Mapowanie kolorów Rummikub (ID: RGB)
RUMMI_COLORS = {
    0: (200, 0, 0),    # Red
    1: (0, 0, 200),    # Blue
    2: (0, 0, 0),      # Black
    3: (255, 140, 0),  # Yellow/Orange
    4: (200, 0, 200),  # Joker
    -1: (50, 50, 50)   # Nieznany (szary tekst)
}

#liczba klocków na ręce startowo
STARTING_TILE_COUNT = 14