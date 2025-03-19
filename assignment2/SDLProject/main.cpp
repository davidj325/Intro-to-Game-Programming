/**
* Author: David Jiang
* Assignment: Pong Clone
* Date due: 2025-3-01, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"                // 4x4 Matrix
#include "glm/gtc/matrix_transform.hpp"  // Matrix transformation methods
#include "ShaderProgram.h"               // We'll talk about these later in the course

enum AppStatus { RUNNING, TERMINATED };

struct Ball {
    glm::vec2 position;
    glm::vec2 velocity;
};

// Our window dimensions
constexpr int WINDOW_WIDTH  = 640,
              WINDOW_HEIGHT = 360;

// Background color components
constexpr float BG_RED     = 0.1922f,
                BG_BLUE    = 0.549f,
                BG_GREEN   = 0.9059f,
                BG_OPACITY = 1.0f;

// Our viewport—or our "camera"'s—position and dimensions
constexpr int VIEWPORT_X      = 0,
              VIEWPORT_Y      = 0,
              VIEWPORT_WIDTH  = WINDOW_WIDTH,
              VIEWPORT_HEIGHT = WINDOW_HEIGHT;

// Our shader filepaths; these are necessary for a number of things
// Not least, to actually draw our shapes
// We'll have a whole lecture on these later
constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl";
constexpr char F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr char BACKGROUND_TEXTURE_PATH[] = "/Users/davidj/Desktop/Intro-to-Game-Programming/assignment2/SDLProject/court.png";
constexpr char PADDLE_TEXTURE_PATH[]     = "/Users/davidj/Desktop/Intro-to-Game-Programming/assignment2/SDLProject/paddle.png";
constexpr char BALL_TEXTURE_PATH[]       = "/Users/davidj/Desktop/Intro-to-Game-Programming/assignment2/SDLProject/ball.png";
constexpr char LEFT_WIN_TEXTURE_PATH[]   = "/Users/davidj/Desktop/Intro-to-Game-Programming/assignment2/SDLProject/left-win.png";
constexpr char RIGHT_WIN_TEXTURE_PATH[]  = "/Users/davidj/Desktop/Intro-to-Game-Programming/assignment2/SDLProject/right-win.png";

AppStatus g_app_status = RUNNING;
SDL_Window* g_display_window;

ShaderProgram g_shader_program;

glm::mat4 g_view_matrix,        // Defines the position (location and orientation) of the camera
          g_model_matrix,       // Defines every translation, rotation, and/or scaling applied to an object; we'll look at these next week
          g_projection_matrix;  // Defines the characteristics of your camera, such as clip panes, field of view, projection method, etc.

GLuint g_background_texture_id;
GLuint g_paddle_texture_id;
GLuint g_ball_texture_id;
GLuint g_left_win_texture_id;
GLuint g_right_win_texture_id;

// Paddle movement and positions
float g_previous_ticks = 0.0f;
constexpr float PADDLE_SPEED = 5.0f;
float g_player1_y = 0.0f;
float g_player2_y = 0.0f;

// one-player mode variables
bool g_autoPaddleMode = false;
float g_autoPaddle_velocity = 2.0f;

float g_ball_half_size = 0.2f;

bool g_game_over = false;
int  g_winner = 0;


Ball g_balls[3];
// defult one ball
int g_numBalls = 1;

GLuint load_texture(const char* filepath) {
    int width, height, num_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &num_components, STBI_rgb_alpha);
    if (!image) {
        std::cerr << "Failed to load texture: " << filepath << std::endl;
        exit(1);
    }
    
    GLuint texture_id;
    glGenTextures(1, &texture_id);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
    
    stbi_image_free(image);
    return texture_id;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Pong Tennis!",
                                      SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                      WINDOW_WIDTH, WINDOW_HEIGHT,
                                      SDL_WINDOW_OPENGL);
    
    if (g_display_window == nullptr)
    {
        std::cerr << "ERROR: SDL Window could not be created.\n";
        g_app_status = TERMINATED;
        
        SDL_Quit();
        exit(1);
    }
    
    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);
    
#ifdef _WINDOWS
    glewInit();
#endif
    
    // Initialise our camera
    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    // Load up our shaders
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    // Initialise our view, model, and projection matrices
    g_view_matrix       = glm::mat4(1.0f);  // Defines the position (location and orientation) of the camera
    g_model_matrix      = glm::mat4(1.0f);  // Defines every translation, rotations, or scaling applied to an object
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  // Defines the characteristics of your camera, such as clip planes, field of view, projection method etc.
    
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    // Notice we haven't set our model matrix yet!
    
    g_background_texture_id = load_texture(BACKGROUND_TEXTURE_PATH);
    g_paddle_texture_id = load_texture(PADDLE_TEXTURE_PATH);
    g_ball_texture_id = load_texture(BALL_TEXTURE_PATH);
    g_left_win_texture_id = load_texture(LEFT_WIN_TEXTURE_PATH);
    g_right_win_texture_id = load_texture(RIGHT_WIN_TEXTURE_PATH);
    
    // initialize all balls
    for (int i = 0; i < 3; i++) {
        g_balls[i].position = glm::vec2(0.0f, 0.0f);
        g_balls[i].velocity = glm::vec2(4.0f, 3.0f);
    }

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
    g_previous_ticks = SDL_GetTicks() / 1000.0f;

}

void process_input()
{
    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE)
        {
            g_app_status = TERMINATED;
        }
        
        if (event.type == SDL_KEYDOWN){
            if (event.key.keysym.scancode == SDL_SCANCODE_T){
                g_autoPaddleMode = !g_autoPaddleMode;
            }
            else if (event.key.keysym.scancode == SDL_SCANCODE_1) {
                g_numBalls = 1;
                g_balls[0].position = glm::vec2(0.0f, 0.0f);

            }
            else if (event.key.keysym.scancode == SDL_SCANCODE_2) {
                g_numBalls = 2;
                g_balls[0].position = glm::vec2(0.0f, 0.3f);
                g_balls[1].position = glm::vec2(0.0f, -0.3f);
            }
            else if (event.key.keysym.scancode == SDL_SCANCODE_3) {
                g_numBalls = 3;
                g_balls[0].position = glm::vec2(0.0f, 0.3f);
                g_balls[1].position = glm::vec2(0.0f, 0.0f);
                g_balls[2].position = glm::vec2(0.0f, -0.3f);
            }
        }
    }
}

void update() { 
    
    if (g_game_over){
        return;
    }
    
    // Compute delta time
    float ticks = SDL_GetTicks() / 1000.0f;
    float dt = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    
    // left paddle using w and s
    if (keys[SDL_SCANCODE_W]) {
        g_player1_y += PADDLE_SPEED * dt;
    }
    if (keys[SDL_SCANCODE_S]) {
        g_player1_y -= PADDLE_SPEED * dt;
    }
    
    float paddle_half_height = 0.75f;
    float top_bound = 3.75f - paddle_half_height;
    float bottom_bound = -3.75f + paddle_half_height;
    
    if (g_player1_y > top_bound) g_player1_y = top_bound;
    if (g_player1_y < bottom_bound) g_player1_y = bottom_bound;
    
    if (!g_autoPaddleMode){
        // right paddle using up and down
        if (keys[SDL_SCANCODE_UP]) {
            g_player2_y += PADDLE_SPEED * dt;
        }
        if (keys[SDL_SCANCODE_DOWN]) {
            g_player2_y -= PADDLE_SPEED * dt;
        }
    }
    else{
        // In auto mode
        g_player2_y += g_autoPaddle_velocity * dt;
        if (g_player2_y > top_bound) {
            g_player2_y = top_bound;
            g_autoPaddle_velocity = -fabs(g_autoPaddle_velocity);
        }
        else if (g_player2_y < bottom_bound) {
            g_player2_y = bottom_bound;
            g_autoPaddle_velocity = fabs(g_autoPaddle_velocity);
        }
    }
    
    
    if (g_player2_y > top_bound) g_player2_y = top_bound;
    if (g_player2_y < bottom_bound) g_player2_y = bottom_bound;
    
    // update each active ball.
    for (int i = 0; i < g_numBalls; i++) {
        g_balls[i].position += g_balls[i].velocity * dt;
        
        // bounce off top and bottom walls
        float topWall = 3.75f;
        float bottomWall = -3.75f;
        if (g_balls[i].position.y + g_ball_half_size >= topWall ||
            g_balls[i].position.y - g_ball_half_size <= bottomWall) {
            g_balls[i].velocity.y = -g_balls[i].velocity.y;
        }
        
        // check if ball has hit left or right wall
        if (g_balls[i].position.x - g_ball_half_size <= -5.0f) {
            g_game_over = true;
            g_winner = 2;
        }
        else if (g_balls[i].position.x + g_ball_half_size >= 5.0f) {
            g_game_over = true;
            g_winner = 1;
        }
        
        // collision detection for left paddle
        float paddle1_center_x = -4.5f;
        float paddle1_center_y = g_player1_y;
        float paddle1_half_width = 0.25f;
        float paddle1_half_height = 0.75f;
        
        float ball_left   = g_balls[i].position.x - g_ball_half_size;
        float ball_right  = g_balls[i].position.x + g_ball_half_size;
        float ball_top    = g_balls[i].position.y + g_ball_half_size;
        float ball_bottom = g_balls[i].position.y - g_ball_half_size;
        
        float paddle1_left   = paddle1_center_x - paddle1_half_width;
        float paddle1_right  = paddle1_center_x + paddle1_half_width;
        float paddle1_top    = paddle1_center_y + paddle1_half_height;
        float paddle1_bottom = paddle1_center_y - paddle1_half_height;
        
        if (ball_left < paddle1_right &&
            ball_right > paddle1_left &&
            ball_top > paddle1_bottom &&
            ball_bottom < paddle1_top) {
            g_balls[i].velocity.x = fabs(g_balls[i].velocity.x);
            g_balls[i].position.x = paddle1_right + g_ball_half_size;
        }
        
        // collision detection for right paddle
        float paddle2_center_x = 4.5f;
        float paddle2_center_y = g_player2_y;
        float paddle2_half_width = 0.25f;
        float paddle2_half_height = 0.75f;
        
        float paddle2_left   = paddle2_center_x - paddle2_half_width;
        float paddle2_right  = paddle2_center_x + paddle2_half_width;
        float paddle2_top    = paddle2_center_y + paddle2_half_height;
        float paddle2_bottom = paddle2_center_y - paddle2_half_height;
        
        if (ball_right > paddle2_left &&
            ball_left < paddle2_right &&
            ball_top > paddle2_bottom &&
            ball_bottom < paddle2_top) {
            g_balls[i].velocity.x = -fabs(g_balls[i].velocity.x);
            g_balls[i].position.x = paddle2_left - g_ball_half_size;
        }
    }

}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    
//    g_shader_program.set_model_matrix(g_model_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    g_shader_program.set_projection_matrix(g_projection_matrix);
    

    float vertices[] = {
            // first triangle
            -0.5f, -0.5f,
             0.5f, -0.5f,
             0.5f,  0.5f,
            // second triangle
            -0.5f, -0.5f,
             0.5f,  0.5f,
            -0.5f,  0.5f
        };
    
    float texCoords[] = {
            // first triangle
            0.0f, 0.0f,
            1.0f, 0.0f,
            1.0f, 1.0f,
            // second triangle
            0.0f, 0.0f,
            1.0f, 1.0f,
            0.0f, 1.0f
        };
    
    // Enable vertex attributes (position and tex coords)
    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, GL_FALSE, 0, texCoords);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    if (g_game_over) {
        glm::mat4 win_model_matrix = glm::mat4(1.0f);
        win_model_matrix = glm::scale(win_model_matrix, glm::vec3(3.0f, -1.5f, 1.0f));
        g_shader_program.set_model_matrix(win_model_matrix);
        if (g_winner == 1) {
            glBindTexture(GL_TEXTURE_2D, g_left_win_texture_id);
        } else if (g_winner == 2) {
            glBindTexture(GL_TEXTURE_2D, g_right_win_texture_id);
        }
        glDrawArrays(GL_TRIANGLES, 0, 6);
        SDL_GL_SwapWindow(g_display_window);
        return;
    }
    
    // Background
    glm::mat4 background_model_matrix = glm::mat4(1.0f);
    background_model_matrix = glm::scale(background_model_matrix, glm::vec3(10.0f, 7.5f, 1.0f));
    g_shader_program.set_model_matrix(background_model_matrix);
    glBindTexture(GL_TEXTURE_2D, g_background_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // Left Paddle
    glm::mat4 paddle1_model_matrix = glm::mat4(1.0f);
    paddle1_model_matrix = glm::translate(paddle1_model_matrix, glm::vec3(-4.5f, g_player1_y, 0.0f));
    paddle1_model_matrix = glm::scale(paddle1_model_matrix, glm::vec3(0.5f, -1.5f, 1.0f));
    g_shader_program.set_model_matrix(paddle1_model_matrix);
    glBindTexture(GL_TEXTURE_2D, g_paddle_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // Right Paddle
    glm::mat4 paddle2_model_matrix = glm::mat4(1.0f);
    paddle2_model_matrix = glm::translate(paddle2_model_matrix, glm::vec3(4.5f, g_player2_y, 0.0f));
    paddle2_model_matrix = glm::scale(paddle2_model_matrix, glm::vec3(0.5f, -1.5f, 1.0f));
    g_shader_program.set_model_matrix(paddle2_model_matrix);
    glBindTexture(GL_TEXTURE_2D, g_paddle_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
    
    // Ball
    for (int i = 0; i < g_numBalls; i++) {
        glm::mat4 ball_model_matrix = glm::mat4(1.0f);
        ball_model_matrix = glm::translate(ball_model_matrix, glm::vec3(g_balls[i].position, 0.0f));
        ball_model_matrix = glm::scale(ball_model_matrix, glm::vec3(g_ball_half_size * 2, g_ball_half_size * 2, 1.0f));
        g_shader_program.set_model_matrix(ball_model_matrix);
        glBindTexture(GL_TEXTURE_2D, g_ball_texture_id);
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

/**
 Start here—we can see the general structure of a game loop without worrying too much about the details yet.
 */
int main(int argc, char* argv[])
{
    // Initialise our program—whatever that means
    initialise();
    
    while (g_app_status == RUNNING)
    {
        process_input();  // If the player did anything—press a button, move the joystick—process it
        update();         // Using the game's previous state, and whatever new input we have, update the game's state
        render();         // Once updated, render those changes onto the screen
    }
    
    shutdown();  // The game is over, so let's perform any shutdown protocols
    return 0;
}
