#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>

/*
    Written by: Alexander Tysklind
    Description:
        PongPlus is just a traditional pong game made in c++. The reason for making it is as a
        first practice project using the SFML library.
*/

namespace Vector
{
    sf::Vector2f normalize(const sf::Vector2f& vector)
    {
        float length = std::sqrt(vector.x * vector.x + vector.y * vector.y);
        if (length != 0) return sf::Vector2f(vector.x / length, vector.y / length);
        else             return sf::Vector2f(0, 0);
    }
}

float lerp(float start, float end, float t) {
    // Ensure t is clamped between 0 and 1
    t = std::max(0.0f, std::min(1.0f, t));
    // Calculate the interpolated value
    return start + (end - start) * t;
}

int main()
{
    // Seed randomization to ensure that the random numbers are not the same.
    srand(time(NULL));

    // Window & Shapes.
    sf::RenderWindow    window(sf::VideoMode(1920, 1080), "PongPlus", sf::Style::Titlebar | sf::Style::Close);
    sf::RectangleShape  paddleA(sf::Vector2f(12, 144));
    sf::RectangleShape  paddleB(sf::Vector2f(12, 144));
    sf::CircleShape     ball(10.0f);
    sf::CircleShape     ballTrail[5];

    sf::Music deflect1;
    deflect1.openFromFile("resources/audio/deflect-3.mp3");
    deflect1.setPitch(1.15f);

    sf::Music deflect2;
    deflect2.openFromFile("resources/audio/deflect-3.mp3");
    deflect2.setPitch(0.85f);
    deflect2.setVolume(50);

    sf::Music music;
    music.openFromFile("resources/audio/music-2.mp3");
    music.setLoop(true);
    music.setVolume(15);
    music.setPitch(0.75f);
    music.play();

    sf::Font font;
    font.loadFromFile("resources/fonts/PixelifySans.ttf");

    const sf::Color TEXT_COLOR = sf::Color(50, 50, 50);

    sf::Text szPaddSpeed("Paddle Speed: 0", font, 30);
    sf::Text szBallSpeed("Ball Speed: 0", font, 30);
    sf::Text szController("Controller: CPU", font, 30);
    sf::Text szToggleEsc("(Esc) Pause", font, 24);
    sf::Text szToggleP("(P) Toggle Control", font, 24);
    sf::Text szPause("PAUSED", font, 72);
    sf::Text szScoreA("0", font, 30);
    sf::Text szScoreB("0", font, 30);
    sf::Text szScoreC("SCORE", font, 50);
    szPaddSpeed.setFillColor(TEXT_COLOR);
    szBallSpeed.setFillColor(TEXT_COLOR);
    szController.setFillColor(TEXT_COLOR);
    szToggleEsc.setFillColor(sf::Color::White);
    szToggleP.setFillColor(sf::Color::White);
    szScoreA.setFillColor(sf::Color::White);
    szScoreB.setFillColor(sf::Color::White);
    szScoreC.setFillColor(sf::Color::White);

    szScoreC.setPosition(window.getSize().x / 2 - szScoreC.getLocalBounds().width / 2, 0);
    szScoreB.setPosition(szScoreC.getPosition().x + szScoreC.getLocalBounds().width + 30, 15);
    szScoreA.setPosition(szScoreC.getPosition().x - szScoreA.getLocalBounds().width - 30, 15);
    szController.setPosition(window.getSize().x / 2 - szController.getGlobalBounds().width / 2,
                             window.getSize().y / 2 - szController.getGlobalBounds().height / 2 - 54);
    szPause.setPosition(window.getSize().x / 2 - szPause.getGlobalBounds().width / 2,
                        window.getSize().y / 2 - szPause.getGlobalBounds().height / 2);
    szToggleEsc.setPosition(24, 24);
    szToggleP.setPosition(24, 60);

    

    // Time control
    sf::Clock       deltaClock;
    sf::Time        elapsedTime;

    // Game logic variables
    sf::Vector2f    directionPaddleA(0.0f, 0.0f);
    sf::Vector2f    directionPaddleB(0.0f, 0.0f);
    sf::Vector2f    directionBall(1.0f, 0.5f); // Randomize ball direction on spawn.
    float           paddleSpeed = 800.00f;
    float           ballSpeed = 500.00f;
    int             ScoreA = 0;
    int             ScoreB = 0;

    // Generic size variables.
    const float PADDLE_WIDTH = paddleA.getSize().x;
    const float PADDLE_HEIGHT = paddleA.getSize().y;

    // Set the startup locations.
    paddleA.setPosition(sf::Vector2f(PADDLE_WIDTH, (window.getSize().x / 2) - (PADDLE_HEIGHT / 2)));
    paddleB.setPosition(sf::Vector2f(window.getSize().x - (PADDLE_WIDTH * 2), paddleA.getPosition().y));
    ball.setPosition(window.getSize().x / 2 - ball.getRadius(), window.getSize().y / 2 - ball.getRadius());

    enum PlayerTurn { PADDLE_A, PADDLE_B };
    enum Direction { RIGHT = 1, LEFT = -1, UP = -1, DOWN = 1, NONE = 0 };

    bool pause = false;
    bool resetBall = false;
    PlayerTurn playerTurn = PlayerTurn::PADDLE_B;
    bool playerControl = false;

    while (window.isOpen())
    {
        // Handle delta time
        elapsedTime += deltaClock.getElapsedTime();
        sf::Time deltaTime = deltaClock.restart();

        if (!pause)
        {

            // User input for manual play.
            if (playerControl)
            {

                sf::Mouse mouse;
                int y = mouse.getPosition().y - window.getPosition().y;
                int py = paddleA.getPosition().y + PADDLE_HEIGHT / 2;

                if      (y - py == 0) directionPaddleA.y = Direction::NONE * paddleSpeed * deltaTime.asSeconds();
                else if (y - py  < 0) directionPaddleA.y = Direction::UP   * paddleSpeed * deltaTime.asSeconds();
                else if (y - py  > 0) directionPaddleA.y = Direction::DOWN * paddleSpeed * deltaTime.asSeconds();

            }
            

            // Automated play for 50% of left side.
            if (ball.getPosition().x <= window.getSize().x / 2)
            {
                float ballHorizontalCenter = ball.getPosition().y + ball.getRadius();
                float distance, maxDistance, interpolate, modifier, minimumSpeed;
                float paddleHorizontalCenter;


                minimumSpeed = lerp(0.01f, 0.65f, ballSpeed / 7500.0f);
                distance    = window.getSize().x / 2 - ball.getPosition().x; // Calculate the distance from the ball to the middle of the screen
                maxDistance = window.getSize().x / 2; // Distance from the left edge to the middle of the screen
                interpolate = distance / maxDistance; // Map distance to range [0, 1]
                modifier    = lerp(minimumSpeed, 1.0f, interpolate);

                // PaddleA updates if player is not controlling the paddle.
                if (!playerControl)
                {
                    paddleHorizontalCenter = paddleA.getPosition().y + PADDLE_HEIGHT / 2;
                    if      (paddleHorizontalCenter > ballHorizontalCenter) directionPaddleA.y = Direction::UP   * deltaTime.asSeconds() * paddleSpeed * modifier;
                    else if (paddleHorizontalCenter < ballHorizontalCenter) directionPaddleA.y = Direction::DOWN * deltaTime.asSeconds() * paddleSpeed * modifier;
                }

                // PaddleB updates.
                paddleHorizontalCenter = paddleB.getPosition().y + PADDLE_HEIGHT / 2;
                if      (paddleHorizontalCenter > ballHorizontalCenter) directionPaddleB.y = Direction::UP   * deltaTime.asSeconds() * paddleSpeed * minimumSpeed;
                else if (paddleHorizontalCenter < ballHorizontalCenter) directionPaddleB.y = Direction::DOWN * deltaTime.asSeconds() * paddleSpeed * minimumSpeed;
            }

            // Automated play for 50% of right side.
            if (ball.getPosition().x >= window.getSize().x / 2)
            {
                float ballHorizontalCenter = ball.getPosition().y + ball.getRadius();
                float distance, maxDistance, interpolate, modifier, minimumSpeed;
                float paddleHorizontalCenter;
                
                minimumSpeed = lerp(0.01f, 0.65f, ballSpeed / 7500.0f);
                distance    = ball.getPosition().x - window.getSize().x / 2; // Calculate the distance from the middle of the screen to the ball
                maxDistance = window.getSize().x / 2; // Distance from the middle of the screen to the right edge
                interpolate = distance / maxDistance; // Map distance to range [0, 1]
                modifier    = lerp(minimumSpeed, 1.0f, interpolate);

                // PaddleA updates if player is not controlling the paddle.
                if (!playerControl)
                {
                    paddleHorizontalCenter = paddleA.getPosition().y + PADDLE_HEIGHT / 2;
                    if      (paddleHorizontalCenter > ballHorizontalCenter) directionPaddleA.y = Direction::UP   * deltaTime.asSeconds() * paddleSpeed * minimumSpeed;
                    else if (paddleHorizontalCenter < ballHorizontalCenter) directionPaddleA.y = Direction::DOWN * deltaTime.asSeconds() * paddleSpeed * minimumSpeed;
                }

                // PaddleB updates.
                paddleHorizontalCenter = paddleB.getPosition().y + PADDLE_HEIGHT / 2;
                if      (paddleHorizontalCenter > ballHorizontalCenter) directionPaddleB.y = Direction::UP   * deltaTime.asSeconds() * paddleSpeed * modifier;
                else if (paddleHorizontalCenter < ballHorizontalCenter) directionPaddleB.y = Direction::DOWN * deltaTime.asSeconds() * paddleSpeed * modifier;

            }

            // Update position of the paddles.
            paddleA.setPosition(sf::Vector2f(paddleA.getPosition().x, paddleA.getPosition().y + directionPaddleA.y));
            paddleB.setPosition(sf::Vector2f(paddleB.getPosition().x, paddleB.getPosition().y + directionPaddleB.y));

            // Handle ball trails.
            for (int i = 0; i < sizeof(ballTrail) / sizeof(ballTrail[0]); ++i)
            {
                ballTrail[i].setRadius((7.0f - i));
                sf::Vector2f position;
                float fOffset   = 10;
                float invertedX = -directionBall.x;
                float invertedY = -directionBall.y;
                float trailBallRadius = ballTrail[i].getRadius();

                if (i == 0)
                {
                    ballTrail[i].setFillColor(sf::Color(225, 225, 225));
                    sf::Vector2f center(ball.getPosition().x + ball.getRadius(), ball.getPosition().y + ball.getRadius());
                    ballTrail[i].setPosition(
                        center.x - trailBallRadius + (invertedX * fOffset),
                        center.y - trailBallRadius + (invertedY * fOffset));
                }

                else
                {
                    char reduction = i * 15;
                    sf::Color c = ballTrail[i - 1].getFillColor();
                    ballTrail[i].setFillColor(sf::Color(c.r - reduction, c.g - reduction, c.b - reduction));
                    sf::Vector2f center(
                        ballTrail[i - 1].getPosition().x + ballTrail[i - 1].getRadius(), 
                        ballTrail[i - 1].getPosition().y + ballTrail[i - 1].getRadius());

                    ballTrail[i].setPosition(
                        center.x - trailBallRadius + (invertedX * fOffset),
                        center.y - trailBallRadius + (invertedY * fOffset));
                }

            }
                

            // Diagonal directions needs to be normalized!
            sf::Vector2f n(Vector::normalize(sf::Vector2f(directionBall.x, directionBall.y)));
            sf::Vector2f ballPosition = sf::Vector2f(
                ball.getPosition().x + (n.x * ballSpeed * deltaTime.asSeconds()),
                ball.getPosition().y + (n.y * ballSpeed * deltaTime.asSeconds()));

            // Paddle A collision checks.
            if (ball.getPosition().x <= paddleA.getPosition().x + PADDLE_WIDTH
                && playerTurn == PlayerTurn::PADDLE_A)
            {
                float heightTop = paddleA.getPosition().y;
                float heightBot = heightTop + PADDLE_HEIGHT;
                float radius = ball.getRadius();

                if (ball.getPosition().y >= heightTop - radius &&
                    ball.getPosition().y + (radius * 2) <= heightBot + radius)
                {
                    playerTurn = PlayerTurn::PADDLE_B;
                    directionBall.x = Direction::RIGHT;
                    ballSpeed += 10.0f;
                    //paddleSpeed += 8.0f;

                    int r = (rand() % 128) + 127;
                    int g = (rand() % 128) + 127;
                    int b = (rand() % 128) + 127;
                    paddleA.setFillColor(sf::Color(r, g, b));

                    deflect1.play();
                }

            }
            // Paddle B colloision checks.
            if (ball.getPosition().x + ball.getRadius() * 2 >= paddleB.getPosition().x
                && playerTurn == PlayerTurn::PADDLE_B)
            {
                float heightTop = paddleB.getPosition().y;
                float heightBot = heightTop + PADDLE_HEIGHT;
                float radius = ball.getRadius();

                if (ball.getPosition().y >= heightTop - radius &&
                    ball.getPosition().y + (radius * 2) <= heightBot + radius)
                {
                    playerTurn = PlayerTurn::PADDLE_A;
                    directionBall.x = Direction::LEFT;
                    ballSpeed += 10.0f;
                    //paddleSpeed += 8.0f;

                    int r = rand() % 255;
                    int g = rand() % 255;
                    int b = rand() % 255;
                    paddleB.setFillColor(sf::Color(r, g, b));

                    deflect1.play();
                }

            }

            // Top & bottom edge collision checks.
            if (ball.getPosition().y <= 0.0f)
            {
                directionBall.y = 1.0f;
                deflect2.play();
            }
            if (ball.getPosition().y >= window.getSize().y - (ball.getRadius() * 2))
            {
                directionBall.y = -1.0f;
                deflect2.play();
            }

            // Left & right edge collision checks.
            if (ball.getPosition().x < PADDLE_WIDTH / 2)
            {
                resetBall = true;
                ScoreB++;
                szScoreB.setString(std::to_string(ScoreB));
            }
            if (ball.getPosition().x > window.getSize().x - PADDLE_WIDTH / 2)
            {
                resetBall = true;
                ScoreA++;
                szScoreA.setString(std::to_string(ScoreA));
            }

            if (!resetBall)
                ball.setPosition(ballPosition);

            else
            {
                resetBall = false;
                ballSpeed = 500.0f;
                paddleSpeed = 650.0f;

                float a = (rand() % 2) == 0 ? -1.0f : 1.0f;
                float b = (rand() % 2) == 1 ? -1.0f : 1.0f;

                directionBall = sf::Vector2f(a, b);
                if (directionBall.x == -1.0f) playerTurn = PlayerTurn::PADDLE_A;
                if (directionBall.x ==  1.0f) playerTurn = PlayerTurn::PADDLE_B;
                ball.setPosition(window.getSize().x / 2 - ball.getRadius(), window.getSize().y / 2 - ball.getRadius());

                paddleA.setPosition(sf::Vector2f(PADDLE_WIDTH, (window.getSize().x / 2) - (PADDLE_HEIGHT / 2)));
                paddleB.setPosition(sf::Vector2f(window.getSize().x - (PADDLE_WIDTH * 2), paddleA.getPosition().y));
            }

            // Reset directions
            directionPaddleA.x = directionPaddleA.y = 0.0f;
            directionPaddleB.x = directionPaddleB.y = 0.0f;

            // Text
            szBallSpeed.setString("Ball speed: " + std::to_string((int)ballSpeed));
            szBallSpeed.setPosition(window.getSize().x / 2 - szBallSpeed.getGlobalBounds().width / 2,
                window.getSize().y / 2 - szBallSpeed.getGlobalBounds().height / 2 + 18);
            szPaddSpeed.setString("Paddle Speed: " + std::to_string((int)paddleSpeed));
            szPaddSpeed.setPosition(window.getSize().x / 2 - szPaddSpeed.getGlobalBounds().width / 2,
                window.getSize().y / 2 - szPaddSpeed.getGlobalBounds().height / 2 - 18);

            // Increase speed of paddles and the ball overtime.
            ballSpeed += ballSpeed / 750 * 32.0f * deltaTime.asSeconds();
            paddleSpeed += (paddleSpeed / 700.0f + ballSpeed / 4000.0f) * 30.0f * deltaTime.asSeconds();
        }

        // Handle events
        sf::Event event;
        while (window.pollEvent(event))
        {
            if (event.type == sf::Event::KeyPressed)
            {
                switch (event.key.code)
                {
                case sf::Keyboard::P: 
                    playerControl = !playerControl;
                    if (playerControl)
                    {
                        szController.setString("Controller: Player");
                        window.setMouseCursorVisible(false);
                    }
                    else
                    {
                        szController.setString("Controller: CPU");
                        window.setMouseCursorVisible(true);
                    }
                    szController.setPosition(window.getSize().x / 2 - szController.getGlobalBounds().width / 2,
                                             window.getSize().y / 2 - szController.getGlobalBounds().height / 2 - 54);
                    break;

                case sf::Keyboard::Escape: pause = !pause; break;
                default: break;
                }
            }

            if (event.type == sf::Event::Closed)
            {
                window.close();
            }
        }


        // Draw on screen
        window.clear(sf::Color(0, 0, 0));
        window.draw(paddleA);
        window.draw(paddleB);

        

        if (!pause)
        {
            window.draw(ball); 

            for (int i = 0; i < sizeof(ballTrail) / sizeof(ballTrail[0]); ++i)
                window.draw(ballTrail[i]);

            window.draw(szBallSpeed);
            window.draw(szPaddSpeed);
            window.draw(szController);
            window.draw(szToggleEsc);
            window.draw(szToggleP);
            window.draw(szScoreA);
            window.draw(szScoreB);
            window.draw(szScoreC);
        }

        else
        {
            window.draw(szPause);
        }

        window.display();

    }

    std::cout << "Program ran for: " << elapsedTime.asSeconds() << " seconds.";
    return 0;
}