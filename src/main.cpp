#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include "shader.h"

#include <bits/stdc++.h>
#include <ctime>

#ifndef STB_IMAGE_IMPLEMENTATION
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#endif

#include "background/background.cpp"
#include "background/loss.cpp"
#include "background/win.cpp"
#include "coins/coins.cpp"
#include "zappers/zappers.cpp"

#include <ft2build.h>
#include FT_FREETYPE_H

#define SCREEN_HEIGHT 1080.0f
#define SCREEN_WIDTH 1920.0f

#define INITIAL_VELOCITY 0.0f
#define JETPACK_ACCELERATION 1000.0f

#define LEVEL_LENGTH 2000

GLfloat ypos = 0.0f, yspeed = 0.0f, gravity = 500.0f, jetpack, flytime = 0.0f, maxyspeed = 0.0f, maxypos = 0.0f, prevyspeed = 0.0f, prevypos = 0.0f;
int airflag = 0, ceilflag = 0;

/// Holds all state information relevant to a character as loaded using FreeType
struct Character {
    unsigned int TextureID; // ID handle of the glyph texture
    glm::ivec2   Size;      // Size of glyph
    glm::ivec2   Bearing;   // Offset from baseline to left/top of glyph
    unsigned int Advance;   // Horizontal offset to advance to next glyph
};

std::map<GLchar, Character> Characters;
unsigned int textVAO, textVBO;

void setySpeed(GLfloat t)
{
    yspeed = prevyspeed + (jetpack -gravity) * t;
}

void setyPos(GLfloat t)
{
    ypos = prevypos + prevyspeed * t + 0.5 * (jetpack - gravity) * t * t;
}

void processInput(GLFWwindow *window, int *imgindex)
{
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
    {
        *imgindex = 5;
        if (airflag == 0) flytime = 0;
        airflag = 1;
        jetpack = JETPACK_ACCELERATION;
        setySpeed(flytime);
        setyPos(flytime);
        flytime += 0.001f;
        // std::cout << "flying until top with jetpack" << std::endl;
    }
    else if (airflag == 1)
    {
        airflag = 0;
        flytime = 0;
        maxyspeed = yspeed;
        maxypos = ypos;
        prevypos = ypos;
        prevyspeed = yspeed;
    }
    
}

int checkCollision(glm::mat4 vertices1, glm::mat4 vertices2)
{
    // iterate through all edges of v1
    for(int i=0; i<4; i++)
    {
        // iterate through all vertices of v2
        for(int j=0; j<4; j++)
        {
            float x = vertices2[j][0];
            float y = vertices2[j][1];
            float x1 = vertices1[i][0];
            float y1 = vertices1[i][1];
            float x2 = vertices1[(i+1)%4][0];
            float y2 = vertices1[(i+1)%4][1];
            float dist = sqrtf((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2));
            float dist1 = sqrtf((x1 - x)*(x1 - x) + (y1 - y)*(y1 - y));
            float dist2 = sqrtf((x - x2)*(x - x2) + (y - y2)*(y - y2));

            if ((dist1 + dist2 - dist) < 0.001) return 1;
        }
    }
    // iterate through all edges of v2
    for(int i=0; i<4; i++)
    {
        // iterate through all vertices of v1
        for(int j=0; j<4; j++)
        {
            float x = vertices1[j][0];
            float y = vertices1[j][1];
            float x1 = vertices2[i][0];
            float y1 = vertices2[i][1];
            float x2 = vertices2[(i+1)%4][0];
            float y2 = vertices2[(i+1)%4][1];
            float dist = sqrtf((x1 - x2)*(x1 - x2) + (y1 - y2)*(y1 - y2));
            float dist1 = sqrtf((x1 - x)*(x1 - x) + (y1 - y)*(y1 - y));
            float dist2 = sqrtf((x - x2)*(x - x2) + (y - y2)*(y - y2));

            if ((dist1 + dist2 - dist) < 0.001) return 1;
        }
    }
    return 0;
}

// int transform_arrays(GLfloat vertices1)

// glfw: whenever the window size changed (by OS or user resize) this callback function executes
// ---------------------------------------------------------------------------------------------
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    // make sure the viewport matches the new window dimensions; note that width and 
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);
}

void RenderText(Shader &shader, std::string text, float x, float y, float scale, glm::vec3 color)
{
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glGenVertexArrays(1, &textVAO);
    glGenBuffers(1, &textVBO);
    glBindVertexArray(textVAO);
    glBindBuffer(GL_ARRAY_BUFFER, textVBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(float) * 6 * 4, NULL, GL_DYNAMIC_DRAW);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), 0);
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);


    // activate corresponding render state	
    shader.use();
    glUniform3f(glGetUniformLocation(shader.ID, "textColor"), color.x, color.y, color.z);
    glActiveTexture(GL_TEXTURE0);
    glBindVertexArray(textVAO);

    // iterate through all characters
    std::string::const_iterator c;
    for (c = text.begin(); c != text.end(); c++) 
    {
        Character ch = Characters[*c];

        float xpos = x + ch.Bearing.x * scale;
        float ypos = y - (ch.Size.y - ch.Bearing.y) * scale;

        float w = ch.Size.x * scale;
        float h = ch.Size.y * scale;
        // update VBO for each character
        float vertices[6][4] = {
            { xpos,     ypos + h,   0.0f, 0.0f },            
            { xpos,     ypos,       0.0f, 1.0f },
            { xpos + w, ypos,       1.0f, 1.0f },

            { xpos,     ypos + h,   0.0f, 0.0f },
            { xpos + w, ypos,       1.0f, 1.0f },
            { xpos + w, ypos + h,   1.0f, 0.0f }           
        };
        // render glyph texture over quad
        glBindTexture(GL_TEXTURE_2D, ch.TextureID);
        // update content of VBO memory
        glBindBuffer(GL_ARRAY_BUFFER, textVBO);
        glBufferSubData(GL_ARRAY_BUFFER, 0, sizeof(vertices), vertices); // be sure to use glBufferSubData and not glBufferData

        glBindBuffer(GL_ARRAY_BUFFER, 0);
        // render quad
        glDrawArrays(GL_TRIANGLES, 0, 6);
        // now advance cursors for next glyph (note that advance is number of 1/64 pixels)
        x += (ch.Advance >> 6) * scale; // bitshift by 6 to get value in pixels (2^6 = 64 (divide amount of 1/64th pixels by 64 to get amount of pixels))
    }
    glBindVertexArray(0);
    glBindTexture(GL_TEXTURE_2D, 0);
}

int main()
{
    glfwInit();
    
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow *window = glfwCreateWindow(SCREEN_WIDTH, SCREEN_HEIGHT, "Jetpack Joyride", NULL, NULL);
    if (window == NULL)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }

    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    GLfloat vertices[] = {
        -0.68f, -0.87f, 0.0f,         0.0f, 0.0f,
        -0.555f, -0.87f, 0.0f,       1.0f, 0.0f, 
        -0.555f, -0.62f, 0.0f,      1.0f, 1.0f,
        -0.68f, -0.62f, 0.0f,        0.0f, 1.0f
    };

    GLfloat playerverts[4][4];
    for(int i=0; i<4; i++)
    {
        for(int j=0; j<3; j++)
        {
            playerverts[i][j] = vertices[i*5 + j];
        }
        playerverts[i][3] = 1.0f;
    }

    GLfloat zappervertices[] = {
        1.00f, -0.40f, 0.0f,     0.0f, 0.0f,
        1.12f, -0.40f, 0.0f,      1.0f, 0.0f,
        1.12f, 0.40f, 0.0f,       1.0f, 1.0f,
        1.00f, 0.40f, 0.0f,      0.0f, 1.0f
    };

    GLfloat coinvertices[] = {
        -0.10f, -0.10f, 0.0f,     0.0f, 0.0f,
        0.00f, -0.10f, 0.0f,      1.0f, 0.0f,
        0.00f, 0.10f, 0.0f,       1.0f, 1.0f,
        -0.10f, 0.10f, 0.0f,      0.0f, 1.0f
    };

    GLfloat zapperverts[4][4];
    for(int i=0; i<4; i++)
    {
        for(int j=0; j<3; j++)
        {
            zapperverts[i][j] = zappervertices[i*5 + j];
        }
        zapperverts[i][3] = 1.0f;
    }

    GLfloat coinverts[4][4];
    for(int i=0; i<4; i++)
    {
        for(int j=0; j<3; j++)
        {
            coinverts[i][j] = coinvertices[i*5 + j];
        }
        coinverts[i][3] = 1.0f;
    }

    GLuint indices[] = {
        0, 1, 2,                // triangle player
        0, 2, 3,
    };
    
    gladLoadGL();

    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    GLuint bgtexture, bgVAO;
    bgsetup(&bgVAO, &bgtexture);

    GLuint cointexture, coinVAO;
    coinsetup(&coinVAO, &cointexture, coinvertices);

    GLuint lossbgtexture, lossbgVAO;
    lossbgsetup(&lossbgVAO, &lossbgtexture);

    GLuint winbgtexture, winbgVAO;
    winbgsetup(&winbgVAO, &winbgtexture);

    GLuint zappertexture, zapperVAO;
    zappersetup(&zapperVAO, &zappertexture, zappervertices);

    GLuint VAO, VBO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(unsigned int)));
    // glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);

    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);

    glBindVertexArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);


    unsigned int texture;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    // set the texture wrapping/filtering options (on the currently bound texture object)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);  


    glClearColor(0.07f, 0.13f, 0.17f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glfwSwapBuffers(window);

    Shader ourShader("../src/vshader.vs", "../src/fshader.fs");
    Shader textShader("../src/textshader.vs", "../src/textshader.fs");
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINES);
    int time = 0;

    const char images[6][20] = {"../src/images/1.png", "../src/images/2.png", "../src/images/3.png", "../src/images/4.png", "../src/images/5.png", "../src/images/6.png"};
    int imgindex = 0;

    GLfloat bgshift = 0.0f, zappershift = 0.0f;

    glm::mat4 model = glm::mat4(1.0f);
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINES);

    GLfloat levels[3][4];
    levels[0][0] = 3 + rand()%2;
    levels[0][1] = 10;
    levels[0][2] = 0.004;
    levels[0][3] = 0;
    levels[1][0] = 4 + rand()%2;
    levels[1][1] = 10;
    levels[1][2] = 0.006;
    levels[1][3] = 3 + rand()%3;
    levels[2][0] = 4;
    levels[2][1] = 10;
    levels[2][2] = 0.006;
    levels[2][3] = 10 + rand()%10;

    int loss = 0, win = 0, score = 0;

    int level = 0, leveltime = 0;

    int numOfZappers = 4;
    time_t curtime;
    curtime = std::time(NULL);
    srand(curtime);
    GLfloat zapperdisp[4][5];

    for(int i=0; i<numOfZappers; i++)
    {
        if (i%2==0) zapperdisp[i][0] = rand()/(float)RAND_MAX * 0.75 - 0.75;
        else zapperdisp[i][0] = rand()/(float)RAND_MAX * 0.75;
        zapperdisp[i][1] = i * 2.4f/numOfZappers;
        zapperdisp[i][2] = rand() % (11 + (int)levels[level][3]);
        zapperdisp[i][3] = 0;
        zapperdisp[i][4] = 0;
    }

    int numOfCoins = 10;
    GLfloat coindisp[10][3];

    for(int i=0; i<numOfCoins; i++)
    {
        coindisp[i][0] = rand()/(float)RAND_MAX * 1.5 - 0.75;
        coindisp[i][1] = i * 3.2f/numOfCoins;
        coindisp[i][2] = 0;
    }

    ourShader.setInt("glow", 0);
    glm::vec4 midpoint = glm::vec4(0, 0, 0, 1);
    ourShader.setVec4("midpoint", midpoint);

    glEnable(GL_CULL_FACE);
    glEnable(GL_BLEND);
    glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);

    // FreeType
    // --------
    FT_Library ft;
    // All functions return a value different than 0 whenever an error occurred
    if (FT_Init_FreeType(&ft))
    {
        std::cout << "ERROR::FREETYPE: Could not init FreeType Library" << std::endl;
        return -1;
    }

	// find path to font
    std::string font_name = "../src/fonts/Antonio-Bold.ttf";
    if (font_name.empty())
    {
        std::cout << "ERROR::FREETYPE: Failed to load font_name" << std::endl;
        return -1;
    }
	
	// load font as face
    FT_Face face;
    if (FT_New_Face(ft, font_name.c_str(), 0, &face)) {
        std::cout << "ERROR::FREETYPE: Failed to load font" << std::endl;
        return -1;
    }
    else {
        // set size to load glyphs as
        FT_Set_Pixel_Sizes(face, 0, 48);

        // disable byte-alignment restriction
        glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

        // load first 128 characters of ASCII set
        for (unsigned char c = 0; c < 128; c++)
        {
            // Load character glyph 
            if (FT_Load_Char(face, c, FT_LOAD_RENDER))
            {
                std::cout << "ERROR::FREETYTPE: Failed to load Glyph" << std::endl;
                continue;
            }
            // generate texture
            unsigned int texture;
            glGenTextures(1, &texture);
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexImage2D(
                GL_TEXTURE_2D,
                0,
                GL_RED,
                face->glyph->bitmap.width,
                face->glyph->bitmap.rows,
                0,
                GL_RED,
                GL_UNSIGNED_BYTE,
                face->glyph->bitmap.buffer
            );
            // set texture options
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            // now store character for later use
            Character character = {
                texture,
                glm::ivec2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
                glm::ivec2(face->glyph->bitmap_left, face->glyph->bitmap_top),
                static_cast<unsigned int>(face->glyph->advance.x)
            };
            Characters.insert(std::pair<char, Character>(c, character));
        }
        glBindTexture(GL_TEXTURE_2D, 0);
    }
    // destroy FreeType once we're finished
    FT_Done_Face(face);
    FT_Done_FreeType(ft);

    while(!glfwWindowShouldClose(window))
    {
        if (loss)
            std::cout << "You lost, your score is: " << score << std::endl;
        else if (win)
            std::cout << "You won, your score is: " << score << std::endl;
        
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        processInput(window, &imgindex);

        ourShader.use();

        if (loss & !win)
        {
            model = glm::mat4(1);
            ourShader.setMat4("model", model);
            ourShader.setInt("glow", 0);
            lossbgwhile(&lossbgVAO, &lossbgtexture, ourShader);

            std::string scoretext = "YOU LOSE! Your score is: " + std::to_string(score);
            RenderText(textShader, scoretext, -0.55f, -0.9f, 0.0025f, glm::vec3(0.3, 0.7f, 0.9f));
        }
        else if (win & !loss)
        {
            model = glm::mat4(1);
            ourShader.setMat4("model", model);
            ourShader.setInt("glow", 0);
            winbgwhile(&winbgVAO, &winbgtexture, ourShader);

            std::string scoretext = "Congrats! Your score is: " + std::to_string(score);
            RenderText(textShader, scoretext, -0.55f, -0.95f, 0.0025f, glm::vec3(0.3, 0.7f, 0.9f));
        }
        else
        {
            if (time >= 20 / levels[level][2] * 0.004)
            {
                time = 0;
                imgindex = (imgindex + 1) % 4;
                if (ypos > 0)
                {
                    imgindex = 4;
                }
            }
            time++;
            leveltime++;
            if (leveltime > LEVEL_LENGTH)   // change this step value to increase level length
            {
                level++;
                leveltime = 0;
                std::cout << "NEW LEVEL" << std::endl;

                for(int i=0; i<levels[level][0]; i++)
                {
                    if (i%2==0) zapperdisp[i][0] = rand()/(float)RAND_MAX * 0.75 - 0.75;
                    else zapperdisp[i][0] = rand()/(float)RAND_MAX * 0.75;
                    zapperdisp[i][1] = i * 2.4f/levels[level][0];
                    zapperdisp[i][2] = rand() % (11 + (int)levels[level][3]);
                    zapperdisp[i][3] = 0;
                    zapperdisp[i][4] = 0;
                }

                for(int i=0; i<levels[level][1]; i++)
                {
                    coindisp[i][0] = rand()/(float)RAND_MAX * 1.5 - 0.75;
                    coindisp[i][1] = i * 3.2f/levels[level][1];
                    coindisp[i][2] = 0;
                }
            }
            if (level >= 3){
                win = 1;
                loss = 0;
            }

            numOfZappers = levels[level][0];
            numOfCoins = levels[level][1];

            if (ypos < 0)
            {
                ypos = 0;
                flytime = 0;
                prevypos = 0;
                prevyspeed = 0;
            }
            if (ypos > 1.5 || (airflag == 1 && round(ypos * 1000000)/(float)1000000 == 1.5))
            {
                ypos = 1.5;
                flytime = 0;
                prevypos = 1.5;
                if (airflag == 0)
                {
                    prevyspeed = -0.3 * yspeed;
                    maxypos = 1.5;
                    maxyspeed = -0.3 * yspeed;
                }
                else
                {
                    prevyspeed = 0;
                    maxyspeed = 0;
                }

            }

            if (airflag == 0 && ypos>0)
            {
                if (round((maxypos + maxyspeed * flytime - 0.5 * gravity * flytime * flytime)*100000) == 0 || (maxypos + maxyspeed * flytime - 0.5 * gravity * flytime * flytime < 0))
                {
                    yspeed = 0.0f;
                    flytime = 0.0f;
                    ypos = 0.0f;
                    // std::cout << "at ground" << std::endl;
                    prevyspeed = yspeed;
                    prevypos = ypos;
                }
                else
                {
                    yspeed = maxyspeed - gravity * flytime;
                    ypos = maxypos + maxyspeed * flytime - 0.5 * gravity * flytime * flytime;
                    flytime += 0.001f;
                    // std::cout << "freefall" << std::endl;
                    prevyspeed = yspeed;
                    prevypos = ypos;
                }
            }

            for(int i=0; i<numOfZappers; i++)
            {
                if (zapperdisp[i][1] < -2.25)
                {
                    if (i%2==0) zapperdisp[i][0] = rand()/(float)RAND_MAX * 0.75 - 0.75;
                    else zapperdisp[i][0] = rand()/(float)RAND_MAX * 0.75;
                    zapperdisp[i][1] = zapperdisp[(i+numOfZappers-1)%numOfZappers][1] + 2.4f/numOfZappers;
                    zapperdisp[i][2] = rand() % (11 + (int)levels[level][3]);
                    zapperdisp[i][3] = 0;
                    zapperdisp[i][4] = 0;
                    // std::cout << zapperdisp[i][0] << std::endl;
                }
                else zapperdisp[i][1] -= levels[level][2];
            }

            for(int i=0; i<numOfCoins; i++)
            {
                if (coindisp[i][1] < -2.25)
                {
                    coindisp[i][0] = rand()/(float)RAND_MAX * 1.5 - 0.75;
                    coindisp[i][1] = coindisp[(i+numOfCoins-1)%numOfCoins][1] + 3.2f/numOfCoins;
                    coindisp[i][2] = 0;
                    // std::cout << zapperdisp[i][0] << std::endl;
                }
                else coindisp[i][1] -= levels[level][2];
            }

            ourShader.setInt("glow", 0);
            midpoint = glm::vec4(0, 0, 0, 1);
            ourShader.setVec4("midpoint", midpoint);
            bgwhile(&bgVAO, &bgtexture, ourShader);
            ourShader.setInt("glow", 2);
            
            for(int i=0; i<numOfZappers; i++)
            {
                model[3][0] = zapperdisp[i][1];
                model[3][1] = zapperdisp[i][0];
                glm::mat4 translate = glm::mat4(1);
                translate[3][0] = -1.06f - zapperdisp[i][1];
                translate[3][1] = 0.0f - zapperdisp[i][0];
                glm::mat4 translateback = glm::mat4(1);
                translateback[3][0] = 1.06f + zapperdisp[i][1];
                translateback[3][1] = 0.0f + zapperdisp[i][0];
                glm::mat4 rotate = glm::mat4(1);
                float theta;
                if (zapperdisp[i][2] >= 0 && zapperdisp[i][2] <= 2)
                    theta = 0.0f;
                else if (zapperdisp[i][2] >= 6 && zapperdisp[i][2] <= 7)
                    theta = 45.0f;
                else if (zapperdisp[i][2] >= 3 && zapperdisp[i][2] <= 5)
                    theta = 90.0f;
                else if (zapperdisp[i][2] >= 8 && zapperdisp[i][2] <= 9)
                    theta = -45.0f;
                else if (zapperdisp[i][2] >= 10)
                {
                    theta = zapperdisp[i][3];
                    zapperdisp[i][3] += 1.0f;
                }    
                
                rotate[1][1] = cos(glm::radians(theta));
                rotate[1][0] = -sin(glm::radians(theta));
                rotate[0][1] = sin(glm::radians(theta));
                rotate[0][0] = cos(glm::radians(theta));
                glm::mat4 scale = glm::mat4(1);
                if (zapperdisp[i][2] > 2)
                {
                    scale[0][0] = SCREEN_HEIGHT/SCREEN_WIDTH;
                }
                glm::mat4 zappermodel = translateback * scale * rotate * translate * model; // scaling issue
                
                ourShader.setMat4("model", zappermodel);
                int collision = 0;

                glm::vec4 vec1 = zappermodel * glm::make_vec4(zapperverts[0]);
                glm::vec4 vec2 = zappermodel * glm::make_vec4(zapperverts[1]);
                glm::vec4 vec3 = zappermodel * glm::make_vec4(zapperverts[2]);
                glm::vec4 vec4 = zappermodel * glm::make_vec4(zapperverts[3]);
                
                glm::mat4 zapperverts2 = glm::mat4(1);

                for(int i=0; i<4; i++)
                {
                    zapperverts2[0][i] = vec1[i];
                    zapperverts2[1][i] = vec2[i];
                    zapperverts2[2][i] = vec3[i];
                    zapperverts2[3][i] = vec4[i];
                }
                glm::mat4 playermodel = glm::mat4(1);
                playermodel[3][1] = ypos;

                vec1 = playermodel * glm::make_vec4(playerverts[0]);
                vec2 = playermodel * glm::make_vec4(playerverts[1]);
                vec3 = playermodel * glm::make_vec4(playerverts[2]);
                vec4 = playermodel * glm::make_vec4(playerverts[3]);
                
                glm::mat4 playerverts2 = glm::mat4(1);

                for(int i=0; i<4; i++)
                {
                    playerverts2[0][i] = vec1[i];
                    playerverts2[1][i] = vec2[i];
                    playerverts2[2][i] = vec3[i];
                    playerverts2[3][i] = vec4[i];
                }

                if (checkCollision(zapperverts2, playerverts2))
                {
                    if (loss == 0)
                        // std::cout << "Collision " << zapperdisp[i][2] << "\n" << std::endl; 
                    zapperdisp[i][4] = 1;
                    loss = 1;
                    win = 0;
                }
                midpoint[0] = (zapperverts2[0][0] + zapperverts2[1][0] + zapperverts2[2][0] + zapperverts2[3][0]) / 4;
                midpoint[1] = (zapperverts2[0][1] + zapperverts2[1][1] + zapperverts2[2][1] + zapperverts2[3][1]) / 4;
                midpoint[2] = 0.0f;
                midpoint[3] = 1.0f;
                ourShader.setVec4("midpoint", midpoint);
                if (zapperdisp[i][4] == 0)
                    zapperwhile(&zapperVAO, &zappertexture, ourShader);
            }
            ourShader.setInt("glow", 0);
            for(int i=0; i<numOfCoins; i++)
            {
                glm::mat4 coinverts2 = glm::mat4(1);
                glm::mat4 coinmodel = glm::mat4(1); // scaling issue
                coinmodel[3][0] = coindisp[i][1];
                coinmodel[3][1] = coindisp[i][0];
                
                ourShader.setMat4("model", coinmodel);
                int collision = 0;

                glm::vec4 vec1 = coinmodel * glm::make_vec4(coinverts[0]);
                glm::vec4 vec2 = coinmodel * glm::make_vec4(coinverts[1]);
                glm::vec4 vec3 = coinmodel * glm::make_vec4(coinverts[2]);
                glm::vec4 vec4 = coinmodel * glm::make_vec4(coinverts[3]);

                for(int i=0; i<4; i++)
                {
                    coinverts2[0][i] = vec1[i];
                    coinverts2[1][i] = vec2[i];
                    coinverts2[2][i] = vec3[i];
                    coinverts2[3][i] = vec4[i];
                }

                glm::mat4 playermodel = glm::mat4(1);
                playermodel[3][1] = ypos;

                vec1 = playermodel * glm::make_vec4(playerverts[0]);
                vec2 = playermodel * glm::make_vec4(playerverts[1]);
                vec3 = playermodel * glm::make_vec4(playerverts[2]);
                vec4 = playermodel * glm::make_vec4(playerverts[3]);
                
                glm::mat4 playerverts2 = glm::mat4(1);

                for(int i=0; i<4; i++)
                {
                    playerverts2[0][i] = vec1[i];
                    playerverts2[1][i] = vec2[i];
                    playerverts2[2][i] = vec3[i];
                    playerverts2[3][i] = vec4[i];
                }

                if (checkCollision(coinverts2, playerverts2))
                {
                    if (coindisp[i][2] == 0)
                    {
                        score++;
                        std:: cout << "score: " << score << std::endl;
                    }
                    coindisp[i][2] = 1;
                }
                midpoint[0] = (playerverts2[0][0] + playerverts2[1][0] + playerverts2[2][0] + playerverts2[3][0]) / 4;
                midpoint[1] = (playerverts2[0][1] + playerverts2[1][1] + playerverts2[2][1] + playerverts2[3][1]) / 4;
                midpoint[2] = 0.0f;
                midpoint[3] = 1.0f;
                ourShader.setVec4("midpoint", midpoint);
                if (coindisp[i][2] == 0)
                    coinwhile(&coinVAO, &cointexture, ourShader);
            }
            if (airflag == 1) ourShader.setInt("glow", 1);
            else ourShader.setInt("glow", 0);
            std::string scoretext = "Score: "+std::to_string(score);
            std::string leveltext = "Level: "+std::to_string(level+1);
            std::string leveldist = "Distance: "+std::to_string((int)(leveltime/10 * levels[level][2]/0.004))+"/"+std::to_string((int)(LEVEL_LENGTH/10 * levels[level][2]/0.004))+" m";

            RenderText(textShader, scoretext, -0.95f, 0.8f, 0.0025f, glm::vec3(0.3, 0.7f, 0.9f));
            RenderText(textShader, leveltext, -0.4f, 0.8f, 0.0025f, glm::vec3(0.3, 0.7f, 0.9f));
            RenderText(textShader, leveldist, 0.05f, 0.8f, 0.0025f, glm::vec3(0.3, 0.7f, 0.9f));
            ourShader.use();           
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, texture);

            // load and generate the texture
            int width, height, nrChannels;
            glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
            stbi_set_flip_vertically_on_load(true);
            unsigned char *data = stbi_load(images[imgindex], &width, &height, &nrChannels, 0);
            if (data)
            {
                glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
                glGenerateMipmap(GL_TEXTURE_2D);
            }
            else
            {
                std::cout << "Failed to load texture" << std::endl;
            }
            stbi_image_free(data);


            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
            
            glUniform1i(glGetUniformLocation(ourShader.ID, "OurTexture"), 0);

            glBindVertexArray(VAO);
            glBindBuffer(GL_ARRAY_BUFFER, VBO);
            model = glm::mat4(1.0f);

            model[3][1] = ypos;
            ourShader.setMat4("model", model);
            midpoint = glm::vec4(0, 0, 0, 1);
            ourShader.setVec4("midpoint", midpoint);
            glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, (void *)0);

            model[3][1] = 0.0f;

            if (bgshift >= -2) bgshift -= levels[level][2];
            else bgshift = 0.0;

            model[3][0] = bgshift;
            ourShader.setMat4("model", model);
        }

        glfwSwapBuffers(window);

        glfwPollEvents();
    }

    glfwDestroyWindow(window);    
    glfwTerminate();
}

// The zappers/coins of next level overlap with those of prev level, FIX THAT