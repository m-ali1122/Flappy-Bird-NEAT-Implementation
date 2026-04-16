import brain
import pygame
import config


class Player:
    def __init__(self):
        # Bird physics / state
        self.x, self.y = 50, 200
        self.rect  = pygame.Rect(self.x, self.y, config.BIRD_W, config.BIRD_H)
        self.vel   = 0
        self.flap  = False
        self.alive = True
        self.lifespan = 0

        # AI
        self.decision = None
        self.vision   = [0.5, 1, 0.5, 0]
        self.fitness  = 0
        self.inputs   = 4
        self.brain    = brain.Brain(self.inputs)
        self.brain.generate_net()

    # ── Drawing ───────────────────────────────────────────────────────────────
    def draw(self, window, is_best=False):
        # Best bird = yellow, rest = blue
        frames = config.bird_frames_yellow if is_best else config.bird_frames_blue

        # Choose animation frame from velocity
        if self.vel < -2:
            frame_idx = 2   # upflap
        elif self.vel > 2:
            frame_idx = 0   # downflap
        else:
            frame_idx = 1   # midflap

        img = frames[frame_idx]


        angle   = max(-25, min(25, -self.vel * 3))
        rotated = pygame.transform.rotate(img, angle)

        # Centre the rotated surface over the original rect
        offset_x = (rotated.get_width()  - img.get_width())  // 2
        offset_y = (rotated.get_height() - img.get_height()) // 2
        window.blit(rotated, (self.rect.x - offset_x, self.rect.y - offset_y))


    def ground_collision(self, ground):
        return pygame.Rect.colliderect(self.rect, ground)

    def sky_collision(self):
        return bool(self.rect.y < 30)

    def pipe_collision(self):
        for p in config.pipes:
            return pygame.Rect.colliderect(self.rect, p.top_rect) or \
                   pygame.Rect.colliderect(self.rect, p.bottom_rect)


    def update(self, ground):
        if not (self.ground_collision(ground) or self.pipe_collision()):
            # Gravity
            self.vel += 0.2
            self.rect.y += self.vel
            if self.vel > 4:
                self.vel = 4
            self.lifespan += 1
        else:
            self.alive = False
            self.flap  = False
            self.vel   = 0

    def bird_flap(self):
        if not self.flap and not self.sky_collision():
            self.flap = True
            self.vel  = -4
        if self.vel >= 2:
            self.flap = False

    @staticmethod
    def closest_pipe():
        for p in config.pipes:
            if not p.passed:
                return p


    def look(self):
        if config.pipes:
            closest = self.closest_pipe()

            # Distance to bottom of top pipe
            self.vision[0] = max(0, self.rect.center[1] - closest.top_rect.bottom) / 500
            pygame.draw.line(config.window, (255, 255, 0), self.rect.center,
                             (self.rect.center[0], config.pipes[0].top_rect.bottom))

            # Horizontal distance to next pipe
            self.vision[1] = max(0, closest.x - self.rect.center[0]) / 500
            pygame.draw.line(config.window, (255, 255, 0), self.rect.center,
                             (config.pipes[0].x, self.rect.center[1]))

            # Distance to top of bottom pipe
            self.vision[2] = max(0, closest.bottom_rect.top - self.rect.center[1]) / 500
            pygame.draw.line(config.window, (255, 255, 0), self.rect.center,
                             (self.rect.center[0], config.pipes[0].bottom_rect.top))

            # Normalised velocity
            self.vision[3] = 0.5 + (self.vel / 10)

    def think(self):
        self.decision = self.brain.feed_forward(self.vision)
        if self.decision > 0.6:
            self.bird_flap()

    # Fitness
    def calculate_fitness(self):
        self.fitness = self.lifespan * self.lifespan

        pipes_passed = 0
        for p in config.pipes:
            if p.passed and p.x < self.rect.x:
                pipes_passed += 1
                self.fitness += 500

        if config.pipes and self.alive:
            closest_pipe = self.closest_pipe()
            if closest_pipe:
                pipe_center_y       = closest_pipe.top_height + (closest_pipe.opening / 2)
                distance_from_center = abs(self.rect.center[1] - pipe_center_y)
                positioning_bonus   = max(0, 300 - distance_from_center)
                self.fitness += positioning_bonus

        if pipes_passed > 0:
            self.fitness += (pipes_passed * pipes_passed) * 200

    def clone(self):
        clone         = Player()
        clone.fitness = self.fitness
        clone.brain   = self.brain.clone()
        clone.brain.generate_net()
        return clone
