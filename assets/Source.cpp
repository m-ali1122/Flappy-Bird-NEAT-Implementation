#include <SFML/Graphics.hpp>
#include <SFML/Window.hpp>
#include <SFML/Audio.hpp>  // For sound support
#include <vector>
#include <cstdlib>
#include <ctime>
#include <iostream>
#include <sstream>
#include <algorithm>
#include <fstream>   // For file I/O


// ---------------------------------------------------------------------------
// Constants
// ---------------------------------------------------------------------------

static const int WINDOW_WIDTH = 288;
static const int WINDOW_HEIGHT = 512;

// Bird physics
static const float GRAVITY = 800.f;
static const float FLAP_VELOCITY = -250.f;

// Pipe settings
static const float PIPE_SPEED = 100.f;
static const float PIPE_SPAWN_TIME = 1.5f;
static const float PIPE_GAP_HEIGHT = 100.f;   // vertical gap for the bird

// Base (ground) scrolling speed
static const float BASE_SCROLL_SPEED = 100.f;

// ---------------------------------------------------------------------------
// Game States
// ---------------------------------------------------------------------------
enum class GameState {
    WAITING,   // showing "Get Ready" message
    PLAYING,   // main game loop
    GAME_OVER  // showing "Game Over" screen
};

// ---------------------------------------------------------------------------
// Bird class (handles animation frames, position, collision)
// ---------------------------------------------------------------------------
class Bird {
public:
    Bird() : velocity(0.f), currentFrame(0), animationTime(0.f) {}

    void addFrame(const sf::Texture& texture) {
        frames.push_back(texture);
    }

    void init(const sf::Vector2f& position) {
        sprite.setTexture(frames[0]);
        sprite.setPosition(position);
        auto bounds = sprite.getLocalBounds();
        sprite.setOrigin(bounds.width / 2.f, bounds.height / 2.f);
    }

    void update(float dt) {
        velocity += GRAVITY * dt;
        sprite.move(0.f, velocity * dt);
        float angle = std::min(velocity * 0.1f, 25.f);
        sprite.setRotation(angle);
        animationTime += dt;
        if (animationTime >= 0.1f) {
            animationTime = 0.f;
            currentFrame = (currentFrame + 1) % frames.size();
            sprite.setTexture(frames[currentFrame]);
        }
    }

    void flap() {
        velocity = FLAP_VELOCITY;
    }

    void reset(const sf::Vector2f& position) {
        velocity = 0.f;
        sprite.setRotation(0.f);
        sprite.setPosition(position);
        currentFrame = 0;
        sprite.setTexture(frames[0]);
    }

    // Improved collision bounds using a percentage margin

    sf::FloatRect getBounds() const 
    {
        sf::FloatRect box = sprite.getGlobalBounds();
        float marginX = box.width * 0.2f;   // 20% margin on each side
        float marginY = box.height * 0.2f;  // 20% margin on top and bottom
        box.left += marginX;
        box.width -= 2 * marginX;
        box.top += marginY;
        box.height -= 2 * marginY;
        return box;
    }

    sf::Sprite& getSprite() { return sprite; }

private:
    sf::Sprite sprite;
    std::vector<sf::Texture> frames;
    float velocity;
    int currentFrame;
    float animationTime;
};

// ---------------------------------------------------------------------------
// Pipe class (two sprites: top and bottom pipe)
// ---------------------------------------------------------------------------
class Pipe {
public:
    // Pass separate textures for top and bottom pipes.
    Pipe(const sf::Texture& topTexture,
        const sf::Texture& bottomTexture,
        float startX,
        float gapY)
    {
        // --- Top Pipe ---
        topSprite.setTexture(topTexture);
        sf::Vector2u topSize = topTexture.getSize();
        float topWidth = static_cast<float>(topSize.x);
        float topHeight = static_cast<float>(topSize.y);
        // Set origin to bottom-center so the opening is at the bottom.
        topSprite.setOrigin(topWidth / 2.f, topHeight);
        // Position so that its bottom edge aligns with gapY.
        topSprite.setPosition(startX + topWidth / 2.f, gapY);

        // --- Bottom Pipe ---
        bottomSprite.setTexture(bottomTexture);
        sf::Vector2u bottomSize = bottomTexture.getSize();
        float bottomWidth = static_cast<float>(bottomSize.x);
        bottomSprite.setOrigin(bottomWidth / 2.f, 0.f);
        // Position so that its top edge aligns with gapY + PIPE_GAP_HEIGHT.
        bottomSprite.setPosition(startX + bottomWidth / 2.f, gapY + PIPE_GAP_HEIGHT);

        scored = false;  // Initialize scoring flag for this pipe.
    }

    void update(float dt) {
        topSprite.move(-PIPE_SPEED * dt, 0.f);
        bottomSprite.move(-PIPE_SPEED * dt, 0.f);
    }

    void draw(sf::RenderWindow& window) {
        window.draw(topSprite);
        window.draw(bottomSprite);
    }

    bool isOffScreen() const {
        return (topSprite.getPosition().x + topSprite.getGlobalBounds().width < 0);
    }

    sf::FloatRect getTopBounds() const {
        sf::FloatRect box = topSprite.getGlobalBounds();
        box.left += 2;
        box.width -= 4;
        box.top += 2;
        box.height -= 2;
        return box;
    }
    sf::FloatRect getBottomBounds() const {
        sf::FloatRect box = bottomSprite.getGlobalBounds();
        box.left += 2;
        box.width -= 4;
        box.height -= 2;
        return box;
    }

    // Public member to track if this pipe has been scored already.
    bool scored;

private:
    sf::Sprite topSprite;
    sf::Sprite bottomSprite;
};

// ---------------------------------------------------------------------------
// Scrolling Base class
// ---------------------------------------------------------------------------
class Base {
public:
    Base(const sf::Texture& baseTexture, float yPos) {
        sprite1.setTexture(baseTexture);
        sprite2.setTexture(baseTexture);
        sprite1.setPosition(0.f, yPos);
        sprite2.setPosition(sprite1.getGlobalBounds().width, yPos);
    }

    void update(float dt) {
        sprite1.move(-BASE_SCROLL_SPEED * dt, 0.f);
        sprite2.move(-BASE_SCROLL_SPEED * dt, 0.f);

        if (sprite1.getPosition().x + sprite1.getGlobalBounds().width < 0)
            sprite1.setPosition(sprite2.getPosition().x + sprite2.getGlobalBounds().width, sprite1.getPosition().y);


        if (sprite2.getPosition().x + sprite2.getGlobalBounds().width < 0)
            sprite2.setPosition(sprite1.getPosition().x + sprite1.getGlobalBounds().width, sprite2.getPosition().y);
    }

    void draw(sf::RenderWindow& window) 
    {
        window.draw(sprite1);
        window.draw(sprite2);
    }

private:
    sf::Sprite sprite1;
    sf::Sprite sprite2;
};

// ---------------------------------------------------------------------------
// Scoreboard (using digit textures 0-7)
// ---------------------------------------------------------------------------
 // ---------------------------------------------------------------------------
// Scoreboard (using digit textures 0-9)
// ---------------------------------------------------------------------------
class Scoreboard {
public:
    bool loadDigitTextures() {
       
        for (int i = 0; i < 10; ++i) 
        {
            sf::Texture texture;
            if (!texture.loadFromFile(std::to_string(i) + ".png")) {
                std::cerr << "Failed to load digit " << i << ".png" << std::endl;
                return false;
            }
            digitTextures.push_back(texture);
        }
        return true;
    }

    void draw(sf::RenderWindow& window, int score) {
        std::string scoreStr = std::to_string(score);
        float totalWidth = 0.f;
        std::vector<sf::Sprite> sprites;
        // Use the size of digitTextures for the valid digit range.
        for (char c : scoreStr) {
            int digit = c - '0';
            if (digit < 0 || digit >= static_cast<int>(digitTextures.size()))
                continue;
            sf::Sprite s(digitTextures[digit]);
            sprites.push_back(s);
            totalWidth += s.getGlobalBounds().width;
        }
        float startX = (WINDOW_WIDTH - totalWidth) / 2.f;
        float yPos = 20.f;
        for (auto& s : sprites) {
            s.setPosition(startX, yPos);
            window.draw(s);
            startX += s.getGlobalBounds().width;
        }
    }

private:
    std::vector<sf::Texture> digitTextures;
};


// ---------------------------------------------------------------------------
// Main Game
// ---------------------------------------------------------------------------
int main() {
    sf::RenderWindow window(sf::VideoMode(WINDOW_WIDTH, WINDOW_HEIGHT), "Flappy Bird");
    window.setFramerateLimit(400);

    std::srand(static_cast<unsigned>(std::time(nullptr)));

    // -----------------------------------------------------------------------
    // Load Textures
    // -----------------------------------------------------------------------
    // Load both day and night backgrounds.
    sf::Texture backgroundDayTexture;
    if (!backgroundDayTexture.loadFromFile("background-day.png")) {
        std::cerr << "Error loading background-day.png" << std::endl;
        return -1;
    }
    sf::Texture backgroundNightTexture;
    if (!backgroundNightTexture.loadFromFile("background-night.png")) {
        std::cerr << "Error loading background-night.png" << std::endl;
        return -1;
    }
    // Start with night background.
    bool isDay = false;
    sf::Sprite backgroundSprite;
    backgroundSprite.setTexture(backgroundNightTexture);

    sf::Texture baseTexture;
    if (!baseTexture.loadFromFile("base.png")) {
        std::cerr << "Error loading base.png" << std::endl;
        return -1;
    }

    // Bird frames (yellow bird)
    sf::Texture yellowDown, yellowMid, yellowUp;
    if (!yellowDown.loadFromFile("yellowbird-downflap.png") ||
        !yellowMid.loadFromFile("yellowbird-midflap.png") ||
        !yellowUp.loadFromFile("yellowbird-upflap.png")) {
        std::cerr << "Error loading one of the yellowbird frames" << std::endl;
        return -1;
    }

    // Load separate pipe textures for top and bottom for each color
    sf::Texture pipeGreenTop, pipeGreenBottom;
    if (!pipeGreenTop.loadFromFile("pipe-green-top.png")) {
        std::cerr << "Error loading pipe-green-top.png" << std::endl;
        return -1;
    }
    if (!pipeGreenBottom.loadFromFile("pipe-green-bottom.png")) {
        std::cerr << "Error loading pipe-green-bottom.png" << std::endl;
        return -1;
    }

    sf::Texture pipeRedTop, pipeRedBottom;
    if (!pipeRedTop.loadFromFile("pipe-red-top.png")) {
        std::cerr << "Error loading pipe-red-top.png" << std::endl;
        return -1;
    }
    if (!pipeRedBottom.loadFromFile("pipe-red-bottom.png")) {
        std::cerr << "Error loading pipe-red-bottom.png" << std::endl;
        return -1;
    }

    // UI textures
    sf::Texture gameOverTexture, messageTexture;
    if (!gameOverTexture.loadFromFile("gameover.png")) {
        std::cerr << "Error loading gameover.png" << std::endl;
        return -1;
    }
    if (!messageTexture.loadFromFile("message.png")) {
        std::cerr << "Error loading message.png" << std::endl;
        return -1;
    }

    // -----------------------------------------------------------------------
    // Load Sounds
    // -----------------------------------------------------------------------
    sf::SoundBuffer flapBuffer;
    if (!flapBuffer.loadFromFile("flap.mp3")) {
        std::cerr << "Error loading flap.mp3" << std::endl;
        return -1;
    }
    sf::Sound flapSound;
    flapSound.setBuffer(flapBuffer);

    sf::SoundBuffer dieBuffer;
    if (!dieBuffer.loadFromFile("die.mp3")) {
        std::cerr << "Error loading die.mp3" << std::endl;
        return -1;
    }
    sf::Sound dieSound;
    dieSound.setBuffer(dieBuffer);

    sf::SoundBuffer hitBuffer;
    if (!hitBuffer.loadFromFile("hit.mp3")) {
        std::cerr << "Error loading hit.mp3" << std::endl;
        return -1;
    }
    sf::Sound hitSound;
    hitSound.setBuffer(hitBuffer);

    sf::SoundBuffer pointBuffer;
    if (!pointBuffer.loadFromFile("point.mp3")) {
        std::cerr << "Error loading point.mp3" << std::endl;
        return -1;
    }
    sf::Sound pointSound;
    pointSound.setBuffer(pointBuffer);

    sf::SoundBuffer swooshBuffer;
    if (!swooshBuffer.loadFromFile("swoosh.mp3")) {
        std::cerr << "Error loading swoosh.mp3" << std::endl;
        return -1;
    }
    sf::Sound swooshSound;
    swooshSound.setBuffer(swooshBuffer);

    // Background Music
    sf::Music backgroundMusic;
    if (!backgroundMusic.openFromFile("background_music.ogg")) {
        std::cerr << "Error loading background_music.ogg" << std::endl;
    }
    backgroundMusic.setLoop(true);
    backgroundMusic.play();

    // -----------------------------------------------------------------------
    // Highscore Setup
    // -----------------------------------------------------------------------
    int highScore = 0;
    std::ifstream highScoreFile("highscore.txt");
    if (highScoreFile.is_open()) {
        highScoreFile >> highScore;
        highScoreFile.close();
    }
    bool highScoreUpdated = false;

    // Load font for highscore display.
    sf::Font font;
    if (!font.loadFromFile("ARIAL.ttf")) {
        std::cerr << "Error loading ARIAL.ttf" << std::endl;
        return -1;
    }
    sf::Text highScoreText;
    highScoreText.setFont(font);
    highScoreText.setCharacterSize(20);
    highScoreText.setFillColor(sf::Color::White);
    highScoreText.setStyle(sf::Text::Bold);

    // Create a background rectangle for the highscore text.
    sf::RectangleShape highScoreBackground;

    // -----------------------------------------------------------------------
    // Scoreboard
    // -----------------------------------------------------------------------
    Scoreboard scoreboard;
    if (!scoreboard.loadDigitTextures()) {
        std::cerr << "Digit textures partially loaded. 8 & 9 are missing." << std::endl;
    }

    // Create base (ground) object
    float baseY = WINDOW_HEIGHT - baseTexture.getSize().y;
    Base base(baseTexture, baseY);

    // -----------------------------------------------------------------------
    // Bird Setup
    // -----------------------------------------------------------------------
    Bird bird;
    bird.addFrame(yellowDown);
    bird.addFrame(yellowMid);
    bird.addFrame(yellowUp);
    bird.init({ WINDOW_WIDTH * 0.3f, WINDOW_HEIGHT * 0.5f });

    // -----------------------------------------------------------------------
    // Pipe Vector
    // -----------------------------------------------------------------------
    std::vector<Pipe> pipes;
    float pipeSpawnTimer = 0.f;

    // -----------------------------------------------------------------------
    // UI Sprites
    // -----------------------------------------------------------------------
    sf::Sprite gameOverSprite(gameOverTexture);
    gameOverSprite.setPosition((WINDOW_WIDTH - gameOverSprite.getGlobalBounds().width) / 2.f,
        WINDOW_HEIGHT * 0.25f);
    sf::Sprite messageSprite(messageTexture);
    messageSprite.setPosition((WINDOW_WIDTH - messageSprite.getGlobalBounds().width) / 2.f,
        WINDOW_HEIGHT * 0.1f);

    // -----------------------------------------------------------------------
    // Game Variables
    // -----------------------------------------------------------------------
    GameState gameState = GameState::WAITING;
    int score = 0;
    sf::Clock clock;

    // -----------------------------------------------------------------------
    // Main Loop
    // -----------------------------------------------------------------------
    while (window.isOpen()) {
        float dt = clock.restart().asSeconds();

        sf::Event event;
        while (window.pollEvent(event)) {
            if (event.type == sf::Event::Closed)
                window.close();

            if (event.type == sf::Event::KeyPressed) {
                // Toggle background with the B key.
                if (event.key.code == sf::Keyboard::B) {
                    isDay = !isDay;
                    if (isDay)
                        backgroundSprite.setTexture(backgroundDayTexture);
                    else
                        backgroundSprite.setTexture(backgroundNightTexture);
                }
                if (event.key.code == sf::Keyboard::Space) {
                    if (gameState == GameState::PLAYING) {
                        bird.flap();
                        flapSound.play();
                    }
                    else if (gameState == GameState::WAITING) {
                        gameState = GameState::PLAYING;
                        bird.flap();
                        flapSound.play();
                    }
                }
                if (event.key.code == sf::Keyboard::R && gameState == GameState::GAME_OVER) {
                    gameState = GameState::WAITING;
                    score = 0;
                    pipes.clear();
                    bird.reset({ WINDOW_WIDTH * 0.3f, WINDOW_HEIGHT * 0.5f });
                    highScoreUpdated = false;
                }
            }
        }

        if (gameState == GameState::PLAYING) {
            bird.update(dt);
            base.update(dt);

            pipeSpawnTimer += dt;
            if (pipeSpawnTimer >= PIPE_SPAWN_TIME) {
                pipeSpawnTimer = 0.f;
                bool useGreen = (std::rand() % 2 == 0);
                float minY = 100.f;
                float maxY = baseY - 100.f;
                float gapY = minY + static_cast<float>(std::rand()) / RAND_MAX * (maxY - minY);
                if (useGreen)
                    pipes.emplace_back(pipeGreenTop, pipeGreenBottom, static_cast<float>(WINDOW_WIDTH), gapY);
                else
                    pipes.emplace_back(pipeRedTop, pipeRedBottom, static_cast<float>(WINDOW_WIDTH), gapY);
            }

            for (auto& pipe : pipes)
                pipe.update(dt);

            // Scoring: check each pipe once its center passes the bird.
            for (auto& pipe : pipes) {
                if (!pipe.scored && (pipe.getTopBounds().left + pipe.getTopBounds().width / 2.f < bird.getSprite().getPosition().x)) {
                    pipe.scored = true;
                    score++;
                    pointSound.play();
                }
            }

            // Remove pipes that are off-screen.
            pipes.erase(std::remove_if(pipes.begin(), pipes.end(), [](const Pipe& pipe) {
                return pipe.isOffScreen();
                }), pipes.end());

            sf::FloatRect birdBounds = bird.getBounds();
            for (auto& pipe : pipes) {
                if (birdBounds.intersects(pipe.getTopBounds()) ||
                    birdBounds.intersects(pipe.getBottomBounds())) {
                    gameState = GameState::GAME_OVER;
                }
            }

            if (bird.getSprite().getPosition().y + bird.getSprite().getGlobalBounds().height / 2.f >= baseY)
                gameState = GameState::GAME_OVER;
            if (bird.getSprite().getPosition().y - bird.getSprite().getGlobalBounds().height / 2.f <= 0.f)
                gameState = GameState::GAME_OVER;
        }
        else if (gameState == GameState::WAITING) {
            base.update(dt);
        }
        else if (gameState == GameState::GAME_OVER) {
            base.update(dt);
            if (!highScoreUpdated) {
                if (score > highScore) {
                    highScore = score;
                    std::ofstream ofs("highscore.txt");
                    if (ofs.is_open()) {
                        ofs << highScore;
                        ofs.close();
                    }
                }
                highScoreUpdated = true;
                // Prepare highscore text with bold style.
                highScoreText.setString("High Score: " + std::to_string(highScore));
                sf::FloatRect textBounds = highScoreText.getLocalBounds();
                // Set up background rectangle with some padding.
                highScoreBackground.setSize(sf::Vector2f(textBounds.width + 20, textBounds.height + 20));
                highScoreBackground.setFillColor(sf::Color::Black);
                highScoreBackground.setOutlineColor(sf::Color::White);
                highScoreBackground.setOutlineThickness(2.f);
                // Center the background and text.
                highScoreBackground.setPosition((WINDOW_WIDTH - highScoreBackground.getSize().x) / 2.f,
                    WINDOW_HEIGHT * 0.75f - 10);
                highScoreText.setPosition(highScoreBackground.getPosition().x + 10, highScoreBackground.getPosition().y + 10);
            }
        }

        window.clear();
        window.draw(backgroundSprite);
        for (auto& pipe : pipes)
            pipe.draw(window);
        base.draw(window);
        window.draw(bird.getSprite());
        if (gameState == GameState::WAITING)
            window.draw(messageSprite);
        else if (gameState == GameState::GAME_OVER) {
            window.draw(gameOverSprite);
            // Draw the highscore background and text.
            window.draw(highScoreBackground);
            window.draw(highScoreText);
        }
        if (gameState == GameState::PLAYING || gameState == GameState::GAME_OVER)
            scoreboard.draw(window, score);
        window.display();
    }

    return 0;
}
