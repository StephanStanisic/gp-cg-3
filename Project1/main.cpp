#include <iostream>
#include <vector>

#include <GL/glew.h>
#include <GL/freeglut.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

#include "glsl.h"
#include "objloader.h"

#include "texture.h"

void CheckOpenGLError(const char* stmt, const char* fname, int line)
{
    GLenum err = glGetError();
    if (err != GL_NO_ERROR)
    {
        const char* gl_errors[10] = { "GL_INVALID_ENUM" ,"GL_INVALID_VALUE","GL_INVALID_OPERATION","GL_STACK_OVERFLOW","GL_STACK_UNDERFLOW","GL_OUT_OF_MEMORY","GL_INVALID_FRAMEBUFFER_OPERATION" };
        printf("OpenGL error %08x (%s), at %s:%i - for %s\n", err, gl_errors[err - 0x0500], fname, line, stmt);
        abort();
    }
}

#ifdef _DEBUG
#define GL_CHECK(stmt) do { \
            stmt; \
            CheckOpenGLError(#stmt, __FILE__, __LINE__); \
        } while (0)
#else
#define GL_CHECK(stmt) stmt
#endif


using namespace std;


//--------------------------------------------------------------------------------
// Consts
//--------------------------------------------------------------------------------

const int WIDTH = 800, HEIGHT = 600;

const char* fragshader_name = "fragmentshader.frag";
const char* vertexshader_name = "vertexshader.vert";

unsigned const int DELTA_TIME = 10;

constexpr auto NUMBER_OF_OBJECTS = 2;


//--------------------------------------------------------------------------------
// Variables
//--------------------------------------------------------------------------------

// ID's
GLuint program_id;
GLuint vao[NUMBER_OF_OBJECTS];
GLuint vbo_uvs[NUMBER_OF_OBJECTS];
GLuint vbo_normals[NUMBER_OF_OBJECTS];
GLuint texture_id[NUMBER_OF_OBJECTS];

// Uniform ID's
GLuint uniform_mv;
GLuint uniform_proj;
GLuint uniform_light_pos;
GLuint uniform_material_ambient;
GLuint uniform_material_diffuse;
GLuint uniform_specular;
GLuint uniform_material_power;


GLuint position_id;
GLuint vbo_vertices[NUMBER_OF_OBJECTS];
glm::vec3 light_position,
    ambient_color[NUMBER_OF_OBJECTS],
    diffuse_color[NUMBER_OF_OBJECTS];

// Matrices
glm::mat4 model[NUMBER_OF_OBJECTS], view, projection;
glm::mat4 mv[NUMBER_OF_OBJECTS];
glm::vec3 specular[NUMBER_OF_OBJECTS];
float power[NUMBER_OF_OBJECTS];

//--------------------------------------------------------------------------------
// Mesh variables
//--------------------------------------------------------------------------------

vector<glm::vec3> normals[NUMBER_OF_OBJECTS];
vector<glm::vec3> vertices[NUMBER_OF_OBJECTS];
vector<glm::vec2> uvs[NUMBER_OF_OBJECTS];



//--------------------------------------------------------------------------------
// Keyboard handling
//--------------------------------------------------------------------------------

void keyboardHandler(unsigned char key, int a, int b)
{
    if (key == 27)
        glutExit();
}


//--------------------------------------------------------------------------------
// Rendering
//--------------------------------------------------------------------------------

void Render()
{
    GL_CHECK(glClearColor(0.0, 0.0, 0.0, 1.0));
    GL_CHECK(glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT));

    // Attach to program_id
    GL_CHECK(glUseProgram(program_id));

    for (int i = 0; i < NUMBER_OF_OBJECTS; i++) {
        // Send mvp
        GL_CHECK(glUseProgram(program_id));
        GL_CHECK(glUniformMatrix4fv(uniform_mv, 1, GL_FALSE, glm::value_ptr(mv[i])));
        GL_CHECK(glUniformMatrix4fv(uniform_proj, 1, GL_FALSE, glm::value_ptr(projection)));
        GL_CHECK(glUniform3fv(uniform_light_pos, 1, glm::value_ptr(light_position)));
        GL_CHECK(glUniform3fv(uniform_material_ambient, 1, glm::value_ptr(ambient_color[i])));
        GL_CHECK(glUniform3fv(uniform_material_diffuse, 1, glm::value_ptr(diffuse_color[i])));
        GL_CHECK(glUniform3fv(uniform_specular, 1, glm::value_ptr(specular[i])));
        GL_CHECK(glUniform1f(uniform_material_power, power[i]));

        // Do transformation
        model[i] = glm::rotate(model[i], 0.01f, glm::vec3(0.0f, 1.0f, 0.0f));
        mv[i] = view * model[i];

        // Send mvp
        GL_CHECK(glUniformMatrix4fv(uniform_mv, 1, GL_FALSE, glm::value_ptr(mv[i])));

        GL_CHECK(glBindVertexArray(vao[i]));
        GL_CHECK(glBindTexture(GL_TEXTURE_2D, texture_id[i]));
        GL_CHECK(glDrawArrays(GL_TRIANGLES, 0, vertices[i].size()));
        GL_CHECK(glBindVertexArray(0));
    }

    GL_CHECK(glutSwapBuffers());
}


//------------------------------------------------------------
// void Render(int n)
// Render method that is called by the timer function
//------------------------------------------------------------

void Render(int n)
{
    Render();
    GL_CHECK(glutTimerFunc(DELTA_TIME, Render, 0));
}


//------------------------------------------------------------
// void InitGlutGlew(int argc, char **argv)
// Initializes Glut and Glew
//------------------------------------------------------------

void InitGlutGlew(int argc, char** argv)
{
    GL_CHECK(glutInit(&argc, argv));
    GL_CHECK(glutInitDisplayMode(GLUT_DOUBLE | GLUT_RGBA | GLUT_DEPTH));
    GL_CHECK(glutInitWindowSize(WIDTH, HEIGHT));
    GL_CHECK(glutCreateWindow("Hello OpenGL"));
    GL_CHECK(glutDisplayFunc(Render));
    GL_CHECK(glutKeyboardFunc(keyboardHandler));
    GL_CHECK(glutTimerFunc(DELTA_TIME, Render, 0));

    GL_CHECK(glewInit());
}


//------------------------------------------------------------
// void InitShaders()
// Initializes the fragmentshader and vertexshader
//------------------------------------------------------------

void InitShaders()
{
    char* vertexshader;
    GL_CHECK(vertexshader = glsl::readFile(vertexshader_name));
    GLuint vsh_id;
    GL_CHECK(vsh_id = glsl::makeVertexShader(vertexshader));

    char* fragshader;
    GL_CHECK(fragshader = glsl::readFile(fragshader_name));
    GLuint fsh_id;
    GL_CHECK(fsh_id = glsl::makeFragmentShader(fragshader));

    GL_CHECK(program_id = glsl::makeShaderProgram(vsh_id, fsh_id));
}


//------------------------------------------------------------
// void InitMatrices()
//------------------------------------------------------------

void InitMatrices()
{
    view = glm::lookAt(
        glm::vec3(2.0, 2.0, 7.0),  // eye
        glm::vec3(0.0, 0.0, 0.0),  // center
        glm::vec3(0.0, 1.0, 0.0));  // up
    projection = glm::perspective(
        glm::radians(45.0f),
        1.0f * WIDTH / HEIGHT, 0.1f,
        20.0f);
}


//------------------------------------------------------------
// void InitBuffers()
// Allocates and fills buffers
//------------------------------------------------------------

void InitBuffers()
{

    GL_CHECK(position_id = glGetAttribLocation(program_id, "position"));

    for (int i = 0; i < NUMBER_OF_OBJECTS; i++) {

        // vbo for vertices
        GL_CHECK(glGenBuffers(1, &(vbo_vertices[i])));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices[i]));
        GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
            vertices[i].size() * sizeof(glm::vec3), &vertices[i][0],
            GL_STATIC_DRAW));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

        // vbo for normals
        GL_CHECK(glGenBuffers(1, &(vbo_normals[i])));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_normals[i]));
        GL_CHECK(glBufferData(GL_ARRAY_BUFFER,
            normals[i].size() * sizeof(glm::vec3),
            &normals[i][0], GL_STATIC_DRAW));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

        GL_CHECK(glGenBuffers(1, &(vbo_uvs[i])));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_uvs[i]));
        GL_CHECK(glBufferData(GL_ARRAY_BUFFER, uvs[i].size() * sizeof(glm::vec2),
            &uvs[i][0], GL_STATIC_DRAW));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));


        // Get vertex attributes
        GLuint normal_id;
        GL_CHECK(normal_id = glGetAttribLocation(program_id, "normal"));
        GLuint uv_id;
        GL_CHECK(uv_id = glGetAttribLocation(program_id, "uv"));

        // Allocate memory for vao
        GL_CHECK(glGenVertexArrays(1, &(vao[i])));

        // Bind to vao
        GL_CHECK(glBindVertexArray(vao[i]));

        // Bind vertices to vao
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_vertices[i]));
        GL_CHECK(glVertexAttribPointer(position_id, 3, GL_FLOAT, GL_FALSE, 0, 0));
        GL_CHECK(glEnableVertexAttribArray(position_id));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

        // Bind normals to vao
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_normals[i]));
        GL_CHECK(glVertexAttribPointer(normal_id, 3, GL_FLOAT, GL_FALSE, 0, 0));
        GL_CHECK(glEnableVertexAttribArray(normal_id));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

        // uv maps
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, vbo_uvs[i]));
        GL_CHECK(glVertexAttribPointer(uv_id, 2, GL_FLOAT, GL_FALSE, 0, 0));
        GL_CHECK(glEnableVertexAttribArray(uv_id));
        GL_CHECK(glBindBuffer(GL_ARRAY_BUFFER, 0));

        // Stop bind to vao
        GL_CHECK(glBindVertexArray(0));
    }

    // Make uniform vars
    GL_CHECK(uniform_mv = glGetUniformLocation(program_id, "mv"));
    GL_CHECK(uniform_proj = glGetUniformLocation(program_id, "projection"));
    GL_CHECK(uniform_light_pos = glGetUniformLocation(program_id, "light_pos"));
    GL_CHECK(uniform_material_ambient = glGetUniformLocation(program_id, "mat_ambient"));
    GL_CHECK(uniform_material_diffuse = glGetUniformLocation(program_id, "mat_diffuse"));
    GL_CHECK(uniform_specular = glGetUniformLocation(program_id, "mat_specular"));
    GL_CHECK(uniform_material_power = glGetUniformLocation(program_id, "mat_power"));
}

void InitObjects() {
    bool res = loadOBJ("teapot.obj", vertices[0], uvs[0], normals[0]);
    texture_id[0] = loadBMP("uvtemplate.bmp"); // Heeft GLUT/GLEW nodig!

    bool res2 = loadOBJ("torus.obj", vertices[1], uvs[1], normals[1]);
    texture_id[1] = loadBMP("Yellobrk.bmp"); // Heeft GLUT/GLEW nodig!
}

void InitMaterials() {
    light_position = glm::vec3(4, 4, 4);

    ambient_color[0] = glm::vec3(0.2, 0.2, 0.1);
    diffuse_color[0] = glm::vec3(0.5, 0.5, 0.3);
    specular[0] = glm::vec3(0.7, 0.7, 0.7);
    power[0] = 1024;

    ambient_color[1] = glm::vec3(0.2, 0.2, 0.1);
    diffuse_color[1] = glm::vec3(0.5, 0.5, 0.3);
    specular[1] = glm::vec3(0.7, 0.7, 0.7);
    power[1] = 1024;
}


int main(int argc, char** argv)
{
    InitGlutGlew(argc, argv);
    InitShaders();
    InitMatrices();
    InitObjects();
    InitMaterials();
    InitBuffers();

    GL_CHECK(glEnable(GL_DEPTH_TEST));
    GL_CHECK(glDisable(GL_CULL_FACE));

    // Hide console window
    HWND hWnd = GetConsoleWindow();
    ShowWindow(hWnd, SW_SHOW);

    // Main loop
    glutMainLoop();

    return 0;
}
