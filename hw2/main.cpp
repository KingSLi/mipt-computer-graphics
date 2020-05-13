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
#include <common/objloader.hpp>
#include <iostream>

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
    glClearColor(0.4f, 0.5f, 1.f, 0.0f);

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
    constexpr static float speed = 0.75f;
    constexpr static float radius = 0.7f;
    constexpr static float startDistance = 1.f;

    glm::vec3 position;
    glm::vec3 direction;

    Fireball() = default;
    Fireball(glm::vec3 position, glm::vec3 direction) : position(position), direction(direction){}
};

struct Object{
    GLuint Id;
    GLuint MatrixID;
    GLuint VertexPositionModelspaceID;
    GLuint VertexUVID;
    GLuint Texture;
    GLuint TextureID;

    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> uvs;
    std::vector<glm::vec3> normals;

    GLuint vertexbuffer;
    GLuint uvbuffer;

    explicit Object(const char* imagePath, bool isDDS = true) {
        Id = LoadShaders("TransformVertexShader.vertexshader", "TextureFragmentShader.fragmentshader");
        MatrixID = glGetUniformLocation(Id, "MVP");
        VertexPositionModelspaceID = glGetAttribLocation(Id, "vertexPosition_modelspace");
        VertexUVID = glGetAttribLocation(Id, "vertexUV"); //!!!
        // Load the texture
        Texture = isDDS ? loadDDS(imagePath) : loadBMP_custom(imagePath);
        // Get a handle for our "myTextureSampler" uniform
        TextureID  = glGetUniformLocation(Id, "myTextureSampler");
    }

    void load(const char * pathToObj) {
        loadOBJ(pathToObj, vertices, uvs, normals);

        glGenBuffers(1, &vertexbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(glm::vec3), &vertices[0], GL_STATIC_DRAW);

        glGenBuffers(1, &uvbuffer);
        glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
        glBufferData(GL_ARRAY_BUFFER, uvs.size() * sizeof(glm::vec2), &uvs[0], GL_STATIC_DRAW);
    }

    size_t getVerticesSize() const {
        return vertices.size();
    }

    void draw() {
        // Bind our texture in Texture Unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, Texture);
        // Set our "myTextureSampler" sampler to use Texture Unit 0
        glUniform1i(TextureID, 0);

        // 1rst attribute buffer : vertices
        glEnableVertexAttribArray(VertexPositionModelspaceID);
        glBindBuffer(GL_ARRAY_BUFFER, vertexbuffer);
        glVertexAttribPointer(
                VertexPositionModelspaceID,                  // attribute. No particular reason for 0, but must match the layout in the shader.
                3,                  // size
                GL_FLOAT,           // type
                GL_FALSE,           // normalized?
                0,                  // stride
                (void*)0            // array buffer offset
        );

        // 2nd attribute buffer : UVs
        glEnableVertexAttribArray(VertexUVID);
        glBindBuffer(GL_ARRAY_BUFFER, uvbuffer);
        glVertexAttribPointer(
                VertexUVID,                                // attribute. No particular reason for 1, but must match the layout in the shader.
                2,                                // size : U+V => 2
                GL_FLOAT,                         // type
                GL_FALSE,                         // normalized?
                0,                                // stride
                (void*)0                          // array buffer offset
        );

        // Draw the triangle !
        glDrawArrays(GL_TRIANGLES, 0, getVerticesSize()); // 12*3 indices starting at 0 -> 12 triangles

        glDisableVertexAttribArray(VertexPositionModelspaceID);
        glDisableVertexAttribArray(VertexUVID);
    }


    ~Object() {
        // Cleanup VBO and shader
        glDeleteProgram(Id);
        glDeleteTextures(1, &TextureID);
        glDeleteBuffers(1, &vertexbuffer);
        glDeleteBuffers(1, &uvbuffer);
    }
};

void calculatePosition(GLuint objId, const glm::vec3& position, GLuint matrixID, glm::mat4& ModelMatrix, glm::mat4& MVP, glm::mat4& ProjectionMatrix, glm::mat4& ViewMatrix, bool isCrossHair = false) {
    glUseProgram(objId);
    ModelMatrix = glm::mat4(0.7f);
    glm::vec3 myTranslationVector(position);
    if (isCrossHair) {
        MVP = ModelMatrix;
    }
    else {
        ModelMatrix = glm::translate(glm::mat4(), myTranslationVector) * ModelMatrix;
        MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
    }
    // Send our transformation to the currently bound shader,
    // in the "MVP" uniform
    glUniformMatrix4fv(matrixID, 1, GL_FALSE, &MVP[0][0]);
}

int getSign() {
    return ((std::rand() % 2) == 0) ? 1 : -1;
}

int main( void )
{
	// Initialise GLFW
	initializeContext();

	Object fireballObj("images/virus.bmp", false);
    Object targetEarthObj("images/earthmap.bmp", false);
    Object targetMarsObj("images/Mars.bmp", false);
    Object crossHairObj("images/new_target.DDS");

    fireballObj.load("objects/fireball.obj");
    targetEarthObj.load("objects/target.obj");
    targetMarsObj.load("objects/target.obj");
    crossHairObj.load("objects/crosshair.obj");

    constexpr float targetRadius = 3.f;
    constexpr int minTargetDistance = 5;
    constexpr int maxTargetDistance = 30;

    int mouseState = GLFW_RELEASE;

    float lastSpawnTime = 0.f;

    std::vector<Fireball> fireballs;
    std::vector<std::pair<glm::vec3, size_t>> targets;
    size_t targets_number = 0;

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
        glUseProgram(fireballObj.Id);
        int newState = glfwGetMouseButton(window, GLFW_MOUSE_BUTTON_LEFT);
        if (newState == GLFW_RELEASE && mouseState == GLFW_PRESS) {
            fireballs.push_back({getPosition() + getDirection() * float(Fireball::startDistance), getDirection()});
        }
        mouseState = newState;

        for (auto& fireball: fireballs) {
            calculatePosition(fireballObj.Id, fireball.position, fireballObj.MatrixID, ModelMatrix, MVP,
                              ProjectionMatrix, ViewMatrix);
            fireballObj.draw();
            fireball.position += fireball.direction * float(Fireball::speed);
        }

        ////////////////////////////////////////////////////////////////////////////
        ////////////////////////      Targets         //////////////////////////////
        ////////////////////////////////////////////////////////////////////////////
        if (targets.empty() || (currTime - lastSpawnTime > 2.f)) {
            targets.push_back({{
                getSign() * (std::rand() % (maxTargetDistance - minTargetDistance) + minTargetDistance),
                getSign() * (std::rand() % (maxTargetDistance - minTargetDistance) + minTargetDistance),
                getSign() * (std::rand() % (maxTargetDistance - minTargetDistance) + minTargetDistance)
            }, targets_number++});
            lastSpawnTime = currTime;
        }

        for (const auto& target : targets) {
            auto& obj = (target.second % 2 == 0) ? targetMarsObj : targetEarthObj;
            glUseProgram(obj.Id);
            calculatePosition(obj.Id, target.first, obj.MatrixID, ModelMatrix, MVP, ProjectionMatrix, ViewMatrix);
            obj.draw();
        }

        ////////////////////////////////////////////////////////////////////////////
        ////////////////////////      CrossHair       //////////////////////////////
        ////////////////////////////////////////////////////////////////////////////

        glUseProgram(crossHairObj.Id);
        calculatePosition(crossHairObj.Id, getPosition(), crossHairObj.MatrixID, ModelMatrix, MVP,
                          ProjectionMatrix, ViewMatrix, true);
        crossHairObj.draw();

        ////////////////////////////////////////////////////////////////////////////
        ////////////////////////      Collider        //////////////////////////////
        ////////////////////////////////////////////////////////////////////////////
        for (size_t target_idx = 0; target_idx < targets.size(); ++target_idx) {
            for (size_t fireball_idx = 0; fireball_idx < fireballs.size(); ++fireball_idx) {
                if (glm::distance(targets[target_idx].first, fireballs[fireball_idx].position) < Fireball::radius + targetRadius) {
                    targets.erase(targets.begin() + target_idx);
                    fireballs.erase(fireballs.begin() + fireball_idx);
                }
                if (glm::distance(fireballs[fireball_idx].position, getPosition()) > 2 * maxTargetDistance) {
                    fireballs.erase(fireballs.begin() + fireball_idx);
                }
            }
            if (glm::distance(targets[target_idx].first, getPosition()) < targetRadius) {
                std::cout << "You lose!" << std::endl;
                return 0;
            }
        }

		// Swap buffers
		glfwSwapBuffers(window);
		glfwPollEvents();

	} // Check if the ESC key was pressed or the window was closed
	while( glfwGetKey(window, GLFW_KEY_ESCAPE ) != GLFW_PRESS &&
		   glfwWindowShouldClose(window) == 0 );
	// Close OpenGL window and terminate GLFW
	glfwTerminate();

	return 0;
}

