// Include standard headers
#include <cstdio>
#include <cstdlib>
#include <vector>

// Include GLEW
#include <GL/glew.h>

// Include GLFW
#include <glfw3.h>
GLFWwindow* window;

// Include GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
using namespace glm;

#include <common/shader.hpp>
#include <common/texture.hpp>
#include <common/controls.hpp>

#include "hw2/objects/buffers.h"

int initializeContext() {
    // Initialise GLFW
    if( !glfwInit() )
    {
        fprintf( stderr, "Failed to initialize GLFW\n" );
        getchar();
        return -1;
    }

    glfwWindowHint(GLFW_SAMPLES, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);


    // Open a window and create its OpenGL context
    window = glfwCreateWindow( 1024, 768, "Tutorial 0 - Keyboard and Mouse", NULL, NULL);
    if( window == NULL ){
        fprintf( stderr, "Failed to open GLFW window.\n" );
        getchar();
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // Initialize GLEW
    if (glewInit() != GLEW_OK) {
        fprintf(stderr, "Failed to initialize GLEW\n");
        getchar();
        glfwTerminate();
        return -1;
    }

    // Ensure we can capture the escape key being pressed below
    glfwSetInputMode(window, GLFW_STICKY_KEYS, GL_TRUE);
    // Hide the mouse and enable unlimited mouvement
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // Set the mouse at the center of the screen
    glfwPollEvents();
    glfwSetCursorPos(window, 1024/2, 768/2);

    // Dark blue background
    glClearColor(0.0f, 0.0f, 0.4f, 0.0f);

    // Enable depth test
    glEnable(GL_DEPTH_TEST);
    // Accept fragment if it closer to the camera than the former one
    glDepthFunc(GL_LESS);

    // Cull triangles which normal is not towards the camera
    glEnable(GL_CULL_FACE);

    // Create and compile our GLSL program from the shaders

    return 0;
}

class Fireball{
public:
    constexpr static float speed = 0.5f;
    constexpr static float radius = 1.f;
    constexpr static float startDistance = 1.f;

    glm::vec3 position;
    glm::vec3 direction;

    Fireball() = default;
    Fireball(glm::vec3 position, glm::vec3 direction) : position(position), direction(direction){}
};

void draw(GLuint Texture, GLuint TextureID, GLuint VertexBuffer, GLuint UVBuffer) {
    // Bind our texture in Texture Unit 0
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, Texture);
    // Set our "myTextureSampler" sampler to use Texture Unit 0
    glUniform1i(TextureID, 0);

    // 1rst attribute buffer : vertices
    glEnableVertexAttribArray(0);
    glBindBuffer(GL_ARRAY_BUFFER, VertexBuffer);
    glVertexAttribPointer(
            0,                  // attribute. No particular reason for 0, but must match the layout in the shader.
            3,                  // size
            GL_FLOAT,           // type
            GL_FALSE,           // normalized?
            0,                  // stride
            (void*)0            // array buffer offset
    );

    // 2nd attribute buffer : UVs
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, UVBuffer);
    glVertexAttribPointer(
            1,                                // attribute. No particular reason for 1, but must match the layout in the shader.
            2,                                // size : U+V => 2
            GL_FLOAT,                         // type
            GL_FALSE,                         // normalized?
            0,                                // stride
            (void*)0                          // array buffer offset
    );

    // Draw the triangle !
    glDrawArrays(GL_TRIANGLES, 0, 100 * 3); // 12*3 indices starting at 0 -> 12 triangles
}

void calculate(GLuint objId, const glm::vec3& position, GLuint matrixID, glm::mat4& ModelMatrix, glm::mat4& MVP, glm::mat4& ProjectionMatrix, glm::mat4& ViewMatrix) {
    glUseProgram(objId);
    ModelMatrix = glm::mat4(0.7f);
    glm::vec3 myTranslationVector(position);

    ModelMatrix = glm::translate(glm::mat4(), myTranslationVector) * ModelMatrix;
    MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
    // Send our transformation to the currently bound shader,
    // in the "MVP" uniform
    glUniformMatrix4fv(matrixID, 1, GL_FALSE, &MVP[0][0]);
}

int getSign() {
    return (std::rand() % 2 - 1 > 0) ? 1 : -1;
}

int main( void )
{
	// Initialise GLFW

	initializeContext();
    // Initialise GLFW
	// Create and compile our GLSL program from the shaders
//	GLuint programID = LoadShaders( "TransformVertexShader.vertexshader", "TextureFragmentShader.fragmentshader" );
//	// Get a handle for our "MVP" uniform
//	GLuint MatrixID = glGetUniformLocation(programID, "MVP");


    GLuint fireballId = LoadShaders("TransformVertexShader.vertexshader", "TextureFragmentShader.fragmentshader");
    GLuint fireballMatrixID = glGetUniformLocation(fireballId, "MVP");

	// Load the texture
	GLuint Texture = loadDDS("uvtemplate.DDS");

	// Get a handle for our "myTextureSampler" uniform
	GLuint TextureID  = glGetUniformLocation(fireballId, "myTextureSampler");

	// Our vertices. Tree consecutive floats give a 3D vertex; Three consecutive vertices give a triangle.
	// A cube has 6 faces with 2 triangles each, so this makes 6*2=12 triangles, and 12*3 vertices

    GLuint targetId = LoadShaders( "SMALL.vertexshader", "ColorFragmentShader.fragmentshader" );
    GLuint targetMatrixID = glGetUniformLocation(targetId, "MVP");


	GLuint vertexbuffer;
	glGenBuffers(1, &vertexbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_vertex_buffer_data), g_vertex_buffer_data, GL_STATIC_DRAW);

	GLuint uvbuffer;
	glGenBuffers(1, &uvbuffer);
	glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
	glBufferData(GL_ARRAY_BUFFER, sizeof(g_uv_buffer_data), g_uv_buffer_data, GL_STATIC_DRAW);


    int mouseState = GLFW_RELEASE;
    constexpr auto targetRadius = 1.f;
    constexpr int minTargetDistance = 5;
    constexpr int maxTargetDistance = 30;

    float lastSpawnTime = 0.f;

    std::vector<Fireball> fireballs;
    std::vector<glm::vec3> targets;

    std::srand(unsigned(time(0)));

	do {

		// Clear the screen
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		// Compute the MVP matrix from keyboard and mouse input
		computeMatricesFromInputs();
		glm::mat4 ProjectionMatrix = getProjectionMatrix();
		glm::mat4 ViewMatrix = getViewMatrix();
		glm::mat4 ModelMatrix = glm::mat4(1.0);
		glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

		const auto currTime = glfwGetTime();

        ////////////////////////////////////////////////////////////////////////////
        ////////////////////////      Fireball        //////////////////////////////
        ////////////////////////////////////////////////////////////////////////////
        int newState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
        if (newState == GLFW_RELEASE && mouseState == GLFW_PRESS) {
            fireballs.push_back({getPosition() + getDirection() * float(Fireball::startDistance), getDirection()});
        }
        mouseState = newState;

        for (auto& fireball: fireballs) {
            calculate(fireballId, fireball.position, fireballMatrixID,ModelMatrix, MVP, ProjectionMatrix, ViewMatrix);
            draw(Texture, TextureID, vertexbuffer, uvbuffer);

            fireball.position += fireball.direction * float(Fireball::speed);
        }

        ////////////////////////////////////////////////////////////////////////////
        ////////////////////////      Targets         //////////////////////////////
        ////////////////////////////////////////////////////////////////////////////
        glUseProgram(targetId);

        if (targets.empty() || (currTime - lastSpawnTime > 1.f)) {
            targets.push_back({
                getPosition()[0] + getSign() * (std::rand() % (maxTargetDistance - minTargetDistance) + minTargetDistance),
                getPosition()[1] + getSign() * (std::rand() % (maxTargetDistance - minTargetDistance) + minTargetDistance),
                getPosition()[2] + getSign() * (std::rand() % (maxTargetDistance - minTargetDistance) + minTargetDistance)
            });
            lastSpawnTime = currTime;
        }

        for (const auto& target : targets) {
            calculate(targetId, target, targetMatrixID, ModelMatrix, MVP, ProjectionMatrix, ViewMatrix);
            draw(Texture, TextureID, vertexbuffer, uvbuffer);
        }

        ////////////////////////////////////////////////////////////////////////////
        ////////////////////////      Collider        //////////////////////////////
        ////////////////////////////////////////////////////////////////////////////
        for (size_t target_idx = 0; target_idx < targets.size(); ++target_idx) {
            for (size_t fireball_idx = 0; fireball_idx < fireballs.size(); ++fireball_idx) {
                if (glm::distance(targets[target_idx], fireballs[fireball_idx].position) < Fireball::radius + targetRadius) {
                    targets.erase(targets.begin() + target_idx);
                    fireballs.erase(fireballs.begin() + fireball_idx);
                }
            }
        }

        glDisableVertexAttribArray(0);
        glDisableVertexAttribArray(1);

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );

//	// Cleanup VBO and shader
	glDeleteBuffers(1, &vertexbuffer);
	glDeleteBuffers(1, &uvbuffer);
	glDeleteProgram(fireballId);
	glDeleteTextures(1, &TextureID);

	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}
