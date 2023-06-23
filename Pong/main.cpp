/**
* Author: [Taehun Kim]
* Assignment: Simple 2D Scene
* Date due: 2023-06-11, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <random>
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"

enum Coordinate
{
    x_coordinate,
    y_coordinate
};

#define LOG(argument) std::cout << argument << '\n'

const int WINDOW_WIDTH = 640,
WINDOW_HEIGHT = 400;

// Set the background to be black
const float BG_RED = 1.0f,
BG_BLUE = 1.0f,
BG_GREEN = 1.0f,
BG_ALPHA = 1.0f;

const int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

SDL_Window* display_window;
bool game_is_running = true;
ShaderProgram program;
glm::mat4 view_matrix, ball_matrix, player1_matrix, player2_matrix, projection_matrix, trans_matrix;
float previous_ticks = 0.0f;
const float MILLISECONDS_IN_SECOND = 1000.0;

// texture
const int NUMBER_OF_TEXTURES = 1; // to be generated, that is
const GLint LEVEL_OF_DETAIL = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER = 0;   // this value MUST be zero
const char BALL_SPRITE_FILEPATH[] = "ball.png";
const char PLAYER1_SPRITE_FILEPATH[] = "player.png";
const char PLAYER2_SPRITE_FILEPATH[] = "player.png";



GLuint ball_texture_id;
GLuint player1_texture_id;
GLuint player2_texture_id;

glm::vec3 g_player1_movement = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 g_player2_movement = glm::vec3(0.0f, 0.0f, 0.0f);


float BALL_MOVEMENT_X;
float BALL_MOVEMENT_Y;
float scale_vertices = 0.15f;
float scale_vertices_player = 0.5f;

glm::vec3 ball_position = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 player1_position = glm::vec3(-5.0f + scale_vertices_player, 0.0f, 0.0f);
glm::vec3 player2_position = glm::vec3(5.0f - scale_vertices_player, 0.0f, 0.0f);
// Window size
float LEFT_BOUND = -5.0f + scale_vertices;
float RIGHT_BOUND = 5.0f - scale_vertices;
float TOP_BOUND = 3.75 - scale_vertices;
float BOT_BOUND = -3.75 + scale_vertices;

const float MINIMUM_COLLISION_DISTANCE = 0.35f;

int PLAYER1_SCORE = 0;
int PLAYER2_SCORE = 0;

// LOADING TEXTURE
GLuint load_texture(const char* filepath)
{
    // STEP 1: Loading the image file
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    // STEP 2: Generating and binding a texture ID to our image
    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    // STEP 3: Setting our texture filter parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    // STEP 4: Releasing our file from memory and returning our texture id
    stbi_image_free(image);

    return textureID;
}


// INITIALIZATION
void initialize()
{
    // INITIALIZE VIDEO
    SDL_Init(SDL_INIT_VIDEO);

    display_window = SDL_CreateWindow("Assignment 1",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(display_window);
    SDL_GL_MakeCurrent(display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);
    program.Load(V_SHADER_PATH, F_SHADER_PATH);
    ball_matrix = glm::mat4(1.0f);
    player1_matrix = glm::mat4(1.0f);
    player2_matrix = glm::mat4(1.0f);

    view_matrix = glm::mat4(1.0f);
    projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);  // Defines the characteristics of your camera, such as clip planes, field of view, projection method etc.

    program.SetProjectionMatrix(projection_matrix);
    program.SetViewMatrix(view_matrix);

    glUseProgram(program.programID);

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_ALPHA);
    ball_texture_id = load_texture(BALL_SPRITE_FILEPATH);
    player1_texture_id = load_texture(PLAYER1_SPRITE_FILEPATH);
    player2_texture_id = load_texture(PLAYER2_SPRITE_FILEPATH);

    // emable blending
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    ball_matrix = glm::translate(ball_matrix, ball_position);
    player1_matrix = glm::translate(player1_matrix, player1_position);
    player2_matrix = glm::translate(player2_matrix, player2_position);


    ball_position.x = 0.0f;
    ball_position.y = 0.0f;

    // randomly chooses which way the ball starts after the first point
    std::random_device rd;
    std::mt19937 gen(rd());

    int option1 = -2;
    int option2 = 2;

    std::uniform_int_distribution<int> dist(0, 1);

    int random_numberX = dist(gen);
    int random_numberY = dist(gen);

    int selected_optionX = (random_numberX == 0) ? option1 : option2;
    int selected_optionY = (random_numberY == 0) ? option1 : option2;

    BALL_MOVEMENT_X = selected_optionX;
    BALL_MOVEMENT_Y = selected_optionY;
}

void process_input()
{
    // VERY IMPORTANT: If nothing is pressed, we don't want to go anywhere
    g_player1_movement = glm::vec3(0.0f);
    g_player2_movement = glm::vec3(0.0f);

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
         // End game
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            game_is_running = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_q:
                // Quit the game with a keystroke
                game_is_running = false;
                break;

            default:
                break;
            }

            break;
        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);


    if (key_state[SDL_SCANCODE_UP] && player2_position.y < 3.75f - 0.5f)
    {
        g_player2_movement.y = 4.0f;
    }
    else if (key_state[SDL_SCANCODE_DOWN] && player2_position.y > -3.75f + 0.5f)
    {
        g_player2_movement.y = -4.0f;
    }
    if (key_state[SDL_SCANCODE_E] && player1_position.y < 3.75f - 0.5f)
    {
        g_player1_movement.y = 4.0f;
    }
    else if (key_state[SDL_SCANCODE_D] && player1_position.y > -3.75f + 0.5f)
    {
        g_player1_movement.y = -4.0f;
    }
}

bool check_collision(glm::vec3& position_a, glm::vec3& position_b)
{
    return sqrt(pow(position_b[0] - position_a[0], 2) + pow(position_b[1] - position_a[1], 2)) < MINIMUM_COLLISION_DISTANCE;
}

// update
void update()
{

    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - previous_ticks; // the delta time is the difference from the last frame
    previous_ticks = ticks;


    if (ball_position.x <= LEFT_BOUND) {
        game_is_running = false;
        std::cout << "PLAYER 2 WINS! YOU SUCK PLAYER1!";
    }
    if (ball_position.x >= RIGHT_BOUND) {
        game_is_running = false;
        std::cout << "PLAYER 1 WINS! YOU SUCK PLAYER2!";
    }

    if (ball_position.y >= TOP_BOUND) {
        BALL_MOVEMENT_Y = -3.0f;
    }
    if (ball_position.y <= BOT_BOUND) {
        BALL_MOVEMENT_Y = 3.0f;
    }

    if (check_collision(player1_position, ball_position))
    {
        BALL_MOVEMENT_X = 3.0f;
    }

    if (check_collision(player2_position, ball_position))
    {
        BALL_MOVEMENT_X = -3.0f;
    }

    ball_position.x += BALL_MOVEMENT_X * delta_time;
    ball_position.y += BALL_MOVEMENT_Y * delta_time;
    ball_matrix = glm::mat4(1.0f);
    ball_matrix = glm::translate(ball_matrix, glm::vec3(ball_position.x, ball_position.y, 0.0f));

    player2_position += g_player2_movement * delta_time;
    player2_matrix = glm::mat4(1.0f);  // Reset the player1_matrix to an identity matrix
    player2_matrix = glm::translate(player2_matrix, glm::vec3(player2_position.x, player2_position.y, 0.0f));

    player1_position += g_player1_movement * delta_time;
    player1_matrix = glm::mat4(1.0f);  // Reset the player1_matrix to an identity matrix
    player1_matrix = glm::translate(player1_matrix, glm::vec3(player1_position.x, player1_position.y, 0.0f));




    
}

// draw
void draw_object(glm::mat4& object_model_matrix, GLuint object_texture_id)
{
    program.SetModelMatrix(object_model_matrix);
    glBindTexture(GL_TEXTURE_2D, object_texture_id);
    glDrawArrays(GL_TRIANGLES, 0, 6); // we are now drawing 2 triangles, so we use 6 instead of 3
}

void render() {
    glClear(GL_COLOR_BUFFER_BIT);

 
// Vertices for the Ball
    float vertices[] = {
        -1.0f, -1.0f, // bottom left
        1.0f, -1.0f,  // bottom right
        1.0f, 1.0f,   // top right

        -1.0f, -1.0f, // bottom left
        1.0f, 1.0f,   // top right
        -1.0f, 1.0f   // top left
    };

 // Vertices for the Player
    float vertices_player[] = {
        -1.0f, -1.0f, // bottom left
        1.0f, -1.0f,  // bottom right
        1.0f, 1.0f,   // top right

        -1.0f, -1.0f, // bottom left
        1.0f, 1.0f,   // top right
        -1.0f, 1.0f   // top left
    };
// Textures
    float texture_coordinates[] = {
        0.0f, 1.0f, // bottom left
        1.0f, 1.0f, // bottom right
        1.0f, 0.0f, // top right

        0.0f, 1.0f, // bottom left
        1.0f, 0.0f, // top right
        0.0f, 0.0f  // top left
    };


    glVertexAttribPointer(program.texCoordAttribute, 2, GL_FLOAT, false, 0, texture_coordinates);
    glEnableVertexAttribArray(program.texCoordAttribute);

    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices_player);
    glEnableVertexAttribArray(program.positionAttribute);

    for (int i = 0; i < 12; i++) {
        vertices_player[i] *= scale_vertices_player;
    }

    draw_object(player1_matrix, player1_texture_id);
    draw_object(player2_matrix, player2_texture_id);

    glVertexAttribPointer(program.positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program.positionAttribute);
   
    for (int i = 0; i < 12; i++) {
        vertices[i] *= scale_vertices;
    }

    draw_object(ball_matrix, ball_texture_id);


    // We disable two attribute arrays now
    glDisableVertexAttribArray(program.positionAttribute);
    glDisableVertexAttribArray(program.texCoordAttribute);

    SDL_GL_SwapWindow(display_window);
}

void shutdown()
{
    SDL_Quit();
}

int main(int argc, char* argv[])
{
    initialize();

    while (game_is_running)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}