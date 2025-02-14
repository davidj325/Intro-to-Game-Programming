/**
* Author: David Jiang
* Assignment: Simple 2D Scene
* Date due: 2025-02-15, 11:59pm
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
#include "glm/mat4x4.hpp"                // 4x4 Matrix
#include "glm/gtc/matrix_transform.hpp"  // Matrix transformation methods
#include "ShaderProgram.h"               // We'll talk about these later in the course

enum AppStatus { RUNNING, TERMINATED };

// Our window dimensions
constexpr int WINDOW_WIDTH  = 840,
              WINDOW_HEIGHT = 680;

// Background color components
constexpr float BG_RED     = 0.1922f,
                BG_BLUE    = 0.549f,
                BG_GREEN   = 0.9059f,
                BG_OPACITY = 1.0f;

constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl";
constexpr char F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr char BASKETBALL_TEXTURE_PATH[] = "Basketball.png";
constexpr char LEBRON_TEXTURE_PATH[] = "leb.jpg";

constexpr float BASKETBALL_ROTATION_SPEED = 1.0f;
constexpr float LEBRON_ROTATION_SPEED = -2.0f;

AppStatus g_app_status = RUNNING;
SDL_Window* g_display_window;

ShaderProgram g_shader_program;
GLuint g_basketball_texture_id;
GLuint g_lebron_texture_id;

glm::mat4 g_view_matrix,g_basketball_model_matrix, g_lebron_model_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_basketball_angle = 0.0f;
float g_lebron_angle = 0.0f;

//basketball heartbeat
constexpr float GROWTH_FACTOR = 1.10f;
constexpr float SHRINK_FACTOR = 0.90f;
constexpr int MAX_FRAME = 40;

int g_frame_counter = 0;
bool g_is_growing = true;

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
    // HARD INITIALISE ———————————————————————————————————————————————————————————————————
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Hello Basketball!",
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
    
    // ———————————————————————————————————————————————————————————————————————————————————
    
    // SOFT INITIALISE ———————————————————————————————————————————————————————————————————
    // Initialise our camera
    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);
    
    // Load up our shaders
    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);
    
    // Initialise our view, model, and projection matrices
    g_view_matrix       = glm::mat4(1.0f);  // Defines the position (location and orientation) of the camera
    g_basketball_model_matrix = glm::mat4(1.0f);
    g_lebron_model_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  // Defines the characteristics of your camera, such as clip planes, field of view, projection method etc.
    
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);
    
    g_basketball_texture_id = load_texture(BASKETBALL_TEXTURE_PATH);
    g_lebron_texture_id = load_texture(LEBRON_TEXTURE_PATH);
    
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
    float ticks = (float)SDL_GetTicks() / 1000.0f;
    float delta_time = ticks - g_previous_ticks;
    g_previous_ticks = ticks;
    
    // Rotate basketball
    g_basketball_angle += BASKETBALL_ROTATION_SPEED * delta_time;
    float orbit_radius = 3.0f;
    float posX = orbit_radius * cos(g_basketball_angle);
    float posY = orbit_radius * sin(g_basketball_angle);
    glm::mat4 basketball_model = glm::translate(glm::mat4(1.0f), glm::vec3(posX, posY, 0.0f));
    
    // Scalling basketball
    g_frame_counter += 1;
    if (g_frame_counter >= MAX_FRAME) {
        g_is_growing = !g_is_growing;
        g_frame_counter = 0;
    }
    glm::vec3 scale_vector = glm::vec3(g_is_growing ? GROWTH_FACTOR : SHRINK_FACTOR,
                                           g_is_growing ? GROWTH_FACTOR : SHRINK_FACTOR,
                                           1.0f);
    basketball_model = glm::scale(basketball_model, scale_vector);
    g_basketball_model_matrix = basketball_model;
    
    // Rotate lebron
    g_lebron_angle += LEBRON_ROTATION_SPEED * delta_time;
    g_lebron_model_matrix = glm::mat4(1.0f);
    g_lebron_model_matrix = glm::rotate(g_lebron_model_matrix, g_lebron_angle, glm::vec3(1.0f, 1.0f, 1.0f));
    g_lebron_model_matrix = glm::scale(g_lebron_model_matrix, glm::vec3(2.0f, 2.0f, 1.0f));
}

void draw_object(glm::mat4 &model_matrix, GLuint texture_id) {
    g_shader_program.set_model_matrix(model_matrix);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6);
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);
    
    float vertices[] = {
            // Triangle 1
            -0.5f, -0.5f, 0.5f, -0.5f, 0.5f,  0.5f,
            // Triangle 2
            -0.5f, -0.5f, 0.5f, 0.5f, -0.5f,  0.5f
        };
    
    float texture_coordinates[] = {
            // Triangle 1
            0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,
            // Triangle 2
            0.0f, 0.0f, 1.0f, 1.0f, 0.0f, 1.0f
        };
    
    glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, GL_FALSE, 0, vertices);
    glEnableVertexAttribArray(g_shader_program.get_position_attribute());
    
    glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, GL_FALSE, 0, texture_coordinates);
    glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    draw_object(g_basketball_model_matrix, g_basketball_texture_id);
    draw_object(g_lebron_model_matrix, g_lebron_texture_id);
    
    glDisableVertexAttribArray(g_shader_program.get_position_attribute());
    glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
    
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

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
