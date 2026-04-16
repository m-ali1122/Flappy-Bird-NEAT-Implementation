import pygame
from sys import exit
import config
import components
import population

clock = pygame.time.Clock()
population = population.Population(500)

# HUD font – use the bundled TTF for a consistent look
import os
_font_path = os.path.join(config.ASSETS, "ARIAL.TTF")
font = pygame.font.Font(_font_path, 16) if os.path.exists(_font_path) else pygame.font.SysFont("Arial", 16)

# Speed control
simulation_speed = 1  # default speed multiplier


def generate_pipes():
    config.pipes.append(components.Pipes(config.win_width))


def quit_game():
    global simulation_speed
    for event in pygame.event.get():
        if event.type == pygame.QUIT:
            pygame.quit()
            exit()
        elif event.type == pygame.KEYDOWN:
            if event.key == pygame.K_1:
                simulation_speed = 1
            elif event.key == pygame.K_2:
                simulation_speed = 2
            elif event.key == pygame.K_3:
                simulation_speed = 5
            elif event.key == pygame.K_4:
                simulation_speed = 10


def display_stats(window, pop):
    # Semi-transparent dark panel so text is readable over the background
    panel = pygame.Surface((200, 110), pygame.SRCALPHA)
    panel.fill((0, 0, 0, 140))
    window.blit(panel, (5, 5))

    gen_text   = font.render(f"Generation: {pop.generation}",         True, (255, 255, 255))
    alive_count = sum(1 for p in pop.players if p.alive)
    alive_text = font.render(f"Birds Alive: {alive_count}/{len(pop.players)}", True, (255, 255, 255))
    sp_text    = font.render(f"Species: {len(pop.species)}",           True, (255, 255, 255))
    spd_text   = font.render(f"Speed: {simulation_speed}x  (Keys 1-4)", True, (255, 255, 255))

    window.blit(gen_text,   (10, 10))
    window.blit(alive_text, (10, 30))
    window.blit(sp_text,    (10, 50))

    if pop.species and pop.generation > 1:
        best_fitness = max(s.benchmark_fitness for s in pop.species)
        fit_text = font.render(f"Best Fitness: {best_fitness:.0f}", True, (255, 255, 255))
        window.blit(fit_text, (10, 70))

    window.blit(spd_text, (10, 90))


def main():
    pipes_spawn_time = 10

    while True:
        quit_game()

        for _ in range(simulation_speed):
            # 1. Background
            config.window.blit(config.background, (0, 0))

            # 2. Pipes (drawn before ground so ground overlaps pipe bottoms)
            if pipes_spawn_time <= 0:
                generate_pipes()
                pipes_spawn_time = 200
            pipes_spawn_time -= 1

            for p in config.pipes:
                p.draw(config.window)
                p.update()
                if p.off_screen:
                    config.pipes.remove(p)

            # 3. Ground / base (drawn on top of pipes)
            config.ground.draw(config.window)

            # 4. Birds
            if not population.extinct():
                population.update_live_players()
            else:
                config.pipes.clear()
                population.natural_selection()
                pipes_spawn_time = 10
                break  # restart the speed loop for the new generation

        # HUD drawn once per rendered frame
        display_stats(config.window, population)

        clock.tick(60)
        pygame.display.flip()


main()
