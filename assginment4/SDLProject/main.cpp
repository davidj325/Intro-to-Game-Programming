/**
* Author: David Jiang
* Assignment: Rise of the AI
* Date due: 2025-04-05, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#include <iostream>
#include <string>
#include <vector>
#include <cassert>

#define LOG(argument) std::cout << argument << '\n'
#define STB_IMAGE_IMPLEMENTATION
#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include <SDL_mixer.h>
#include "stb_image.h"
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"

constexpr int WINDOW_WIDTH  = 640,
              WINDOW_HEIGHT = 480;

constexpr float BG_RED     = 0.1f,
                BG_GREEN   = 0.1f,
                BG_BLUE    = 0.1f,
                BG_OPACITY = 1.0f;


constexpr char V_SHADER_PATH[] = "shaders/vertex_textured.glsl";
constexpr char F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

constexpr char FONTSHEET_FILEPATH[] = "/Users/davidj/Desktop/Intro-to-Game-Programming/assginment4/SDLProject/font1.png";
constexpr char TILEMAP_FILEPATH[]   = "/Users/davidj/Desktop/Intro-to-Game-Programming/assginment4/SDLProject/tilemap_packed.png";
constexpr char CHARACTER_FILEPATH[] = "/Users/davidj/Desktop/Intro-to-Game-Programming/assginment4/SDLProject/character-male-c.png";
constexpr char GHOST_FILEPATH[]        = "/Users/davidj/Desktop/Intro-to-Game-Programming/assginment4/SDLProject/character-ghost.png";
constexpr char SKELETON_FILEPATH[] = "/Users/davidj/Desktop/Intro-to-Game-Programming/assginment4/SDLProject/character-skeleton.png";
constexpr char BGM_FILEPATH[]          = "/Users/davidj/Desktop/Intro-to-Game-Programming/assginment4/SDLProject/Cloud_Dancer.mp3";
constexpr char COFFIN_FILEPATH[]       = "/Users/davidj/Desktop/Intro-to-Game-Programming/assginment4/SDLProject/coffin.png";

constexpr GLint NUMBER_OF_TEXTURES = 1,
              LEVEL_OF_DETAIL    = 0,
              TEXTURE_BORDER     = 0;

constexpr int FONTBANK_SIZE = 16;

constexpr int CD_QUAL_FREQ    = 44100;
constexpr int AUDIO_CHAN_AMT  = 2;
constexpr int AUDIO_BUFF_SIZE = 4096;

int g_playerLives = 3;


enum AppStatus { RUNNING, TERMINATED };

AppStatus g_app_status = RUNNING;
SDL_Window* g_display_window;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_model_matrix, g_projection_matrix;

GLuint g_font_texture_id;

Mix_Music* g_backgroundMusic = nullptr;

const glm::vec2 g_coffinSize = glm::vec2(2.0f, 0.5f);
const glm::vec2 g_coffinPos = glm::vec2(0.0f, 0.0f);
GLuint g_coffinTexture;

GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);
    if (image == nullptr)
    {
        LOG("Unable to load image. Make sure the path is correct: " << filepath);
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);

    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER,
                 GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;
}

void draw_text(ShaderProgram *shader_program, GLuint font_texture_id, std::string text,
               float font_size, float spacing, glm::vec3 position)
{
    float width = 1.0f / FONTBANK_SIZE;
    float height = 1.0f / FONTBANK_SIZE;

    std::vector<float> vertices;
    std::vector<float> texture_coordinates;

    for (int i = 0; i < text.size(); i++) {
        int spritesheet_index = (int) text[i];
        float offset = (font_size + spacing) * i;

        float u_coordinate = (float)(spritesheet_index % FONTBANK_SIZE) / FONTBANK_SIZE;
        float v_coordinate = (float)(spritesheet_index / FONTBANK_SIZE) / FONTBANK_SIZE;

        vertices.insert(vertices.end(), {
            offset + (-0.5f * font_size),  0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size),  0.5f * font_size,
            offset + (0.5f * font_size), -0.5f * font_size,
            offset + (0.5f * font_size),  0.5f * font_size,
            offset + (-0.5f * font_size), -0.5f * font_size,
        });

        texture_coordinates.insert(texture_coordinates.end(), {
            u_coordinate,           v_coordinate,
            u_coordinate,           v_coordinate + height,
            u_coordinate + width,   v_coordinate,
            u_coordinate + width,   v_coordinate + height,
            u_coordinate + width,   v_coordinate,
            u_coordinate,           v_coordinate + height,
        });
    }

    glm::mat4 model_matrix = glm::mat4(1.0f);
    model_matrix = glm::translate(model_matrix, position);

    shader_program->set_model_matrix(model_matrix);
    glUseProgram(shader_program->get_program_id());

    glVertexAttribPointer(shader_program->get_position_attribute(), 2, GL_FLOAT, false, 0,
                          vertices.data());
    glEnableVertexAttribArray(shader_program->get_position_attribute());

    glVertexAttribPointer(shader_program->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0,
                          texture_coordinates.data());
    glEnableVertexAttribArray(shader_program->get_tex_coordinate_attribute());

    glBindTexture(GL_TEXTURE_2D, font_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, (int)(text.size() * 6));

    glDisableVertexAttribArray(shader_program->get_position_attribute());
    glDisableVertexAttribArray(shader_program->get_tex_coordinate_attribute());
}

bool check_coffin_collision(const glm::vec3& playerPos, float spriteSize) {
    float coffinLeft = g_coffinPos.x - (g_coffinSize.x / 2.0f);
    float coffinRight = g_coffinPos.x + (g_coffinSize.x / 2.0f);
    float coffinBottom = g_coffinPos.y - (g_coffinSize.y / 2.0f);
    float coffinTop = g_coffinPos.y + (g_coffinSize.y / 2.0f);
    return (playerPos.x < coffinRight &&
            playerPos.x + spriteSize > coffinLeft &&
            playerPos.y < coffinTop &&
            playerPos.y + spriteSize > coffinBottom);
}

void render_coffin(ShaderProgram* shader) {
    glm::mat4 model = glm::translate(glm::mat4(1.0f), glm::vec3(g_coffinPos, 0.0f));
    model = glm::scale(model, glm::vec3(g_coffinSize, 1.0f));
    shader->set_model_matrix(model);
    float vertices[] = {
         -0.5f, -0.5f,
          0.5f, -0.5f,
          0.5f,  0.5f,
         -0.5f, -0.5f,
          0.5f,  0.5f,
         -0.5f,  0.5f
    };
    float texCoords[] = {
         0.0f, 1.0f,
         1.0f, 1.0f,
         1.0f, 0.0f,
         0.0f, 1.0f,
         1.0f, 0.0f,
         0.0f, 0.0f
    };
    glBindTexture(GL_TEXTURE_2D, g_coffinTexture);
    glVertexAttribPointer(shader->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(shader->get_position_attribute());
    glVertexAttribPointer(shader->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texCoords);
    glEnableVertexAttribArray(shader->get_tex_coordinate_attribute());
    glDrawArrays(GL_TRIANGLES, 0, 6);
    glDisableVertexAttribArray(shader->get_position_attribute());
    glDisableVertexAttribArray(shader->get_tex_coordinate_attribute());
}



class Scene {
public:
    virtual void processInput() = 0;
    virtual void update(float deltaTime) = 0;
    virtual void render() = 0;
    virtual bool isFinished() = 0;
    virtual ~Scene() {}
};

class EndScene : public Scene {
public:
    EndScene(std::string message) : message(message), finished(false) { }
    void processInput() override {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                finished = true;
                g_app_status = TERMINATED;
            }
        }
    }
    void update(float deltaTime) override {
    }
    void render() override {
        glClear(GL_COLOR_BUFFER_BIT);
        draw_text(&g_shader_program, g_font_texture_id, message, 0.7f, 0.0f, glm::vec3(-2.5f, 0, 0));
        SDL_GL_SwapWindow(g_display_window);
    }
    bool isFinished() override {
        return finished;
    }
private:
    std::string message;
    bool finished;
};

class MenuScene : public Scene {
public:
    MenuScene() : finished(false) { }
    
    void processInput() override {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                finished = true;
                g_app_status = TERMINATED;
            }
            if (event.type == SDL_KEYDOWN) {
                if (event.key.keysym.sym == SDLK_RETURN) {
                    finished = true;
                }
            }
        }
    }
    
    void update(float deltaTime) override {
    }
    
    void render() override {
        glClear(GL_COLOR_BUFFER_BIT);
        g_model_matrix = glm::mat4(1.0f);
        g_shader_program.set_model_matrix(g_model_matrix);
        
        draw_text(&g_shader_program, g_font_texture_id, "Objective is to reach", 0.4f, 0.0f, glm::vec3(-4.7f, 1.2f, 0));
        draw_text(&g_shader_program, g_font_texture_id, "bottom left corner", 0.4f, 0.0f, glm::vec3(-4.7f, 0.7f, 0));

        draw_text(&g_shader_program, g_font_texture_id, "Project 4", 0.5f, 0.0f, glm::vec3(-3.0f, 2.0f, 0));
        draw_text(&g_shader_program, g_font_texture_id, "Press enter to start", 0.5f, 0.0f, glm::vec3(-4.7f, -0.5f, 0));
        SDL_GL_SwapWindow(g_display_window);
    }
    
    bool isFinished() override {
        return finished;
    }
    
private:
    bool finished;
};

class GameMap {
public:
    
    GameMap(GLuint textureID, float tileSize, int mapWidth, int mapHeight, int tilesPerRow, const std::vector<int>& levelData)
        : textureID(textureID), tileSize(tileSize), mapWidth(mapWidth), mapHeight(mapHeight),
          tilesPerRow(tilesPerRow), levelData(levelData) { }
    
    void render(ShaderProgram* shader) {
        for (int y = 0; y < mapHeight; y++) {
            for (int x = 0; x < mapWidth; x++) {
                int tileValue = levelData[y * mapWidth + x];
                if (tileValue == 0) continue;
                
                int tileIndex = tileValue - 1;
                int tileCol = tileIndex % tilesPerRow;
                int tileRow = tileIndex / tilesPerRow;
                
                float u = (float)tileCol / (float)tilesPerRow;
                float v = (float)tileRow / (float)tilesPerRow;
                float tileWidthUV = 1.0f / (float)tilesPerRow;
                float tileHeightUV = 1.0f / (float)tilesPerRow;
                
                
                float xpos = -5.0f + x * tileSize;
                float ypos = 3.75f - y * tileSize;
                
                float vertices[] = {
                    xpos,           ypos,
                    xpos + tileSize, ypos,
                    xpos + tileSize, ypos - tileSize,
                    xpos,           ypos,
                    xpos + tileSize, ypos - tileSize,
                    xpos,           ypos - tileSize,
                };
                
                float texCoords[] = {
                    u,               v,
                    u + tileWidthUV, v,
                    u + tileWidthUV, v + tileHeightUV,
                    u,               v,
                    u + tileWidthUV, v + tileHeightUV,
                    u,               v + tileHeightUV,
                };
                
                glm::mat4 model = glm::mat4(1.0f);
                shader->set_model_matrix(model);
                glBindTexture(GL_TEXTURE_2D, textureID);
                
                glVertexAttribPointer(shader->get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
                glEnableVertexAttribArray(shader->get_position_attribute());
                glVertexAttribPointer(shader->get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texCoords);
                glEnableVertexAttribArray(shader->get_tex_coordinate_attribute());
                
                glDrawArrays(GL_TRIANGLES, 0, 6);
                
                glDisableVertexAttribArray(shader->get_position_attribute());
                glDisableVertexAttribArray(shader->get_tex_coordinate_attribute());
            }
        }
    }
    
private:
    GLuint textureID;
    float tileSize;
    int mapWidth, mapHeight;
    int tilesPerRow;
    std::vector<int> levelData;
};

class Level1Scene : public Scene {
public:
    Level1Scene() : finished(false) {
        srand((unsigned)time(0));
        tilemapTexture = load_texture(TILEMAP_FILEPATH);
        
        levelData = {
            1, 2, 1, 1, 1, 1, 1, 2, 1, 1,
            1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
            1, 1, 2, 2, 2, 2, 2, 1, 2, 1,
            1, 1, 2, 1, 1, 1, 2, 1, 1, 1,
            1, 1, 2, 1, 2, 1, 2, 1, 1, 1,
            1, 2, 1, 1, 1, 1, 1, 1, 2, 1,
            1, 1, 1, 2, 1, 1, 1, 2, 1, 1,
        };
        
        int mapWidth = 10;
        int mapHeight = 7;
        float tileSize = 1.0f;
        int tilesPerRow = 8;
        
        gameMap = new GameMap(tilemapTexture, tileSize, mapWidth, mapHeight, tilesPerRow, levelData);
        
        //character texture
        characterTexture = load_texture(CHARACTER_FILEPATH);
        characterStart = glm::vec3(-5.0f, 3.75f - 1.0f, 0.0f);
        characterPosition = characterStart;
        characterSpeed = 3.0f;
        
        //ghost texture
        ghostTexture = load_texture(GHOST_FILEPATH);
        ghostPosition = glm::vec3(0.0f, 0.0f, 0.0f);
        ghostSpeed = 2.0f;
        float angle = ((float)rand() / RAND_MAX) * 2 * 3.14159f;
        ghostVelocity = glm::vec3(cos(angle), sin(angle), 0.0f) * ghostSpeed;
        ghostChangeTimer = 0.0f;
    }
    
    ~Level1Scene() {
        delete gameMap;
    }
    
    void processInput() override {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                finished = true;
                g_app_status = TERMINATED;
            }
        }
    }
    
    void update(float deltaTime) override {
        const Uint8* keyState = SDL_GetKeyboardState(NULL);
        glm::vec3 prevPos = characterPosition;
        glm::vec3 movement(0.0f);
        if(keyState[SDL_SCANCODE_W]) { movement.y += 1.0f; }
        if(keyState[SDL_SCANCODE_S]) { movement.y -= 1.0f; }
        if(keyState[SDL_SCANCODE_A]) { movement.x -= 1.0f; }
        if(keyState[SDL_SCANCODE_D]) { movement.x += 1.0f; }
        if(glm::length(movement) > 0.0f) {
            movement = glm::normalize(movement);
        }
        characterPosition += movement * characterSpeed * deltaTime;
        
        // clamp player
        float spriteSize = 1.0f;
        float leftBoundary   = -5.0f;
        float rightBoundary  = 5.0f - spriteSize;
        float bottomBoundary = -3.75f;
        float topBoundary    = 3.75f - spriteSize;
        if (characterPosition.x < leftBoundary)  characterPosition.x = leftBoundary;
        if (characterPosition.x > rightBoundary) characterPosition.x = rightBoundary;
        if (characterPosition.y < bottomBoundary) characterPosition.y = bottomBoundary;
        if (characterPosition.y > topBoundary)    characterPosition.y = topBoundary;
        
        if(check_coffin_collision(characterPosition, spriteSize)) {
            characterPosition = prevPos;
        }
        
        // ghost ai random movement
        ghostChangeTimer += deltaTime;
        if(ghostChangeTimer >= 1.0f) {
            float angle = ((float)rand() / RAND_MAX) * 2 * 3.14159f;
            ghostVelocity = glm::vec3(cos(angle), sin(angle), 0.0f) * ghostSpeed;
            ghostChangeTimer = 0.0f;
        }
        ghostPosition += ghostVelocity * deltaTime;
        
        // clamp ghost
        if (ghostPosition.x < leftBoundary)  ghostPosition.x = leftBoundary;
        if (ghostPosition.x > rightBoundary) ghostPosition.x = rightBoundary;
        if (ghostPosition.y < bottomBoundary) ghostPosition.y = bottomBoundary;
        if (ghostPosition.y > topBoundary)    ghostPosition.y = topBoundary;
        
        // collision check between player and ghost
        if (fabs(characterPosition.x - ghostPosition.x) < 0.8f &&
            fabs(characterPosition.y - ghostPosition.y) < 0.8f) {
            if(g_playerLives > 0) {
                g_playerLives--;
                characterPosition = characterStart;
            }
            if(g_playerLives <= 0)
            {
                finished = true;
            }
        }
        
        // win condition
        if (characterPosition.x >= rightBoundary && characterPosition.y <= bottomBoundary + 0.01f) {
            finished = true;
        }
    }
    
    void render() override {
        glClear(GL_COLOR_BUFFER_BIT);
        gameMap->render(&g_shader_program);
        
        render_coffin(&g_shader_program);
        
        glm::mat4 model = glm::translate(glm::mat4(1.0f), characterPosition);
        g_shader_program.set_model_matrix(model);
        float spriteSize = 1.0f;
        float vertices[] = {
            0.0f,         0.0f,
            spriteSize,   0.0f,
            spriteSize,   spriteSize,
            0.0f,         0.0f,
            spriteSize,   spriteSize,
            0.0f,         spriteSize
        };
        float texCoords[] = {
            0.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 0.0f
        };
        glBindTexture(GL_TEXTURE_2D, characterTexture);
        glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(g_shader_program.get_position_attribute());
        glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(g_shader_program.get_position_attribute());
        glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
        
        model = glm::translate(glm::mat4(1.0f), ghostPosition);
        g_shader_program.set_model_matrix(model);
        float ghostVertices[] = {
            0.0f,         0.0f,
            spriteSize,   0.0f,
            spriteSize,   spriteSize,
            0.0f,         0.0f,
            spriteSize,   spriteSize,
            0.0f,         spriteSize
        };
        float ghostTexCoords[] = {
            0.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 0.0f
        };
        
        glBindTexture(GL_TEXTURE_2D, ghostTexture);
        glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, ghostVertices);
        glEnableVertexAttribArray(g_shader_program.get_position_attribute());
        glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, ghostTexCoords);
        glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(g_shader_program.get_position_attribute());
        glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
        

        std::string healthText = "Lives: " + std::to_string(g_playerLives);
        draw_text(&g_shader_program, g_font_texture_id, healthText, 0.4f, 0.0f, glm::vec3(1.5f, 3.25f, 0));
        SDL_GL_SwapWindow(g_display_window);
    }
    
    bool isFinished() override {
        return finished;
    }
    
private:
    bool finished;
    GLuint tilemapTexture;
    std::vector<int> levelData;
    GameMap* gameMap;
    
    GLuint characterTexture;
    glm::vec3 characterPosition;
    glm::vec3 characterStart;
    float characterSpeed;
    
    GLuint ghostTexture;
    glm::vec3 ghostPosition;
    glm::vec3 ghostVelocity;
    float ghostSpeed;
    float ghostChangeTimer;
};

class Level2Scene : public Scene {
public:
    Level2Scene() : finished(false) {
        srand((unsigned)time(0));
        tilemapTexture = load_texture(TILEMAP_FILEPATH);
        levelData = {
            1, 2, 1, 1, 1, 1, 1, 2, 1, 1,
            1, 1, 2, 1, 2, 1, 2, 1, 1, 1,
            1, 2, 1, 1, 2, 1, 1, 1, 2, 1,
            1, 1, 2, 2, 2, 2, 2, 1, 2, 1,
            1, 1, 1, 2, 1, 1, 1, 2, 1, 1,
            1, 1, 2, 1, 1, 1, 2, 1, 1, 1,
            1, 2, 1, 1, 1, 1, 1, 1, 2, 1,
            
        };
        int mapWidth = 10;
        int mapHeight = 7;
        float tileSize = 1.0f;
        int tilesPerRow = 8;
        gameMap = new GameMap(tilemapTexture, tileSize, mapWidth, mapHeight, tilesPerRow, levelData);
        
        characterTexture = load_texture(CHARACTER_FILEPATH);
        characterStart = glm::vec3(-5.0f, 3.75f - 1.0f, 0.0f);
        characterPosition = characterStart;
        characterSpeed = 3.0f;
        
        glm::vec3 enemySpawn = glm::vec3(-5.0f, -3.75f + 0.5f, 0.0f);
        
        ghostTexture = load_texture(GHOST_FILEPATH);
        ghostPosition = enemySpawn;
        ghostSpeed = 2.0f;
        float angleG = ((float)rand() / RAND_MAX) * 2 * 3.14159f;
        ghostVelocity = glm::vec3(cos(angleG), sin(angleG), 0.0f) * ghostSpeed;
        ghostChangeTimer = 0.0f;
        
        skeletonTexture = load_texture(SKELETON_FILEPATH);
        skeletonPosition = enemySpawn;
        skeletonSpeed = 2.0f;
        float angleS = ((float)rand() / RAND_MAX) * 2 * 3.14159f;
        skeletonVelocity = glm::vec3(cos(angleS), sin(angleS), 0.0f) * skeletonSpeed;
        skeletonChangeTimer = 0.0f;
    }
    
    ~Level2Scene() {
        delete gameMap;
    }
    
    void processInput() override {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                finished = true;
                g_app_status = TERMINATED;
            }
        }
    }
    
    void update(float deltaTime) override {
        // player movement
        const Uint8* keyState = SDL_GetKeyboardState(NULL);
        glm::vec3 prevPos = characterPosition;
        glm::vec3 movement(0.0f);
        if(keyState[SDL_SCANCODE_W]) { movement.y += 1.0f; }
        if(keyState[SDL_SCANCODE_S]) { movement.y -= 1.0f; }
        if(keyState[SDL_SCANCODE_A]) { movement.x -= 1.0f; }
        if(keyState[SDL_SCANCODE_D]) { movement.x += 1.0f; }
        if(glm::length(movement) > 0.0f)
            movement = glm::normalize(movement);
        characterPosition += movement * characterSpeed * deltaTime;
        
        float spriteSize = 1.0f;
        float leftBoundary   = -5.0f;
        float rightBoundary  = 5.0f - spriteSize;
        float bottomBoundary = -3.75f;
        float topBoundary    = 3.75f - spriteSize;
        if (characterPosition.x < leftBoundary)  characterPosition.x = leftBoundary;
        if (characterPosition.x > rightBoundary) characterPosition.x = rightBoundary;
        if (characterPosition.y < bottomBoundary) characterPosition.y = bottomBoundary;
        if (characterPosition.y > topBoundary)    characterPosition.y = topBoundary;
        
        if(check_coffin_collision(characterPosition, spriteSize)) {
            characterPosition = prevPos;
        }
        
        ghostChangeTimer += deltaTime;
        if(ghostChangeTimer >= 1.0f) {
            float angle = ((float)rand() / RAND_MAX) * 2 * 3.14159f;
            ghostVelocity = glm::vec3(cos(angle), sin(angle), 0.0f) * ghostSpeed;
            ghostChangeTimer = 0.0f;
        }
        ghostPosition += ghostVelocity * deltaTime;
        if (ghostPosition.x < leftBoundary)  ghostPosition.x = leftBoundary;
        if (ghostPosition.x > rightBoundary) ghostPosition.x = rightBoundary;
        if (ghostPosition.y < bottomBoundary) ghostPosition.y = bottomBoundary;
        if (ghostPosition.y > topBoundary)    ghostPosition.y = topBoundary;
        
        skeletonChangeTimer += deltaTime;
        if(skeletonChangeTimer >= 1.0f) {
            float angle = ((float)rand() / RAND_MAX) * 2 * 3.14159f;
            skeletonVelocity = glm::vec3(cos(angle), sin(angle), 0.0f) * skeletonSpeed;
            skeletonChangeTimer = 0.0f;
        }
        skeletonPosition += skeletonVelocity * deltaTime;
        if (skeletonPosition.x < leftBoundary)  skeletonPosition.x = leftBoundary;
        if (skeletonPosition.x > rightBoundary) skeletonPosition.x = rightBoundary;
        if (skeletonPosition.y < bottomBoundary) skeletonPosition.y = bottomBoundary;
        if (skeletonPosition.y > topBoundary)    skeletonPosition.y = topBoundary;
        
        // collision check
        bool collisionGhost = (fabs(characterPosition.x - ghostPosition.x) < 0.8f &&
                               fabs(characterPosition.y - ghostPosition.y) < 0.8f);
        bool collisionSkeleton = (fabs(characterPosition.x - skeletonPosition.x) < 0.8f &&
                                  fabs(characterPosition.y - skeletonPosition.y) < 0.8f);
        if(collisionGhost || collisionSkeleton) {
            if(g_playerLives > 0) {
                g_playerLives--;
                characterPosition = characterStart;
            }
            if(g_playerLives <= 0) {
                finished = true;
            }
        }
        
        if (characterPosition.x >= rightBoundary && characterPosition.y <= bottomBoundary + 0.01f) {
            finished = true;
        }
                
    }
    
    void render() override {
        glClear(GL_COLOR_BUFFER_BIT);
        gameMap->render(&g_shader_program);
        
        render_coffin(&g_shader_program);

        glm::mat4 model = glm::translate(glm::mat4(1.0f), characterPosition);
        g_shader_program.set_model_matrix(model);
        float spriteSize = 1.0f;
        float vertices[] = {
            0.0f,         0.0f,
            spriteSize,   0.0f,
            spriteSize,   spriteSize,
            0.0f,         0.0f,
            spriteSize,   spriteSize,
            0.0f,         spriteSize
        };
        float texCoords[] = {
            0.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 0.0f
        };
        glBindTexture(GL_TEXTURE_2D, characterTexture);
        glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, vertices);
        glEnableVertexAttribArray(g_shader_program.get_position_attribute());
        glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, texCoords);
        glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(g_shader_program.get_position_attribute());
        glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
        
        model = glm::translate(glm::mat4(1.0f), ghostPosition);
        g_shader_program.set_model_matrix(model);
        float ghostVertices[] = {
            0.0f,         0.0f,
            spriteSize,   0.0f,
            spriteSize,   spriteSize,
            0.0f,         0.0f,
            spriteSize,   spriteSize,
            0.0f,         spriteSize
        };
        float ghostTexCoords[] = {
            0.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 0.0f
        };
        glBindTexture(GL_TEXTURE_2D, ghostTexture);
        glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, ghostVertices);
        glEnableVertexAttribArray(g_shader_program.get_position_attribute());
        glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, ghostTexCoords);
        glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(g_shader_program.get_position_attribute());
        glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
        
        model = glm::translate(glm::mat4(1.0f), skeletonPosition);
        g_shader_program.set_model_matrix(model);
        float skeletonVertices[] = {
            0.0f,         0.0f,
            spriteSize,   0.0f,
            spriteSize,   spriteSize,
            0.0f,         0.0f,
            spriteSize,   spriteSize,
            0.0f,         spriteSize
        };
        float skeletonTexCoords[] = {
            0.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 0.0f
        };
        glBindTexture(GL_TEXTURE_2D, skeletonTexture);
        glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, skeletonVertices);
        glEnableVertexAttribArray(g_shader_program.get_position_attribute());
        glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, skeletonTexCoords);
        glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(g_shader_program.get_position_attribute());
        glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
        
        std::string healthText = "Lives: " + std::to_string(g_playerLives);
        draw_text(&g_shader_program, g_font_texture_id, healthText, 0.4f, 0.0f, glm::vec3(1.5f, 3.25f, 0));
        SDL_GL_SwapWindow(g_display_window);
    }
    
    bool isFinished() override {
        return finished;
    }
    
private:
    bool finished;
    GLuint tilemapTexture;
    std::vector<int> levelData;
    GameMap* gameMap;
    
    GLuint characterTexture;
    glm::vec3 characterPosition;
    glm::vec3 characterStart;
    float characterSpeed;
    
    GLuint ghostTexture;
    glm::vec3 ghostPosition;
    glm::vec3 ghostVelocity;
    float ghostSpeed;
    float ghostChangeTimer;
    
    GLuint skeletonTexture;
    glm::vec3 skeletonPosition;
    glm::vec3 skeletonVelocity;
    float skeletonSpeed;
    float skeletonChangeTimer;
};

class Level3Scene : public Scene {
public:
    Level3Scene() : finished(false) {
        srand((unsigned)time(0));
        tilemapTexture = load_texture(TILEMAP_FILEPATH);
        levelData = {
            1, 1, 2, 1, 2, 1, 2, 1, 1, 1,
            1, 2, 1, 1, 1, 1, 1, 1, 2, 1,
            1, 2, 1, 1, 2, 1, 1, 1, 2, 1,
            1, 1, 1, 2, 1, 1, 1, 2, 1, 1,
            1, 1, 2, 2, 2, 2, 2, 1, 2, 1,
            1, 2, 1, 1, 1, 1, 1, 2, 1, 1,
            1, 1, 2, 1, 1, 1, 2, 1, 1, 1,
            
        };
        int mapWidth = 10, mapHeight = 7, tilesPerRow = 8;
        float tileSize = 1.0f;
        gameMap = new GameMap(tilemapTexture, tileSize, mapWidth, mapHeight, tilesPerRow, levelData);
        
        characterTexture = load_texture(CHARACTER_FILEPATH);
        characterStart = glm::vec3(-5.0f, 3.75f - 1.0f, 0.0f);
        characterPosition = characterStart;
        characterSpeed = 3.0f;
        
        glm::vec3 enemySpawn = glm::vec3(-5.0f, -3.75f + 0.5f, 0.0f);
        
        ghostTexture = load_texture(GHOST_FILEPATH);
        ghostPosition = enemySpawn;
        ghostSpeed = 4.0f;
        float angleG = ((float)rand() / RAND_MAX) * 2 * 3.14159f;
        ghostVelocity = glm::vec3(cos(angleG), sin(angleG), 0.0f) * ghostSpeed;
        ghostChangeTimer = 0.0f;
        
        skeletonTexture = load_texture(SKELETON_FILEPATH);
        skeletonPosition = enemySpawn;
        skeletonSpeed = 4.0f;
        float angleS = ((float)rand() / RAND_MAX) * 2 * 3.14159f;
        skeletonVelocity = glm::vec3(cos(angleS), sin(angleS), 0.0f) * skeletonSpeed;
        skeletonChangeTimer = 0.0f;
    }
    
    ~Level3Scene() { delete gameMap; }
    
    void processInput() override {
        SDL_Event event;
        while(SDL_PollEvent(&event)) {
            if(event.type == SDL_QUIT || event.type == SDL_WINDOWEVENT_CLOSE) {
                finished = true;
                g_app_status = TERMINATED;
            }
        }
    }
    
    void update(float deltaTime) override {
        const Uint8* keyState = SDL_GetKeyboardState(NULL);
        glm::vec3 prevPos = characterPosition;
        glm::vec3 movement(0.0f);
        if(keyState[SDL_SCANCODE_W]) movement.y += 1.0f;
        if(keyState[SDL_SCANCODE_S]) movement.y -= 1.0f;
        if(keyState[SDL_SCANCODE_A]) movement.x -= 1.0f;
        if(keyState[SDL_SCANCODE_D]) movement.x += 1.0f;
        if(glm::length(movement) > 0.0f)
            movement = glm::normalize(movement);
        characterPosition += movement * characterSpeed * deltaTime;
        
        float spriteSize = 1.0f;
        float leftBoundary = -5.0f, rightBoundary = 5.0f - spriteSize;
        float bottomBoundary = -3.75f, topBoundary = 3.75f - spriteSize;
        if(characterPosition.x < leftBoundary) characterPosition.x = leftBoundary;
        if(characterPosition.x > rightBoundary) characterPosition.x = rightBoundary;
        if(characterPosition.y < bottomBoundary) characterPosition.y = bottomBoundary;
        if(characterPosition.y > topBoundary) characterPosition.y = topBoundary;
        
        if(check_coffin_collision(characterPosition, spriteSize)) {
            characterPosition = prevPos;
        }
        
        ghostChangeTimer += deltaTime;
        if(ghostChangeTimer >= 1.0f) {
            float angle = ((float)rand() / RAND_MAX) * 2 * 3.14159f;
            ghostVelocity = glm::vec3(cos(angle), sin(angle), 0.0f) * ghostSpeed;
            ghostChangeTimer = 0.0f;
        }
        ghostPosition += ghostVelocity * deltaTime;
        if(ghostPosition.x < leftBoundary)  ghostPosition.x = leftBoundary;
        if(ghostPosition.x > rightBoundary) ghostPosition.x = rightBoundary;
        if(ghostPosition.y < bottomBoundary) ghostPosition.y = bottomBoundary;
        if(ghostPosition.y > topBoundary)    ghostPosition.y = topBoundary;
        
        skeletonChangeTimer += deltaTime;
        if(skeletonChangeTimer >= 1.0f) {
            float angle = ((float)rand() / RAND_MAX) * 2 * 3.14159f;
            skeletonVelocity = glm::vec3(cos(angle), sin(angle), 0.0f) * skeletonSpeed;
            skeletonChangeTimer = 0.0f;
        }
        skeletonPosition += skeletonVelocity * deltaTime;
        if(skeletonPosition.x < leftBoundary)  skeletonPosition.x = leftBoundary;
        if(skeletonPosition.x > rightBoundary) skeletonPosition.x = rightBoundary;
        if(skeletonPosition.y < bottomBoundary) skeletonPosition.y = bottomBoundary;
        if(skeletonPosition.y > topBoundary)    skeletonPosition.y = topBoundary;
        
        bool collisionGhost = (fabs(characterPosition.x - ghostPosition.x) < 0.8f &&
                               fabs(characterPosition.y - ghostPosition.y) < 0.8f);
        bool collisionSkeleton = (fabs(characterPosition.x - skeletonPosition.x) < 0.8f &&
                                  fabs(characterPosition.y - skeletonPosition.y) < 0.8f);
        if(collisionGhost || collisionSkeleton) {
            g_playerLives--;
            characterPosition = characterStart;
            if(g_playerLives <= 0) {
                finished = true;
            }
        }
        
        if(g_playerLives > 0 && characterPosition.x >= rightBoundary && characterPosition.y <= bottomBoundary + 0.01f)
            finished = true;
    }
    
    void render() override {
        glClear(GL_COLOR_BUFFER_BIT);
        gameMap->render(&g_shader_program);
        
        render_coffin(&g_shader_program);
        
        glm::mat4 model = glm::translate(glm::mat4(1.0f), characterPosition);
        g_shader_program.set_model_matrix(model);
        float spriteSize = 1.0f;
        float playerVertices[] = {
            0.0f, 0.0f,
            spriteSize, 0.0f,
            spriteSize, spriteSize,
            0.0f, 0.0f,
            spriteSize, spriteSize,
            0.0f, spriteSize
        };
        float playerTexCoords[] = {
            0.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 0.0f
        };
        glBindTexture(GL_TEXTURE_2D, characterTexture);
        glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, playerVertices);
        glEnableVertexAttribArray(g_shader_program.get_position_attribute());
        glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, playerTexCoords);
        glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(g_shader_program.get_position_attribute());
        glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
        
        model = glm::translate(glm::mat4(1.0f), ghostPosition);
        g_shader_program.set_model_matrix(model);
        float ghostVertices[] = {
            0.0f, 0.0f,
            spriteSize, 0.0f,
            spriteSize, spriteSize,
            0.0f, 0.0f,
            spriteSize, spriteSize,
            0.0f, spriteSize
        };
        float ghostTexCoords[] = {
            0.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 0.0f
        };
        glBindTexture(GL_TEXTURE_2D, ghostTexture);
        glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, ghostVertices);
        glEnableVertexAttribArray(g_shader_program.get_position_attribute());
        glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, ghostTexCoords);
        glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(g_shader_program.get_position_attribute());
        glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
        
        model = glm::translate(glm::mat4(1.0f), skeletonPosition);
        g_shader_program.set_model_matrix(model);
        float skeletonVertices[] = {
            0.0f, 0.0f,
            spriteSize, 0.0f,
            spriteSize, spriteSize,
            0.0f, 0.0f,
            spriteSize, spriteSize,
            0.0f, spriteSize
        };
        float skeletonTexCoords[] = {
            0.0f, 1.0f,
            1.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 1.0f,
            1.0f, 0.0f,
            0.0f, 0.0f
        };
        glBindTexture(GL_TEXTURE_2D, skeletonTexture);
        glVertexAttribPointer(g_shader_program.get_position_attribute(), 2, GL_FLOAT, false, 0, skeletonVertices);
        glEnableVertexAttribArray(g_shader_program.get_position_attribute());
        glVertexAttribPointer(g_shader_program.get_tex_coordinate_attribute(), 2, GL_FLOAT, false, 0, skeletonTexCoords);
        glEnableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
        glDrawArrays(GL_TRIANGLES, 0, 6);
        glDisableVertexAttribArray(g_shader_program.get_position_attribute());
        glDisableVertexAttribArray(g_shader_program.get_tex_coordinate_attribute());
        
        std::string healthText = "Lives: " + std::to_string(g_playerLives);
        draw_text(&g_shader_program, g_font_texture_id, healthText, 0.4f, 0.0f, glm::vec3(1.5f, 3.25f, 0));
        
        SDL_GL_SwapWindow(g_display_window);
    }
    
    bool isFinished() override { return finished; }
    
private:
    bool finished;
    GLuint tilemapTexture;
    std::vector<int> levelData;
    GameMap* gameMap;
    
    GLuint characterTexture;
    glm::vec3 characterPosition;
    glm::vec3 characterStart;
    float characterSpeed;
    
    GLuint ghostTexture;
    glm::vec3 ghostPosition;
    glm::vec3 ghostVelocity;
    float ghostSpeed;
    float ghostChangeTimer;
    
    GLuint skeletonTexture;
    glm::vec3 skeletonPosition;
    glm::vec3 skeletonVelocity;
    float skeletonSpeed;
    float skeletonChangeTimer;
};

void initialise()
{
    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO) < 0) {
        LOG("SDL could not initialize!");
        exit(1);
    }

    g_display_window = SDL_CreateWindow("Project 4: Rise of the AI",
                                        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                        WINDOW_WIDTH, WINDOW_HEIGHT,
                                        SDL_WINDOW_OPENGL);
    if (!g_display_window) {
        LOG("ERROR: SDL Window could not be created.");
        SDL_Quit();
        exit(1);
    }

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

    g_shader_program.load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_model_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);
    g_shader_program.set_projection_matrix(g_projection_matrix);
    g_shader_program.set_view_matrix(g_view_matrix);

    glUseProgram(g_shader_program.get_program_id());
    glClearColor(BG_RED, BG_GREEN, BG_BLUE, BG_OPACITY);

    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    g_font_texture_id = load_texture(FONTSHEET_FILEPATH);
    
    g_coffinTexture = load_texture(COFFIN_FILEPATH);

    if (Mix_OpenAudio(CD_QUAL_FREQ, MIX_DEFAULT_FORMAT, AUDIO_CHAN_AMT, AUDIO_BUFF_SIZE) < 0) {
        std::cerr << "SDL_mixer could not initialize! Error: " << Mix_GetError() << "\n";
    }
    
    g_backgroundMusic = Mix_LoadMUS(BGM_FILEPATH);
    if(g_backgroundMusic == nullptr) {
        std::cerr << "Failed to load background music! Error: " << Mix_GetError() << "\n";
    } else {
        Mix_PlayMusic(g_backgroundMusic, -1);  // Loop indefinitely.
        Mix_VolumeMusic(MIX_MAX_VOLUME / 2);     // Optionally set volume to half.
    }
}


void shutdown() {
    Mix_HaltMusic();
    Mix_FreeMusic(g_backgroundMusic);
    Mix_CloseAudio();
    Mix_Quit();
    SDL_Quit();
}

int main(int argc, char* argv[])
{
    initialise();
    
    Scene* currentScene = new MenuScene();
    Uint32 lastTick = SDL_GetTicks();
    while (g_app_status == RUNNING && !currentScene->isFinished()) {
        Uint32 currentTick = SDL_GetTicks();
        float deltaTime = (currentTick - lastTick) / 1000.0f;
        lastTick = currentTick;
        currentScene->processInput();
        currentScene->update(deltaTime);
        currentScene->render();
    }
    delete currentScene;
    
    // level 1
    if (g_app_status == RUNNING && g_playerLives > 0) {
        currentScene = new Level1Scene();
        lastTick = SDL_GetTicks();
        while (g_app_status == RUNNING && !currentScene->isFinished()) {
            Uint32 currentTick = SDL_GetTicks();
            float deltaTime = (currentTick - lastTick) / 1000.0f;
            lastTick = currentTick;
            currentScene->processInput();
            currentScene->update(deltaTime);
            currentScene->render();
        }
        delete currentScene;
    }
    
    // level 2
    if (g_app_status == RUNNING && g_playerLives > 0) {
        currentScene = new Level2Scene();
        lastTick = SDL_GetTicks();
        while (g_app_status == RUNNING && !currentScene->isFinished()) {
            Uint32 currentTick = SDL_GetTicks();
            float deltaTime = (currentTick - lastTick) / 1000.0f;
            lastTick = currentTick;
            currentScene->processInput();
            currentScene->update(deltaTime);
            currentScene->render();
        }

        delete currentScene;
    }
    
    // level 3
    if (g_app_status == RUNNING && g_playerLives > 0) {
        currentScene = new Level3Scene();
        lastTick = SDL_GetTicks();
        while (g_app_status == RUNNING && !currentScene->isFinished()) {
            Uint32 currentTick = SDL_GetTicks();
            float deltaTime = (currentTick - lastTick) / 1000.0f;
            lastTick = currentTick;
            currentScene->processInput();
            currentScene->update(deltaTime);
            currentScene->render();
        }
        delete currentScene;
        Mix_HaltMusic();

        
        if(g_playerLives > 0) {
             currentScene = new EndScene("You Win!");
        } else {
             currentScene = new EndScene("You Lose!");
        }
        while(g_app_status == RUNNING && !currentScene->isFinished())
        {
             currentScene->processInput();
             currentScene->update(0);
             currentScene->render();
        }
        delete currentScene;
    }
    else
    {
        Mix_HaltMusic();
        currentScene = new EndScene("You Lose!");
        while(g_app_status == RUNNING && !currentScene->isFinished())
        {
            currentScene->processInput();
            currentScene->update(0);
            currentScene->render();
        }
        delete currentScene;
    }
    
    shutdown();
    return 0;
}
