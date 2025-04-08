/**
* Author: David Jiang
* Assignment: Lunar Lander
* Date due: 2025-3-15, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"


enum AppStatus { RUNNING, TERMINATED };

// Our window dimensions
constexpr int WINDOW_WIDTH  = 640,
              WINDOW_HEIGHT = 480;

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


constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl";
constexpr char F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr char BACKGROUND_TEXTURE_PATH[] = "/Users/davidj/Desktop/Intro-to-Game-Programming/assignment3/SDLProject/space.jpg";
constexpr char ROCKET_TEXTURE_PATH[]     = "/Users/davidj/Desktop/Intro-to-Game-Programming/assignment3/SDLProject/rocket.jpg";
constexpr char FLAMES_TEXTURE_PATH[]     = "/Users/davidj/Desktop/Intro-to-Game-Programming/assignment3/SDLProject/flames.jpg";
constexpr char FUEL_TEXTURE_PATH[]       = "/Users/davidj/Desktop/Intro-to-Game-Programming/assignment3/SDLProject/fuel.jpg";
constexpr char PLATFORM_TEXTURE_PATH[]   = "/Users/davidj/Desktop/Intro-to-Game-Programming/assignment3/SDLProject/platform.jpg";
constexpr char WIN_TEXTURE_PATH[]        = "/Users/davidj/Desktop/Intro-to-Game-Programming/assignment3/SDLProject/win.jpg";
constexpr char LOSE_TEXTURE_PATH[]       = "/Users/davidj/Desktop/Intro-to-Game-Programming/assignment3/SDLProject/lose.jpg";

constexpr float GRAVITY = 0.15f;
constexpr float LATERAL_THRUST = 2.0f;
constexpr float DAMPING = 0.99f;
constexpr float UP_THRUST = 0.3f;
constexpr float FUEL_CONSUMPTION_RATE = 100.0f / 30.0f;

constexpr float PLATFORM_WIDTH = 2.0f;
constexpr float PLATFORM_HEIGHT = 0.5f;
constexpr float PLATFORM_CENTER_X = -1.5f;
constexpr float PLATFORM_BOTTOM = -3.75f;

constexpr float MOVING_PLATFORM_WIDTH = 2.0f;
constexpr float MOVING_PLATFORM_HEIGHT = 0.5f;
constexpr float MOVING_PLATFORM_INITIAL_CENTER_X = 3.5f;
constexpr float MOVING_PLATFORM_LEFT_BOUND = 3.0f;
constexpr float MOVING_PLATFORM_RIGHT_BOUND = 4.5f;
constexpr float MOVING_PLATFORM_SPEED = 1.0f;

AppStatus g_app_status = RUNNING;
SDL_Window* g_display_window;

ShaderProgram g_shader_program;
GLuint g_background_texture_id;
GLuint g_rocket_texture_id;
GLuint g_flames_texture_id;
GLuint g_fuel_texture_id;
GLuint g_platform_texture_id;
GLuint g_win_texture_id;
GLuint g_lose_texture_id;

glm::mat4 g_view_matrix, g_background_model_matrix, g_rocket_model_matrix, g_projection_matrix;
glm::mat4 g_platform_model_matrix;
glm::mat4 g_movingPlatform_model_matrix;

float g_previous_ticks = 0.0f;

struct RocketState {
    glm::vec2 position;
    glm::vec2 velocity;
    glm::vec2 acceleration;
};

RocketState g_rocket_state;
float g_fuel = 100.0f;
bool g_show_flames = false;

bool g_game_over = false;
bool g_win = false;

float g_movingPlatformX = MOVING_PLATFORM_INITIAL_CENTER_X;
float g_movingPlatformSpeed = MOVING_PLATFORM_SPEED;

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
    
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height,
                 0, GL_RGBA, GL_UNSIGNED_BYTE, image);
    
    // Set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST); // or GL_LINEAR
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST); // or GL_LINEAR
    
    stbi_image_free(image);
    return texture_id;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    
    g_display_window = SDL_CreateWindow("Welcome to Lunar Lander",
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
    glViewport(0, 0, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    
    // Load up our shaders
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    // Initialise our view, model, and projection matrices
    g_view_matrix       = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    
    g_background_model_matrix = glm::scale(glm::mat4(1.0f), glm::vec3(10.0f, 7.5f, 1.0f));
    
    // initialise the rocket state
    g_rocket_state.position = glm::vec2(0.0f, 3.5f);
    g_rocket_state.velocity = glm::vec2(0.0f, 0.0f);
    g_rocket_state.acceleration = glm::vec2(0.0f, 0.0f);
    g_rocket_model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(g_rocket_state.position, 0.0f));
    g_rocket_model_matrix = glm::scale(g_rocket_model_matrix, glm::vec3(1.0f, -1.0f, 1.0f));
    
    g_background_texture_id = load_texture(BACKGROUND_TEXTURE_PATH);
    g_rocket_texture_id = load_texture(ROCKET_TEXTURE_PATH);
    g_flames_texture_id = load_texture(FLAMES_TEXTURE_PATH);
    g_fuel_texture_id = load_texture(FUEL_TEXTURE_PATH);
    g_platform_texture_id = load_texture(PLATFORM_TEXTURE_PATH);
    g_win_texture_id = load_texture(WIN_TEXTURE_PATH);
    g_lose_texture_id = load_texture(LOSE_TEXTURE_PATH);
    
    float platform_center_y = PLATFORM_BOTTOM + PLATFORM_HEIGHT / 2.0f;
    g_platform_model_matrix = glm::translate(glm::mat4(1.0f),
                                  glm::vec3(PLATFORM_CENTER_X, platform_center_y, 0.0f));
    g_platform_model_matrix = glm::scale(g_platform_model_matrix, glm::vec3(PLATFORM_WIDTH, -PLATFORM_HEIGHT, 1.0f));
    
    float moving_platform_center_y = PLATFORM_BOTTOM + MOVING_PLATFORM_HEIGHT / 2.0f;
    g_movingPlatformX = MOVING_PLATFORM_INITIAL_CENTER_X;
    g_movingPlatformSpeed = MOVING_PLATFORM_SPEED;
    g_movingPlatform_model_matrix = glm::translate(glm::mat4(1.0f),
                                        glm::vec3(g_movingPlatformX, moving_platform_center_y, 0.0f));
    g_movingPlatform_model_matrix = glm::scale(g_movingPlatform_model_matrix, glm::vec3(MOVING_PLATFORM_WIDTH, MOVING_PLATFORM_HEIGHT, 1.0f));
    
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);
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
    }
}

void update() { 
    if (g_game_over) {
        g_rocket_state.velocity = glm::vec2(0.0f, 0.0f);
        return;
    }
    
    float ticks = (float)SDL_GetTicks() / 1000.0f;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    g_movingPlatformX += g_movingPlatformSpeed * delta_time;
    if (g_movingPlatformX <= MOVING_PLATFORM_LEFT_BOUND) {
        g_movingPlatformX = MOVING_PLATFORM_LEFT_BOUND;
        g_movingPlatformSpeed = -g_movingPlatformSpeed;
    } else if (g_movingPlatformX >= MOVING_PLATFORM_RIGHT_BOUND) {
        g_movingPlatformX = MOVING_PLATFORM_RIGHT_BOUND;
        g_movingPlatformSpeed = -g_movingPlatformSpeed;
    }
    
    float moving_platform_center_y = PLATFORM_BOTTOM + MOVING_PLATFORM_HEIGHT / 2.0f;
    g_movingPlatform_model_matrix = glm::translate(glm::mat4(1.0f),
                                        glm::vec3(g_movingPlatformX, moving_platform_center_y, 0.0f));
    g_movingPlatform_model_matrix = glm::scale(g_movingPlatform_model_matrix, glm::vec3(MOVING_PLATFORM_WIDTH, -MOVING_PLATFORM_HEIGHT, 1.0f));
    
    
    g_rocket_state.acceleration = glm::vec2(0.0f, 0.0f);
    
    const Uint8* keys = SDL_GetKeyboardState(NULL);
    if (keys[SDL_SCANCODE_LEFT]) {
        g_rocket_state.acceleration.x = -LATERAL_THRUST;
    }
    if (keys[SDL_SCANCODE_RIGHT]) {
        g_rocket_state.acceleration.x = LATERAL_THRUST;
    }
    
    g_rocket_state.acceleration.y = -GRAVITY;
    
    if (keys[SDL_SCANCODE_UP] && g_fuel > 0.0f){
        g_rocket_state.acceleration.y += UP_THRUST;
        g_fuel -= FUEL_CONSUMPTION_RATE * delta_time;
        if (g_fuel < 0.0f){
            g_fuel = 0.0f;
        }
    }
        
    g_show_flames = keys[SDL_SCANCODE_UP] && (g_fuel > 0.0f);

    g_rocket_state.velocity += g_rocket_state.acceleration * delta_time;
    g_rocket_state.velocity.x *= DAMPING;
    g_rocket_state.position += g_rocket_state.velocity * delta_time;
    
    if (g_rocket_state.position.y < -3.5f) {
        g_rocket_state.position.y = -3.5f;
        g_rocket_state.velocity.y = 0.0f;
    }
    
    if (g_rocket_state.position.y > 3.5f) {
        g_rocket_state.position.y = 3.5f;
        g_rocket_state.velocity.y = 0.0f;
    }
    g_rocket_model_matrix = glm::translate(glm::mat4(1.0f), glm::vec3(g_rocket_state.position, 0.0f));
    g_rocket_model_matrix = glm::scale(g_rocket_model_matrix, glm::vec3(1.0f, -1.0f, 1.0f));
    
    float rocket_left = g_rocket_state.position.x - 0.5f;
    float rocket_right = g_rocket_state.position.x + 0.5f;
    float rocket_bottom = g_rocket_state.position.y - 0.5f;
    
    float static_left = PLATFORM_CENTER_X - (PLATFORM_WIDTH / 2.0f);
    float static_right = PLATFORM_CENTER_X + (PLATFORM_WIDTH / 2.0f);
    float static_top = PLATFORM_BOTTOM + PLATFORM_HEIGHT;
    
    float moving_left = g_movingPlatformX - (MOVING_PLATFORM_WIDTH / 2.0f);
    float moving_right = g_movingPlatformX + (MOVING_PLATFORM_WIDTH / 2.0f);
    float moving_top = PLATFORM_BOTTOM + MOVING_PLATFORM_HEIGHT;
        
    if (!g_game_over && g_rocket_state.velocity.y <= 0 && rocket_bottom <= static_top) {
        bool collide_static = (rocket_right >= static_left && rocket_left <= static_right);
        bool collide_moving = (rocket_right >= moving_left && rocket_left <= moving_right);
        if (collide_static || collide_moving) {
            float effective_top = static_top;
            if (collide_moving && moving_top > effective_top)
                effective_top = moving_top;
            g_rocket_state.position.y = effective_top + 0.5f;
            g_rocket_state.velocity.y = 0.0f;
            g_game_over = true;
            g_win = true;
        } else if (rocket_bottom <= -3.75f) {
            g_rocket_state.position.y = -3.75f + 0.5f;
            g_rocket_state.velocity.y = 0.0f;
            g_game_over = true;
            g_win = false;
        }
    }
}

void draw_object(glm::mat4 &model_matrix, GLuint texture_id) {
    g_shader_program.set_model_matrix(model_matrix);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}


void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    
    float vertices[] = {
        -0.5f, -0.5f,
         0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f, -0.5f,
         0.5f,  0.5f,
        -0.5f,  0.5f
    };
    float texCoords[] = {
        0.0f, 0.0f,
        1.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 0.0f,
        1.0f, 1.0f,
        0.0f, 1.0f
    };
    
    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, GL_FALSE, 0, texCoords);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    draw_object(g_background_model_matrix, g_background_texture_id);
    draw_object(g_platform_model_matrix, g_platform_texture_id);
    draw_object(g_movingPlatform_model_matrix, g_platform_texture_id);
    draw_object(g_rocket_model_matrix, g_rocket_texture_id);
    
    if (g_show_flames) {
        glm::vec2 flames_offset(0.0f, -0.6f);
        glm::mat4 flames_model_matrix = glm::translate(glm::mat4(1.0f),
                                        glm::vec3(g_rocket_state.position + flames_offset, 0.0f));
        flames_model_matrix = glm::scale(flames_model_matrix, glm::vec3(0.5f, 0.5f, 1.0f));
        draw_object(flames_model_matrix, g_flames_texture_id);
    }
    
    float maxFuelBarWidth = 2.0f;
    float barHeight = 0.3f;
    float fuelBarWidth = (g_fuel / 100.0f) * maxFuelBarWidth;
    glm::vec3 fuelBarPosition(5.0f - fuelBarWidth/2.0f - 0.2f, 3.75f - barHeight - 0.2f, 0.0f);
    glm::mat4 fuelBarModel = glm::translate(glm::mat4(1.0f), fuelBarPosition);
    fuelBarModel = glm::scale(fuelBarModel, glm::vec3(fuelBarWidth, barHeight, 1.0f));
    draw_object(fuelBarModel, g_fuel_texture_id);
    
    if (g_game_over) {
        glm::mat4 overlay = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.0f, 0.0f));
        overlay = glm::scale(overlay, glm::vec3(4.0f, -2.0f, 1.0f));
        if (g_win)
            draw_object(overlay, g_win_texture_id);
        else
            draw_object(overlay, g_lose_texture_id);
    }
    
    // Disable vertex attribute arrays.
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
