# tile.py
import pygame

class Tile:
    def __init__(self, tid, color_code, value, location='hand'):
        self.id = tid
        self.color_code = color_code
        self.value = value
        
        # Współrzędne w siatce (zależne od location)
        # location='table' -> grid_y 0..6
        # location='hand'  -> grid_y 0..1
        self.grid_x = 0
        self.grid_y = 0
        
        # 'hand', 'table', 'dragging'
        self.location = location 
        self.orig_loc = location # Do cofania ruchu
        
        # Rect do rysowania i kolizji myszy
        self.rect = pygame.Rect(0, 0, 0, 0)

    def is_joker(self):
        return self.color_code == 4 or self.value == 0

    def __repr__(self):
        return f"Tile({self.id}, {self.color_code}:{self.value} @ {self.location}[{self.grid_x},{self.grid_y}])"