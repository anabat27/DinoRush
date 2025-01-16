#include "Graphics/window.h"
#include "Camera/camera.h"
#include "Shaders/shader.h"
#include "Model Loading/mesh.h"
#include "Model Loading/texture.h"
#include "Model Loading/meshLoaderObj.h"
#include <../glm/gtc/matrix_transform.hpp>
#include <vector>
#include <iostream>

void processKeyboardInput();
void handleMouseInput();

struct Task {
    std::string story;              // Story description
    std::string objective;          // Task description
    glm::vec3 interactionPoint;     // Position of the task
    bool completed;                 // Task status
};

std::vector<Task> tasks = {
    {
    "After the crash, you stumble upon a survival pack near the wreckage. It contains essential supplies for your survival.",
    "Find the survival pack near the wreckage of the plane.",
    glm::vec3(205.0f, -20.0f, 245.0f), // Survival pack position near the wreckage
    false
},
{
    "You find a letter inside the survival pack that mentions a ghillie suit hidden near a rock formation. This will help you avoid detection by the T-Rex.",
    "Find the ghillie suit near the rock formation.",

},
{
    "Inside the ghillie suit's pocket, you find a letter describing a hidden map that leads to a rescue beacon. The map has been lost a long time ago. Go find it!.",
    "Find the hidden map.",

},

{
    "The map you found leads you to a beacon that needs to be activated to call for help. Go to the beacon and turn it on.",
    "Find and activate the beacon to call for help.",
    glm::vec3(150.0f, -20.0f, 300.0f), // Beacon position
    false
},

{
    "With the beacon activated, help is on the way! Reach the helicopter to escape the island safely.",
    "Escape to safety by reaching the helicopter.",
    glm::vec3(500.0f, -20.0f, 400.0f), // Helicopter position
    false
}

};

// ------------------------------------------------
// Struct for scene objects (trees, rocks)
struct SceneObject {
    glm::vec3 position;
    float radius;
    SceneObject() : position(0.0f), radius(0.0f) {}
};

// Sphere-sphere collision checker
bool checkCollisionSphere(const glm::vec3& center1, float radius1,
    const glm::vec3& center2, float radius2)
{
    float dist = glm::distance(center1, center2);
    return dist < (radius1 + radius2);
}

// ------------------------------------------------
// GLOBALS for collision data
std::vector<SceneObject> sceneObjects;  // Trees + rocks stored here
float cameraRadius = 2.0f;             // The radius of the camera
glm::vec3 wallCenter = glm::vec3(-40.0f, -10.0f, 40.0f);
float wallRadius = 360.0f;             // Wall that encloses the map

// GLOBALS for timing / window / camera
float deltaTime = 0.0f;	// time between current frame and last frame
float lastFrame = 0.0f;

Window window("Game Engine", 1600, 900);
Camera camera;

// Light info
glm::vec3 lightColor = glm::vec3(1.0f, 1.0f, 1.0f);
glm::vec3 lightPos = glm::vec3(30.0f, 180.0f, 50.0f);

// ------------------------------------------------
// Mouse callback
void cursor_position_callback(GLFWwindow* window, double xpos, double ypos) {
    static bool firstMouse = true;
    static float lastX = 400.0f, lastY = 400.0f; // screen center

    if (firstMouse) {
        lastX = (float)xpos;
        lastY = (float)ypos;
        firstMouse = false;
    }

    float offsetX = (float)xpos - lastX;
    float offsetY = lastY - (float)ypos; // Reversed since y-coordinates go bottom-to-top
    lastX = (float)xpos;
    lastY = (float)ypos;

    float sensitivity = 0.1f;
    offsetX *= sensitivity;
    offsetY *= sensitivity;

    camera.rotateOy(offsetX); // yaw
    camera.rotateOx(offsetY); // pitch
}

//  Day-night cycle
float dayNightTime = 0.0f;   // increments each frame
float cycleDuration = 60.0f; // 60 seconds for a full day-night cycle

// Meteors
struct Meteor {
    glm::vec3 position;
    glm::vec3 velocity;
    float     scale;
    bool      active;
};

std::vector<Meteor> meteors;

// Return random float in [low, high]
float randBetween(float low, float high) {
    float r = (float)rand() / (float)RAND_MAX;
    return low + r * (high - low);
}

void spawnMeteor() {
    Meteor m;
    // Position high above the map
    float x = randBetween(-300.0f, 300.0f);
    float z = randBetween(-300.0f, 300.0f);
    float y = 200.0f;  // spawn above the sky

    m.position = glm::vec3(x, y, z);

    // Velocity
    float vx = 0.0f;
    float vy = randBetween(-50.0f, -100.0f);
    float vz = 0.0f;
    m.velocity = glm::vec3(vx, vy, vz);

    m.scale = randBetween(1.0f, 5.0f); // random size
    m.active = true;

    meteors.push_back(m);
}

void updateMeteors(float dt) {
    for (auto& m : meteors) {
        if (!m.active) continue;

        m.position += m.velocity * dt;

        if (m.position.y < -20.0f) {
            m.active = false;
        }
    }
}

void drawMeteors(Shader& meteorShader, Mesh& meteorMesh,
    const glm::mat4& ProjectionMatrix,
    const glm::mat4& ViewMatrix,
    GLuint MatrixID, GLuint ModelMatrixID)
{
    for (auto& m : meteors) {
        if (!m.active) continue;

        glm::mat4 Model = glm::mat4(1.0f);
        Model = glm::translate(Model, m.position);
        Model = glm::scale(Model, glm::vec3(m.scale));
        glm::mat4 MVP = ProjectionMatrix * ViewMatrix * Model;

        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &Model[0][0]);

        meteorMesh.draw(meteorShader);
    }
}

void drawSkySphere(Mesh& sphereMesh,
    Shader& shader,
    const glm::vec3& cameraPos,
    const glm::mat4& projection)
{
    glm::mat4 noRotView = glm::translate(glm::mat4(1.0f), -cameraPos);

    shader.use();
    GLint viewLoc = glGetUniformLocation(shader.getId(), "noRotView");
    GLint projLoc = glGetUniformLocation(shader.getId(), "projection");
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, &noRotView[0][0]);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, &projection[0][0]);

    // Build a big model matrix so it encloses your map
    float bigRadius = 5000.0f; // or whichever is large enough
    glm::mat4 model = glm::scale(glm::mat4(1.0f), glm::vec3(bigRadius));
    GLint modelLoc = glGetUniformLocation(shader.getId(), "model");
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, &model[0][0]);

    glDepthFunc(GL_LEQUAL); // ensures it's drawn behind everything

    // draw
    sphereMesh.draw(shader);

    // restore
    glDepthFunc(GL_LESS);
}

// ----------------------------------------------------
void displayCurrentTask() {
    for (const auto& task : tasks) {
        if (!task.completed) {
            std::cout << "Story: " << task.story << std::endl;
            std::cout << "Objective: " << task.objective << std::endl;
            break; // Only display the first incomplete task
        }
    }
}

bool isPlayerNearBackpack(const glm::vec3& playerPosition, const glm::vec3& backpackPosition, float range) {
    return glm::distance(playerPosition, backpackPosition) <= range;
}

bool isPlayerNearGhillieSuit(const glm::vec3& playerPosition, const glm::vec3& ghillieSuitPosition, float range) {
    return glm::distance(playerPosition, ghillieSuitPosition) <= range;
}

bool isPlayerNearHiddenMap(const glm::vec3& playerPosition, const glm::vec3& hiddenMapPosition, float range) {
    return glm::distance(playerPosition, hiddenMapPosition) <= range;
}

bool isPlayerNearKey(const glm::vec3& playerPosition, const glm::vec3& keyPosition, float range) {
    return glm::distance(playerPosition, keyPosition) <= range;
}

bool isPlayerNearBeacon(const glm::vec3& playerPosition, const glm::vec3& beaconPosition, float range) {
    return glm::distance(playerPosition, beaconPosition) <= range;
}

// MAIN
int main()
{
    glClearColor(0.2f, 0.8f, 1.0f, 1.0f);

    camera.setCameraPosition(glm::vec3(0.0f, -20.0f + 14.0f, 0.0f)); // start pos

    // Build and compile shader programs
    Shader shader("Shaders/vertex_shader.glsl", "Shaders/fragment_shader.glsl");
    Shader sunShader("Shaders/sun_vertex_shader.glsl", "Shaders/sun_fragment_shader.glsl");
    Shader meteorShader("Shaders/meteor_vertex_shader.glsl", "Shaders/meteor_fragment_shader.glsl");

    // Load textures
    GLuint tex = loadBMP("Resources/Textures/wood.bmp");
    GLuint tex2 = loadBMP("Resources/Textures/grass.bmp");
    GLuint tex3 = loadBMP("Resources/Textures/orange.bmp");
    GLuint tex4 = loadBMP("Resources/Textures/rockk.bmp");
    GLuint leavesTexture1 = loadBMP("Resources/Textures/Leaves_2_Cartoon.bmp");
    GLuint leavesTexture2 = loadBMP("Resources/Textures/Leaves_2_Cartoon_2.bmp");
    GLuint leavesTexture3 = loadBMP("Resources/Textures/Leaves1.bmp");
    GLuint leavesTexture4 = loadBMP("Resources/Textures/Leaves2.bmp");
    GLuint trunkTexture1 = loadBMP("Resources/Textures/Trunck.bmp");
    GLuint trunkTexture2 = loadBMP("Resources/Textures/Trunk_4_Cartoon.bmp");
    GLuint dinoTexture = loadBMP("Resources/Textures/Leaves2.bmp");
    GLuint meteorTex = loadBMP("Resources/Textures/orange.bmp");
    GLuint skySphereTex = loadBMP("Resources/Skybox/mysky.bmp");
    GLuint alternateSkyTex = loadBMP("Resources/Skybox/front.bmp");
    GLuint helicopterTex = loadBMP("Resources/Textures/helicopter.bmp");
    
    glfwSetCursorPosCallback(window.getWindow(), cursor_position_callback);
    glfwSetInputMode(window.getWindow(), GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    glEnable(GL_DEPTH_TEST);

    // Mesh loading
    std::vector<Vertex> vert;
    vert.push_back(Vertex());
    vert[0].pos = glm::vec3(10.5f, 10.5f, 0.0f);
    vert[0].textureCoords = glm::vec2(1.0f, 1.0f);

    vert.push_back(Vertex());
    vert[1].pos = glm::vec3(10.5f, -10.5f, 0.0f);
    vert[1].textureCoords = glm::vec2(1.0f, 0.0f);

    vert.push_back(Vertex());
    vert[2].pos = glm::vec3(-10.5f, -10.5f, 0.0f);
    vert[2].textureCoords = glm::vec2(0.0f, 0.0f);

    vert.push_back(Vertex());
    vert[3].pos = glm::vec3(-10.5f, 10.5f, 0.0f);
    vert[3].textureCoords = glm::vec2(0.0f, 1.0f);

    vert[0].normals = glm::normalize(glm::cross(vert[1].pos - vert[0].pos, vert[3].pos - vert[0].pos));
    vert[1].normals = glm::normalize(glm::cross(vert[2].pos - vert[1].pos, vert[0].pos - vert[1].pos));
    vert[2].normals = glm::normalize(glm::cross(vert[3].pos - vert[2].pos, vert[1].pos - vert[2].pos));
    vert[3].normals = glm::normalize(glm::cross(vert[0].pos - vert[3].pos, vert[2].pos - vert[3].pos));

    std::vector<int> ind = { 0, 1, 3, 1, 2, 3 };

    std::vector<Texture> textures;
    textures.push_back(Texture());
    textures[0].id = tex;
    textures[0].type = "texture_diffuse";

    std::vector<Texture> textures2_;
    textures2_.push_back(Texture());
    textures2_[0].id = helicopterTex;
    textures2_[0].type = "texture_diffuse";

    std::vector<Texture> textures3_;
    textures3_.push_back(Texture());
    textures3_[0].id = tex2;
    textures3_[0].type = "texture_diffuse";

    Mesh mesh(vert, ind, textures3_);

    // ------------------------------------------------
    // Load OBJ models
    MeshLoaderObj loader;
    Mesh sun = loader.loadObj("Resources/Models/sphere.obj");
    Mesh box = loader.loadObj("Resources/Models/cube.obj", textures);
    Mesh plane = loader.loadObj("Resources/Models/plane.obj", textures3_);
    Mesh tree_trunk = loader.loadObj("Resources/Models/tree_trunk.obj");
    Mesh tree_crown = loader.loadObj("Resources/Models/tree_crown.obj");
    Mesh walls = loader.loadObj("Resources/Models/cubewall.obj");
    Mesh rock = loader.loadObj("Resources/Models/planerock.obj");
    Mesh dino = loader.loadObj("Resources/Models/dino.obj");
    Mesh meteorMesh = loader.loadObj("Resources/Models/meteor.obj");
    Mesh skySphere = loader.loadObj("Resources/Models/sphere_inward.obj");
    Mesh backpack = loader.loadObj("Resources/Models/backpack.obj");
    Mesh beacon = loader.loadObj("Resources/Models/beacon.obj", textures);
    Mesh ghillieSuitMesh = loader.loadObj("Resources/Models/uniform1.obj");
    Mesh hiddenmap = loader.loadObj("Resources/Models/box.obj");
    Mesh key = loader.loadObj("Resources/Models/Key9.obj", textures2_);
    Mesh helicopter = loader.loadObj("Resources/Models/Lowpoly_Helicopter.obj", textures2_);


    std::vector<Texture> skySphereTextures;
    skySphereTextures.push_back(Texture{ skySphereTex, "texture_diffuse" });
    skySphere.setTextures(skySphereTextures);

    // Textures for tree
    std::vector<Texture> trunkTextures;
    trunkTextures.push_back(Texture{ trunkTexture1, "texture_diffuse" });
    tree_trunk.setTextures(trunkTextures);

    std::vector<Texture> crownTextures;
    crownTextures.push_back(Texture{ leavesTexture1, "texture_diffuse" });
    tree_crown.setTextures(crownTextures);

    std::vector<Texture> meteorTextures;
    meteorTextures.push_back(Texture{ meteorTex, "texture_diffuse" });
    meteorMesh.setTextures(meteorTextures);

    // Dino start
    glm::vec3 dinoPosition = glm::vec3(200.0f, -20.0f, 200.0f);
    glm::mat4 ModelMatrix = glm::mat4(1.0f);

    // Define positions for 20 trees
    glm::vec3 treePositions[] = {
        glm::vec3(95.0f, -20.0f, 250.0f),
        glm::vec3(-80.0f, -20.0f,-100.0f),
        glm::vec3(140.0f, -20.0f, 320.0f),
        glm::vec3(-320.0f, -20.0f,-180.0f),
        glm::vec3(-140.0f, -20.0f,-240.0f),
        glm::vec3(220.0f, -20.0f,  70.0f),
        glm::vec3(170.0f, -20.0f,-120.0f),
        glm::vec3(40.0f, -20.0f,  95.0f),
        glm::vec3(-220.0f, -20.0f,-110.0f),
        glm::vec3(200.0f, -20.0f, 240.0f),
        glm::vec3(-50.0f, -20.0f, 260.0f),
        glm::vec3(10.0f, -20.0f, 270.0f),
        glm::vec3(-100.0f, -20.0f, -60.0f),
        glm::vec3(60.0f, -20.0f, 150.0f),
        glm::vec3(-60.0f, -20.0f,  50.0f),
        glm::vec3(30.0f, -20.0f, -30.0f),
        glm::vec3(200.0f, -20.0f,-200.0f),
        glm::vec3(-200.0f, -20.0f, 200.0f),
        glm::vec3(-250.0f, -20.0f,  50.0f),
        glm::vec3(250.0f, -20.0f, -50.0f),
    };

    // Define positions for 12 rocks
    glm::vec3 rockPositions[] = {
        glm::vec3(60.0f, -20.0f, 235.0f),
        glm::vec3(-70.0f, -20.0f, -85.0f),
        glm::vec3(255.0f, -20.0f,-315.0f),
        glm::vec3(-285.0f, -20.0f,-170.0f),
        glm::vec3(-110.0f, -20.0f,-230.0f),
        glm::vec3(-110.0f, -20.0f, 140.0f),
        glm::vec3(235.0f, -20.0f,-145.0f),
        glm::vec3(75.0f, -20.0f,  85.0f),
        glm::vec3(-310.0f, -20.0f,-100.0f),
        glm::vec3(-175.0f, -20.0f, 235.0f),
        glm::vec3(-125.0f, -20.0f, 275.0f),
        glm::vec3(-60.0f, -20.0f, 260.0f),
    };

    // ------------------------------------------------
    // Fill sceneObjects with trees and rocks
    sceneObjects.clear();

    float treeRadius = 20.0f;
    float rockRadius = 40.0f;

    for (int i = 0; i < 20; ++i) {
        SceneObject so;
        so.position = treePositions[i];
        so.radius = treeRadius;
        sceneObjects.push_back(so);
    }

    for (int i = 0; i < 12; ++i) {
        SceneObject so;
        so.position = rockPositions[i];
        so.radius = rockRadius;
        sceneObjects.push_back(so);
    }

    // Spawn a new meteor every 3s
    float meteorSpawnTimer = 0.0f;


    glm::vec3 backpackPosition = glm::vec3(200.0f, -13.0f, 242.0f); // Position of the backpack
    bool backpackFound = false;

    glm::vec3 ghillieSuitPosition = glm::vec3(-280.0f, -20.0f, -200.0f); // Position of the beacon
    bool ghillieSuitFound = false;  // To track whether the ghillie suit has been found

    glm::vec3 hiddenMapPosition = glm::vec3(250.0f, -20.0f, 200.0f);  // Random position for the hidden map
    bool hiddenMapFound = false;

    glm::vec3 beaconPosition = glm::vec3(-260.0f, -20.0f, 170.0f);  // Position for the beacon
    glm::vec3 keyPosition = glm::vec3(-220.0f, -10.0f, 160.0f);      // Position for the key (near the beacon)
    bool keyFound = false; // Track if the key has been picked up
    bool beaconActivated = false;  // Track if the beacon is activated

    glm::vec3 helicopterPosition = glm::vec3(150.0f, 0.0f, -150.0f);

    bool taskDisplayed = false;
    bool isInvisibleToDino = false;
    bool escapeActivated = false;

    float beaconCollisionRadius = 8.f;
    float helicopterCollisionRadius = 12.f;

    // ------------------------------------------------
    // Main loop
    while (!window.isPressed(GLFW_KEY_ESCAPE) &&
        !glfwWindowShouldClose(window.getWindow()))
    {  
        window.clear();
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        glm::mat4 projection = glm::perspective(
            glm::radians(90.0f),
            (float)window.getWidth() / (float)window.getHeight(),
            0.1f,
            10000.0f
        );

        // Draw the sky sphere
        drawSkySphere(skySphere, shader, camera.getCameraPosition(), projection);
        shader.use();

        // Normal camera-based
        glm::mat4 view = glm::lookAt(
            camera.getCameraPosition(),
            camera.getCameraPosition() + camera.getCameraViewDirection(),
            camera.getCameraUp()
        );

        // Movement & collisions
        processKeyboardInput();

        if (!taskDisplayed) {
            displayCurrentTask();
            taskDisplayed = true; // Mark task as displayed
        }

        // Example mouse usage
        if (window.isMousePressed(GLFW_MOUSE_BUTTON_LEFT)) {
            std::cout << "Pressing mouse button" << std::endl;
        }

        // ------------------------------------------------
        // Meteors
        // Example: spawn a meteor every 3s
        meteorSpawnTimer += deltaTime;
        if (meteorSpawnTimer > 3.0f) {
            spawnMeteor();
            meteorSpawnTimer = 0.0f;
        }

        // Update meteors
        updateMeteors(deltaTime);

        // ------------------------------------------------
        // Day-night cycle
        dayNightTime += deltaTime;
        if (dayNightTime > cycleDuration) {
            dayNightTime -= cycleDuration; // wrap around
        }
        float cycleangle = 2.0f * 3.14159f * (dayNightTime / cycleDuration);

        // Let’s assume the sun is ~200 units away from the origin, and goes around in a big circle
        float radius = 200.0f;
        float sunX = radius * cos(cycleangle);
        float sunZ = radius * sin(cycleangle);
        // Let’s keep the Y somewhat high to mimic overhead sunlight
        float sunY = 200.0f + 100.0f * sin(cycleangle); // or just pick something like 100 or 150

        lightPos = glm::vec3(sunX, sunY, sunZ);

        float dayFactor = 0.5f * (1.0f + sin(cycleangle));

        // dayFactor=1 => bright day color
        // dayFactor=0 => dark night color
        glm::vec3 dayColor(0.5f, 0.7f, 1.0f);   // bright, slightly bluish
        glm::vec3 nightColor(0.05f, 0.05f, 0.1f); // near-black, faintly bluish

        glm::vec3 currentAmbient = glm::mix(nightColor, dayColor, dayFactor);

        // You can also change glClearColor if you want the background to change
        glClearColor(currentAmbient.r, currentAmbient.g, currentAmbient.b, 1.0f);

        // ------------------------------------------------
        // Light
        sunShader.use();

        glm::mat4 ProjectionMatrix = glm::perspective(
            90.0f,
            (float)window.getWidth() / (float)window.getHeight(),
            0.1f,
            10000.0f
        );
        glm::mat4 ViewMatrix = glm::lookAt(
            camera.getCameraPosition(),
            camera.getCameraPosition() + camera.getCameraViewDirection(),
            camera.getCameraUp()
        );

        GLuint MatrixID = glGetUniformLocation(sunShader.getId(), "MVP");

        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, lightPos);
        glm::mat4 MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);

        sun.draw(sunShader);

        // ------------------------------------------------
        // Scene shader
        shader.use();

        GLuint MatrixID2 = glGetUniformLocation(shader.getId(), "MVP");
        GLuint ModelMatrixID = glGetUniformLocation(shader.getId(), "model");

        // Example “box” at origin
        ModelMatrix = glm::mat4(1.0f);
        MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
        glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
        glUniform3f(glGetUniformLocation(shader.getId(), "lightColor"),
            lightColor.x, lightColor.y, lightColor.z);
        glUniform3f(glGetUniformLocation(shader.getId(), "lightPos"),
            lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(glGetUniformLocation(shader.getId(), "viewPos"),
            camera.getCameraPosition().x,
            camera.getCameraPosition().y,
            camera.getCameraPosition().z);

        // Plane (the ground)
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(0.0f, -20.0f, 0.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(5.0f, 1.0f, 7.0f));
        MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
        glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
        plane.draw(shader);

        // ------------------------------------------------
        // Calculate the direction vector from the T-Rex to the player
        glm::vec3 playerPosition = camera.getCameraPosition(); // Get player's current position
        glm::vec3 flatPlayerPosition = glm::vec3(playerPosition.x, 0.0f, playerPosition.z); // Ignore Y-axis
        glm::vec3 flatDinoPosition = glm::vec3(dinoPosition.x, 0.0f, dinoPosition.z); // Ignore Y-axis

        glm::vec3 direction = glm::normalize(flatPlayerPosition - flatDinoPosition); // Normalize the direction vector;

        // Update the T-Rex position
        float dinoSpeed = 10.0f; // Adjust the speed as needed
        dinoPosition.y = -20.0f; // Ensure the T-Rex stays on the ground

        if (!isInvisibleToDino) {
            dinoPosition += direction * dinoSpeed * deltaTime; // Move the T-Rex toward the player
        }

        // Calculate rotation angle to face the player
        float angle = atan2(direction.x, direction.z); // Use X and Z components only

        // Update ModelMatrix for the T-Rex
        ModelMatrix = glm::translate(glm::mat4(1.0f), dinoPosition); // Position the T-Rex
        ModelMatrix = glm::rotate(ModelMatrix, -angle, glm::vec3(0.0f, 1.0f, 0.0f)); // Rotate the T-Rex around the Y-axis
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(60.0f, 60.0f, 60.0f)); // Scale the T-Rex
        MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

        // Assign texture to the T-Rex
        std::vector<Texture> dinoTextures;
        dinoTextures.push_back(Texture{ dinoTexture, "texture_diffuse" });
        dino.setTextures(dinoTextures);

        // Update shader uniforms
        glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

        // Draw the T-Rex
        dino.draw(shader);

        // ------------------------------------------------
        // Draw all trees
        for (int i = 0; i < 20; ++i) {
            glm::vec3 position = treePositions[i];

            // trunk
            ModelMatrix = glm::mat4(1.0f);
            ModelMatrix = glm::translate(ModelMatrix, position);
            ModelMatrix = glm::scale(ModelMatrix, glm::vec3(5.0f));
            MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
            glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
            tree_trunk.draw(shader);

            // crown
            ModelMatrix = glm::mat4(1.0f);
            ModelMatrix = glm::translate(ModelMatrix, position + glm::vec3(0.0f, -4.0f, 0.0f));
            ModelMatrix = glm::scale(ModelMatrix, glm::vec3(5.0f));
            MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
            glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
            tree_crown.draw(shader);
        }

        // ------------------------------------------------
        // Draw rocks
        std::vector<Texture> rockTextures;
        rockTextures.push_back(Texture{ tex4, "texture_diffuse" });
        rock.setTextures(rockTextures);

        for (int i = 0; i < 12; ++i) {
            glm::vec3 position = rockPositions[i];
            ModelMatrix = glm::mat4(1.0f);
            ModelMatrix = glm::translate(ModelMatrix, position);
            ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.2f));
            MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
            glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
            rock.draw(shader);
        }

        // ------------------------------------------------
        // Draw walls
        std::vector<Texture> wallTextures;
        wallTextures.push_back(Texture{ tex4, "texture_diffuse" });
        walls.setTextures(wallTextures);

        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, glm::vec3(-40.0f, -10.0f, 40.0f));
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(25.0f, 30.0f, 25.0f));
        MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
        glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);
        walls.draw(shader);

        // -----------------------------------------------
        meteorShader.use();
        // Draw meteors
        glUniform3f(glGetUniformLocation(meteorShader.getId(), "viewPos"),
            camera.getCameraPosition().x,
            camera.getCameraPosition().y,
            camera.getCameraPosition().z);
        glUniform3f(glGetUniformLocation(meteorShader.getId(), "lightPos"),
            lightPos.x, lightPos.y, lightPos.z);
        glUniform3f(glGetUniformLocation(meteorShader.getId(), "lightColor"),
            lightColor.x, lightColor.y, lightColor.z);
        drawMeteors(meteorShader, meteorMesh,
            ProjectionMatrix, ViewMatrix,
            MatrixID2, ModelMatrixID);

        // --------------------------------------------
        //backpack
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, backpackPosition);
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(2.0f, 2.0f, 2.0f)); // Adjust size if needed
        MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;
        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

        glm::vec3 playerPosition1 = camera.getCameraPosition(); // Get player's current position

        if (!backpackFound && isPlayerNearBackpack(camera.getCameraPosition(), backpackPosition, 20.0f)) {
            backpackFound = true; // Mark backpack as found
            std::cout << "You found the backpack!" << std::endl;
            tasks[0].completed = true;
            taskDisplayed = false;
        }

        // Render the backpack if it hasn’t been found
        if (!backpackFound) {
            ModelMatrix = glm::mat4(1.0f);
            ModelMatrix = glm::translate(ModelMatrix, backpackPosition);
            ModelMatrix = glm::scale(ModelMatrix, glm::vec3(2.0f, 2.0f, 2.0f)); // Adjust size if needed
            MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

            glUniformMatrix4fv(MatrixID2, 1, GL_FALSE, &MVP[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

            backpack.draw(shader); // Render the backpack
        }

        // Task 2: Ghillie Suit
        if (backpackFound && !ghillieSuitFound && isPlayerNearGhillieSuit(camera.getCameraPosition(), ghillieSuitPosition, 20.0f)) {
            ghillieSuitFound = true; // Mark the ghillie suit as found
            isInvisibleToDino = true;
            std::cout << "You found the ghillie suit!" << std::endl; // Print message
            tasks[1].completed = true; // Mark Task 2 as completed
            taskDisplayed = false;
        }

        // Render the ghillie suit if it hasn't been found
        if (!ghillieSuitFound && backpackFound) {
            ModelMatrix = glm::mat4(1.0f);
            ModelMatrix = glm::translate(ModelMatrix, ghillieSuitPosition);
            ModelMatrix = glm::scale(ModelMatrix, glm::vec3(2.0f, 2.0f, 2.0f)); // Adjust size if needed
            MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

            // Draw the ghillie suit with the texture applied
            ghillieSuitMesh.draw(shader);
        }

        // Task 3: Hidden Map
        if (backpackFound && ghillieSuitFound && !hiddenMapFound && isPlayerNearHiddenMap(camera.getCameraPosition(), hiddenMapPosition, 20.0f)) {
            hiddenMapFound = true; // Mark the hidden map as found
            std::cout << "You found the hidden map!" << std::endl; // Print message
            tasks[2].completed = true; // Mark Task 3 as completed
            taskDisplayed = false;
        }

        // Render the hidden map if it hasn't been found
        if (!hiddenMapFound && backpackFound && ghillieSuitFound) {
            ModelMatrix = glm::mat4(1.0f);
            ModelMatrix = glm::translate(ModelMatrix, hiddenMapPosition);
            ModelMatrix = glm::scale(ModelMatrix, glm::vec3(2.0f, 2.0f, 2.0f)); // Adjust size if needed
            MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

            // Draw the hidden map (box.obj)
            hiddenmap.draw(shader);
        }

        // Task 4: Beacon and Key
        if (hiddenMapFound && !keyFound && isPlayerNearKey(camera.getCameraPosition(), keyPosition, 10.0f)) {
            std::cout << "You found the key!" << std::endl;
            keyFound = true;
            std::cout << "Press 'E' to pick it up." << std::endl;

            if (window.isPressed(GLFW_KEY_E)) {
                keyFound = true;  // Mark the key as picked up
                std::cout << "You have picked up the key!" << std::endl;
            }
        }

        // Render the beacon (always visible)
        ModelMatrix = glm::mat4(1.0f);
        ModelMatrix = glm::translate(ModelMatrix, beaconPosition);
        ModelMatrix = glm::scale(ModelMatrix, glm::vec3(.4f, .4f, .4f)); // Adjust size if needed
        MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

        glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
        glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

        beacon.draw(shader); // Always draw the beacon

        // Render the key if the player has completed Task 3 (Hidden Map)
        if (hiddenMapFound && !keyFound) {
            ModelMatrix = glm::mat4(1.0f);
            ModelMatrix = glm::translate(ModelMatrix, keyPosition);
            ModelMatrix = glm::scale(ModelMatrix, glm::vec3(6.0f, 6.0f, 6.0f)); // Adjust size if needed
            MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

            key.draw(shader); // Render the key (only visible after Hidden Map is found)
        }

        if (keyFound && !beaconActivated && isPlayerNearBeacon(camera.getCameraPosition(), beaconPosition, 70.0f)) {
            if (window.isPressed(GLFW_KEY_E)) { // Player presses 'E' to activate the beacon
                beaconActivated = true; // Beacon activated
                std::cout << "You activated the power beacon!" << std::endl;
                std::cout << "Help is on the way!" << std::endl;
                tasks[3].completed = true; // Mark Task 4 as completed
                taskDisplayed = false;
            }
        }

        if (hiddenMapFound && !keyFound && isPlayerNearKey(camera.getCameraPosition(), keyPosition, 10.0f)) {
            keyFound = true;  // Mark the key as found automatically
            std::cout << "You found the key!" << std::endl;
            std::cout << "The key has been picked up!" << std::endl; // No need for 'E', it's automatic
        }


        if (beaconActivated) { // Helicopter position (adjust as needed)

            // Render the helicopter if the beacon is activated
            ModelMatrix = glm::mat4(1.0f);
            ModelMatrix = glm::translate(ModelMatrix, helicopterPosition);
            ModelMatrix = glm::scale(ModelMatrix, glm::vec3(0.03f, .03f, .03f)); // Adjust size if needed
            MVP = ProjectionMatrix * ViewMatrix * ModelMatrix;

            glUniformMatrix4fv(MatrixID, 1, GL_FALSE, &MVP[0][0]);
            glUniformMatrix4fv(ModelMatrixID, 1, GL_FALSE, &ModelMatrix[0][0]);

            helicopter.draw(shader); // Render the helicopter
        }

        if (beaconActivated && !escapeActivated && isPlayerNearBeacon(camera.getCameraPosition(), helicopterPosition, 70.0f)) {
            if (window.isPressed(GLFW_KEY_E)) {
                escapeActivated = true;  // Mark escape as triggered
                std::cout << "Good job, you escaped!" << std::endl;  // Print message in the console
            }
        }

        if (escapeActivated) {
            // Fade the screen to black
            glClearColor(0.0f, 0.0f, 0.0f, 1.0f);  // Set screen color to black
            window.clear();  // Clear the window to black
            // End the game (you could also add a countdown or additional transition logic here if desired)
            glfwSetWindowShouldClose(window.getWindow(), GL_TRUE);  // Close the window when the player escapes
        }

        window.update();
    }

    return 0;
}

// ------------------------------------------------
// MOUSE State
float lastX = 400.0f;
float lastY = 400.0f;
bool firstMouse = true;

// ------------------------------------------------
// KEYBOARD + Collision
void processKeyboardInput()
{
    float cameraSpeed = 30.0f * deltaTime;

    // Store old position
    glm::vec3 oldPosition = camera.getCameraPosition();

    // Move the camera
    if (window.isPressed(GLFW_KEY_LEFT_SHIFT) || window.isPressed(GLFW_KEY_RIGHT_SHIFT)) {
        cameraSpeed *= 5.0f;
    }
    if (window.isPressed(GLFW_KEY_W)) { camera.keyboardMoveFront(cameraSpeed); }
    if (window.isPressed(GLFW_KEY_S)) { camera.keyboardMoveBack(cameraSpeed); }
    if (window.isPressed(GLFW_KEY_A)) { camera.keyboardMoveLeft(cameraSpeed); }
    if (window.isPressed(GLFW_KEY_D)) { camera.keyboardMoveRight(cameraSpeed); }

    // Jump logic
    if (window.isPressed(GLFW_KEY_SPACE)) {
        camera.startJump(25.0f);
    }
    camera.updateJump(deltaTime);

    // Collision checks
    glm::vec3 newPos = camera.getCameraPosition();

    // Check collisions with trees & rocks
    bool collidedWithObject = false;
    for (auto& obj : sceneObjects) {
        if (checkCollisionSphere(newPos, cameraRadius, obj.position, obj.radius)) {
            collidedWithObject = true;
            break;
        }
    }

    // Check boundary and stay inside the big sphere
    float distToWalls = glm::distance(newPos, wallCenter);
    bool outsideWalls = distToWalls > (wallRadius - cameraRadius);

    // If any collision, revert
    if (collidedWithObject || outsideWalls) {
        camera.setCameraPosition(oldPosition);
    }
}

// ------------------------------------------------
// Mouse handling (unused in your loop)
void handleMouseInput()
{
    double mouseX, mouseY;
    glfwGetCursorPos(window.getWindow(), &mouseX, &mouseY);

    int screenWidth = window.getWidth();
    int screenHeight = window.getHeight();
    double centerX = (double)screenWidth / 2.0;
    double centerY = (double)screenHeight / 2.0;

    static double lastX_ = centerX;
    static double lastY_ = centerY;

    double offsetX = mouseX - lastX_;
    double offsetY = mouseY - lastY_;

    float sensitivity = 0.1f;
    offsetX = offsetX * sensitivity;
    offsetY = offsetY * sensitivity;

    camera.rotateOy((float)offsetX);
    // camera.rotateOx((float)offsetY); // if needed

    // reset cursor
    glfwSetCursorPos(window.getWindow(), centerX, centerY);
    lastX_ = centerX;
    lastY_ = centerY;
}