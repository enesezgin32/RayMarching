#include <string>

// Loader for OpenGL extensions
// http://glad.dav1d.de/
// THIS IS OPTIONAL AND NOT REQUIRED, ONLY USE THIS IF YOU DON'T WANT GLAD TO INCLUDE windows.h
// GLAD will include windows.h for APIENTRY if it was not previously defined.
// Make sure you have the correct definition for APIENTRY for platforms which define _WIN32 but don't use __stdcall
#ifdef _WIN32
    #define APIENTRY __stdcall
#endif

#include <glad/glad.h>

// GLFW library to create window and to manage I/O
// ImGUI for UI elements
#include <imgui/imgui.h>
#include <imgui/imgui_impl_glfw.h>
#include <imgui/imgui_impl_opengl3.h>
#include <glfw/glfw3.h>

// another check related to OpenGL loader
// confirm that GLAD didn't include windows.h
#ifdef _WINDOWS_
    #error windows.h was included!
#endif

// Utility classes
#include <utils/shader.h>
#include <utils/camera.h>
#include <utils/physics.h>

// we load the GLM classes used in the application
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

// dimensions of application's window
GLuint screenWidth = 900, screenHeight = 900;
GLFWwindow* window;

// callback functions for keyboard and mouse events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_button_callback(GLFWwindow* window, int button, int action, int mods);
void ApplyOtherInputs();
void ApplyCameraMovements();

//Create UI elements
void UIElements();

// basic vertex shader creater
int currentScene = 1;
void BasicMesh(GLuint &VAO,  GLuint &VBO, GLuint &EBO);

// illumination setup
glm::vec3 lightPos0 = glm::vec3(5.0f, 5.0f, 5.0f);

// weights for the diffusive, specular and ambient components
GLfloat Kd = 0.5f;
GLfloat Ks = 0.4f;
GLfloat Ka = 0.1f;
// shininess coefficient for Blinn-Phong shader
GLfloat shininess = 25.0f;

// roughness index for GGX shader
GLfloat alpha = 0.2f;
// Fresnel reflectance at 0 degree (Schlik's approximation)
GLfloat F0 = 0.9f;


// bouncy balls sene setup
void BouncyBallSceneSetup();
btRigidBody* CreateBall(glm::vec3 position);

// key array and menu mode tools 
bool keys[1024];
bool menu_lock = true;
bool menu_mode_on;

// we need to store the previous mouse position to calculate the offset with the current frame
GLfloat lastX, lastY;

// when rendering the first frame, we do not have a "previous state" for the mouse, so we need to manage this situation
bool firstMouse = true;

// parameters for time calculation (for animations)
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;
GLfloat timer = 0.0f;
GLfloat sceneCount = 0.0f;
GLfloat fps = 0.0f;

// we create a camera. We pass the initial position as a parameter to the constructor. The last boolean tells that we want a camera "anchored" to the ground
Camera camera(glm::vec3(0.0f, 0.0f, -7.0f), GL_FALSE);

// physics starter
Physics bulletSimulation;
glm::vec3 bouncyBalls[100];
int ballAmount = 5;
float visibleSize = 0.7;
float collisionSize = 0.3;
float blending = 1;

int fractalIteration = 0;

/////////////////// MAIN function ///////////////////////
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    
    // we create the application's window
    window = glfwCreateWindow(screenWidth, screenHeight, "RayMarching", nullptr, nullptr);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(0);

    // we put in relation the window and the callbacks
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_button_callback);

    // we disable the mouse cursor
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLAD tries to load the context set by GLFW
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    // we define the viewport dimensions
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // we enable Z test
    glEnable(GL_DEPTH_TEST);

    // the "clear" color for the frame buffer
    glClearColor(0.05f, 0.05f, 0.05f, 1.0f);

    // we create and compile shaders
    Shader shader1("basic.vert", "bouncyBalls.frag");
    Shader shader2("basic.vert", "fractalScene.frag");
    Shader shader3("basic.vert", "testScene.frag");
    GLuint VBO, VAO, EBO;
    BasicMesh(VBO, VAO, EBO);

    //ImGUI setup
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO();(void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 410 core");

    // Bouncy Ball Scene setup
    BouncyBallSceneSetup();

    //////////////////////////////////
    while(!glfwWindowShouldClose(window))
    {
        // we determine the time passed from the beginning
        // and we calculate time difference between current frame rendering and the previous one
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;
        GLfloat maxSecPerFrame = 1.0f / 60.0f;
        bulletSimulation.dynamicsWorld->stepSimulation((deltaTime < maxSecPerFrame ? deltaTime : maxSecPerFrame),10);

        if(currentFrame - timer > 2)
        {
            sceneCount = 0;
            fps = 0;
            timer = currentFrame;
            //ballAmount+=1;
        }
        else
        {
            fps = (fps*sceneCount + 1/deltaTime)/(sceneCount+1);
            sceneCount++;
        }
        
        
        // Check is an I/O event is happening
        glfwPollEvents();
        ApplyCameraMovements();
        ApplyOtherInputs();
        Shader shader(shader1);
        if(currentScene == 1)
        {
            shader1.Use();
            shader = shader1;
            // PHYSICS PART
            if(ballAmount > bulletSimulation.dynamicsWorld->getNumCollisionObjects()-6)
                CreateBall(glm::vec3(0,2,0));
            // we cycle among all the Rigid Bodies (starting from 1 to avoid the plane)
            int num_cobjs = bulletSimulation.dynamicsWorld->getNumCollisionObjects();
            btTransform transform;
            GLfloat matrix[16];
            for (int i=6, x=0; i<num_cobjs;i++)
            {
                // we take the Collision Object from the list
                btCollisionObject* obj = bulletSimulation.dynamicsWorld->getCollisionObjectArray()[i];
                // we upcast it in order to use the methods of the main class RigidBody
                btRigidBody* body = btRigidBody::upcast(obj);
                // we take the transformation matrix of the rigid boby, as calculated by the physics engine
                body->getMotionState()->getWorldTransform(transform);
                body->getCollisionShape()->setLocalScaling(btVector3(collisionSize, collisionSize, collisionSize));
                bulletSimulation.dynamicsWorld->updateSingleAabb(obj);
                // apply impulse when pressed space
                if(keys[GLFW_KEY_SPACE])
                {
                    btVector3 impulse(rand()%5-2, rand()%5-2, rand()%5-2);
                    body->activate(true);
                    body->applyCentralImpulse(impulse);
                }
                //Set new positions
                bouncyBalls[x++] = glm::vec3(transform.getOrigin().getX(), transform.getOrigin().getY(),transform.getOrigin().getZ());
            }

            glUniform1f(glGetUniformLocation(shader.Program, "visibleSize"), visibleSize);
            glUniform3fv(glGetUniformLocation(shader.Program, "ballPos"), ballAmount, glm::value_ptr(*bouncyBalls));
            glUniform1i(glGetUniformLocation(shader.Program, "ballAmount"), ballAmount);
            glUniform1f(glGetUniformLocation(shader.Program, "blending"), blending);
        }
        
        if(currentScene == 2)
        {
            shader2.Use();
            shader = shader2;
            glUniform1i(glGetUniformLocation(shader.Program, "fractalIteration"), fractalIteration);
        }
        if(currentScene == 3)
        {
            shader3.Use();
            shader = shader3;
        }
        //Uniforms setup 
        glUniform3f(glGetUniformLocation(shader.Program, "resolution"), (float)width, (float) height, 0.0);
        glUniform3fv(glGetUniformLocation(shader.Program, "camPos"), 1, glm::value_ptr(camera.Position));
        glUniform3fv(glGetUniformLocation(shader.Program, "camDir"), 1, glm::value_ptr(camera.Front));
        glm::mat4 view = camera.GetViewMatrix();
        view = glm::transpose(view);
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "transposedCamView"), 1, GL_FALSE, glm::value_ptr(view));
        glUniform1f(glGetUniformLocation(shader.Program, "time"), currentFrame);


        // we "clear" the frame and z  buffer
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        //Imgui Setup
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        // we render the model
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glBindVertexArray(0);

        //UI elements
        UIElements();

        // Swapping back and front buffers
        glfwSwapBuffers(window);
    }

    bulletSimulation.Clear();
    ImGui_ImplGlfw_Shutdown();
    ImGui_ImplOpenGL3_Shutdown();
    ImGui::DestroyContext();
    shader1.Delete();
    shader2.Delete();
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    // we close and delete the created context
    glfwTerminate();
    return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);
    if(action == GLFW_PRESS)
    {
        keys[key] = true;
    }
    else if(action == GLFW_RELEASE)
    {
        keys[key] = false;
        if(key == GLFW_MOUSE_BUTTON_1)
            menu_lock = false;
    }
        
}

void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
      // we move the camera view following the mouse cursor
      // we calculate the offset of the mouse cursor from the position in the last frame
      // when rendering the first frame, we do not have a "previous state" for the mouse, so we set the previous state equal to the initial values (thus, the offset will be = 0)
      if(firstMouse)
      {
          lastX = xpos;
          lastY = ypos;
          firstMouse = false;
      }

      // offset of mouse cursor position
      GLfloat xoffset = xpos - lastX;
      GLfloat yoffset = lastY - ypos;

      // the new position will be the previous one for the next frame
      lastX = xpos;
      lastY = ypos;

      // we pass the offset to the Camera class instance in order to update the rendering
      if(!menu_mode_on)
        camera.ProcessMouseMovement(xoffset, yoffset);
}

void mouse_button_callback(GLFWwindow* window, int button, int action, int mods)
{
    if(action == GLFW_PRESS)
    {
        keys[button] = true;
    }
    else if(action == GLFW_RELEASE)
    {
        keys[button] = false;
        if(button == GLFW_MOUSE_BUTTON_2)
            menu_lock = false;
    }
}

void ApplyCameraMovements()
{
    if(keys[GLFW_KEY_W])
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if(keys[GLFW_KEY_S])
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if(keys[GLFW_KEY_A])
        camera.ProcessKeyboard(LEFT, deltaTime);
    if(keys[GLFW_KEY_D])
        camera.ProcessKeyboard(RIGHT, deltaTime);
    if(keys[GLFW_KEY_E])
        camera.ProcessKeyboard(UP, deltaTime);
    if(keys[GLFW_KEY_Q])
        camera.ProcessKeyboard(DOWN, deltaTime);
}

void ApplyOtherInputs()
{
    if(!menu_lock)
    {
        menu_lock = true;
        if(menu_mode_on)
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            menu_mode_on = false;
        }
        else
        {
            glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
            menu_mode_on = true;
        }
    }
    if(keys[GLFW_KEY_1])
        currentScene = 1;
    else if(keys[GLFW_KEY_2])
        currentScene = 2;
    else if(keys[GLFW_KEY_3])
        currentScene = 3;
}

void BasicMesh(GLuint &VAO,  GLuint &VBO, GLuint &EBO)
{
    
    /////////////////////////////////////
    // mesh information, and setup ov VAO, VBO and EBO buffers

    // the current mesh consists of only 4 vertices and 2 triangles
    // in the next lecture, we will load models from external files,
    // and we will implement classes for the management of the data structures
    // --- Setted vertices so that they will cover the whole screen
    GLfloat vertices[] = {
         1.0f,  1.0f, 0.0f,  // Top Right
         1.0f, -1.0f, 0.0f,  // Bottom Right
        -1.0f, -1.0f, 0.0f,  // Bottom Left
        -1.0f,  1.0f, 0.0f   // Top Left
    };
    GLuint indices[] = {  // Note that we start from 0!
        0, 1, 3,  // First Triangle
        1, 2, 3   // Second Triangle
    };

    // buffer objects\arrays are initialized
    // a brief description of their role and how they are binded can be found at:
    // https://learnopengl.com/#!Getting-started/Hello-Triangle
    // (in different parts of the page), or here:
    // http://www.informit.com/articles/article.aspx?p=1377833&seqNum=8

    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    
    // VAO is made "active"    
    glBindVertexArray(VAO);
    // we copy data in the VBO - we must set the data dimension, and the pointer to the structure cointaining the data
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    // we copy data in the EBO - we must set the data dimension, and the pointer to the structure cointaining the data
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // we set in the VAO the pointers to the vertex positions (with the relative offsets inside the data structure)
    // these will be the positions to use in the layout qualifiers in the shaders ("layout (location = ...)"")    
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(GLfloat), (GLvoid*)0);

    glBindBuffer(GL_ARRAY_BUFFER, 0); // Note that this is allowed, the call to glVertexAttribPointer registered VBO as the currently bound vertex buffer object so afterwards we can safely unbind
    glBindVertexArray(0); // Unbind VAO (it's always a good thing to unbind any buffer/array to prevent strange bugs), remember: do NOT unbind the EBO, keep it bound to this VAO


}

void UIElements()
{
    if(currentScene == 1)
    {
        ImGui::Begin("Bouncy Balls");
        ImGui::SliderInt("Ball Amount", &ballAmount, 5, 101,"Ball  = %.3f");
        ImGui::SliderFloat("Visible Size", &visibleSize, 0.1f, 3.0f,"Visible Size = %.3f");
        ImGui::SliderFloat("Collision Size", &collisionSize, 0.1f, 3.0f,"Collision Size = %.3f");
        ImGui::SliderFloat("Blending", &blending, 0.0f, 2.0f,"Blending = %.3f");
        ImGui::BulletText("WASD keys to move");
        ImGui::BulletText("RIGHT CLICK to toggle mouse");
        ImGui::BulletText("SPACE key to bounce balls");
        ImGui::BulletText("E/Q keys to fly up/down");
        ImGui::BulletText("1, 2, 3 keys to see other scenes");
        
        ImGui::End();
    }
    if(currentScene == 2)
    {
        ImGui::Begin("Fractal");
        ImGui::SliderInt(" ##1", &fractalIteration, 0, 20,"Iteration  = %.3f");
        ImGui::BulletText("You need some flying to see whole scene");
        ImGui::End();
    }

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
}

void BouncyBallSceneSetup()
{
    btRigidBody* walls1 = bulletSimulation.createRigidBody(BOX,glm::vec3(5,0,0),glm::vec3(2, 8, 8),glm::vec3(0),0.0f,0.3f,0.9f);
    btRigidBody* walls2 = bulletSimulation.createRigidBody(BOX,glm::vec3(-5,0,0),glm::vec3(2, 8, 8),glm::vec3(0),0.0f,0.3f,0.9f);
    btRigidBody* walls3 = bulletSimulation.createRigidBody(BOX,glm::vec3(0,5,0),glm::vec3(8, 2, 8),glm::vec3(0),0.0f,0.3f,0.9f);
    btRigidBody* walls4 = bulletSimulation.createRigidBody(BOX,glm::vec3(0,-5,0),glm::vec3(8, 2, 8),glm::vec3(0),0.0f,0.3f,0.9f);
    btRigidBody* walls5 = bulletSimulation.createRigidBody(BOX,glm::vec3(0,0,5),glm::vec3(8, 8, 2),glm::vec3(0),0.0f,0.3f,0.9f);
    btRigidBody* walls6 = bulletSimulation.createRigidBody(BOX,glm::vec3(0,0,-5),glm::vec3(8, 8, 2),glm::vec3(0),0.0f,0.3f,0.9f);

    
    for(int i = -1 ; i < 2; i++)
        for(int j = -1 ; j < 2; j++)
            for(int k = -1 ; k < 1; k++)
                CreateBall(glm::vec3(i, j, k));
    
}
btRigidBody* CreateBall(glm::vec3 position)
{
    btRigidBody* ball;
    ball = bulletSimulation.createRigidBody(SPHERE, position, glm::vec3(1), glm::vec3(0), 1, 0.3, 0.9);
    ball->getCollisionShape()->setLocalScaling(btVector3(collisionSize, collisionSize, collisionSize));
    return ball;
}

