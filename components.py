import pygame
import random


class Ground:
    ground_level  = 500
    _EARTH_COLOUR = (222, 216, 149)

    def __init__(self, win_width):
        self.win_width = win_width
        self.y = Ground.ground_level
        # Collision rect used by player.ground_collision()
        self.rect = pygame.Rect(0, Ground.ground_level, win_width, 220)
        # Cumulative scroll counter
        self._scroll = 0

    def draw(self, window):
        import config
        img   = config.base_img
        img_w = img.get_width()


        self._scroll = (self._scroll + 2) % img_w


        x = -self._scroll
        while x < self.win_width:
            window.blit(img, (x, self.y))
            x += img_w


        strip_bottom = self.y + img.get_height()
        if strip_bottom < config.win_height:
            pygame.draw.rect(
                window, Ground._EARTH_COLOUR,
                pygame.Rect(0, strip_bottom,
                            config.win_width, config.win_height - strip_bottom)
            )


class Pipes:

    width   = 80
    opening = 100

    def __init__(self, win_width):
        self.x = win_width
        self.bottom_height = random.randint(80, 250)
        self.top_height    = Ground.ground_level - self.bottom_height - self.opening
        # Collision rects updated every frame in draw()
        self.bottom_rect = pygame.Rect(0, 0, 0, 0)
        self.top_rect    = pygame.Rect(0, 0, 0, 0)
        self.passed    = False
        self.off_screen = False

    def draw(self, window):
        import config  # lazy import


        top_sprite_y = self.top_height - config.PIPE_H
        window.blit(config.pipe_top_img, (self.x, top_sprite_y))


        bottom_sprite_y = Ground.ground_level - self.bottom_height
        window.blit(config.pipe_bottom_img, (self.x, bottom_sprite_y))


        self.top_rect = pygame.Rect(
            self.x, 0,
            Pipes.width, self.top_height
        )
        self.bottom_rect = pygame.Rect(
            self.x, Ground.ground_level - self.bottom_height,
            Pipes.width, self.bottom_height
        )

    def update(self):
        self.x -= 2
        if self.x + Pipes.width <= 50:
            self.passed = True
        if self.x <= -Pipes.width:
            self.off_screen = True
