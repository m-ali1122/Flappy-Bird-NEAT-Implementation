import pygame
import os

pygame.init()

win_height = 720
win_width  = 550
window = pygame.display.set_mode((win_width, win_height))
pygame.display.set_caption("Flappy Bird – NEAT")

ASSETS = os.path.join(os.path.dirname(__file__), "assets")


PIPE_W  = 80    # display width of each pipe sprite
PIPE_H  = 400   # display height – tall enough to extend off-screen top/bottom
BIRD_W  = 40    # display width of bird sprite
BIRD_H  = 28    # display height of bird sprite
BASE_H  = 100   # display height of the scrolling ground strip


_GROUND_Y = 500

_sky = pygame.transform.scale(
    pygame.image.load(os.path.join(ASSETS, "background-day.png")).convert(),
    (win_width, _GROUND_Y)
)
background = pygame.Surface((win_width, win_height))
background.blit(_sky, (0, 0))

pygame.draw.rect(background, (222, 216, 149),
                 pygame.Rect(0, _GROUND_Y, win_width, win_height - _GROUND_Y))

#ground sprite
_base_raw = pygame.image.load(os.path.join(ASSETS, "base.png")).convert()
_base_w   = int(_base_raw.get_width() * BASE_H / _base_raw.get_height())
base_img  = pygame.transform.scale(_base_raw, (_base_w, BASE_H))

#Pipe sprites
pipe_top_img = pygame.transform.scale(
    pygame.image.load(os.path.join(ASSETS, "pipe-green-top.png")).convert_alpha(),
    (PIPE_W, PIPE_H)
)
pipe_bottom_img = pygame.transform.scale(
    pygame.image.load(os.path.join(ASSETS, "pipe-green-bottom.png")).convert_alpha(),
    (PIPE_W, PIPE_H)
)

# Bird sprites
def _load_bird(color):
    return [
        pygame.transform.scale(
            pygame.image.load(os.path.join(ASSETS, f"{color}bird-{frame}.png")).convert_alpha(),
            (BIRD_W, BIRD_H)
        )
        for frame in ("downflap", "midflap", "upflap")
    ]

bird_frames_yellow = _load_bird("yellow")   # best bird
bird_frames_blue   = _load_bird("blue")     # regular birds
bird_frames_red    = _load_bird("red")      # available, unused by default

# Game state
import components

ground = components.Ground(win_width)
pipes  = []

generation    = 1
species_count = 0
target_species = 10
