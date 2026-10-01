#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <fstream>
#include <string>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

static unsigned int ss_id = 0;
void dump_framebuffer_to_ppm(std::string prefix, unsigned int width, unsigned int height)
{
    int pixelChannel = 3;
    int totalPixelSize = pixelChannel * width * height * sizeof(GLubyte);
    GLubyte *pixels = new GLubyte[totalPixelSize];
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels);
    std::string file_name = prefix + std::to_string(ss_id) + ".ppm";
    std::ofstream fout(file_name);
    fout << "P3\n"
         << width << " " << height << "\n"
         << 255 << std::endl;
    for (size_t i = 0; i < height; i++)
    {
        for (size_t j = 0; j < width; j++)
        {
            size_t cur = pixelChannel * ((height - i - 1) * width + j);
            fout << (int)pixels[cur] << " " << (int)pixels[cur + 1] << " " << (int)pixels[cur + 2] << " ";
        }
        fout << std::endl;
    }
    ss_id++;
    delete[] pixels;
    fout.flush();
    fout.close();
}

// key board control
void processInput(GLFWwindow *window)
{
    // press escape to exit
    if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(window, true);

    // press p to capture screen
    if (glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS)
    {
        std::cout << "Capture Window " << ss_id << std::endl;
        int buffer_width, buffer_height;
        glfwGetFramebufferSize(window, &buffer_width, &buffer_height);
        dump_framebuffer_to_ppm("Assignment01-ss", buffer_width, buffer_height);
    }
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// shader
const char *vertexShaderSource = "#version 410 core\n"
                                 "layout (location = 0) in vec3 aPos;\n"
                                 "layout (location = 1) in vec3 aColor;\n"
                                 "\n"
                                 "out vec3 vertexColor;\n"
                                 "\n"
                                 "uniform mat4 model;\n"
                                 "uniform mat4 view;\n"
                                 "uniform mat4 projection;\n"
                                 "\n"
                                 "void main()\n"
                                 "{\n"
                                 "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
                                 "   vertexColor = aColor;\n"
                                 "}\0";

const char *fragmentShaderSource = "#version 410 core\n"
                                   "in vec3 vertexColor;\n"
                                   "out vec4 FragColor;\n"
                                   "\n"
                                   "void main()\n"
                                   "{\n"
                                   "   FragColor = vec4(vertexColor, 1.0);\n"
                                   "}\n\0";

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    int windowWidth = 1024;
    int windowHeight = 768;
    GLFWwindow *window = glfwCreateWindow(windowWidth, windowHeight, "Assignment01", NULL, NULL);

    if (window == NULL)
    {
        std::cout << "GLFW Window Failed" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "GLAD Initialization Failed" << std::endl;
        return -1;
    }

    // shaders
    int success;
    char error_msg[512];
    unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vs, 1, &vertexShaderSource, NULL);
    glCompileShader(vs);
    glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(vs, 512, NULL, error_msg);
        std::cout << "Vertex Shader Failed: " << error_msg << std::endl;
    }
    unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fs, 1, &fragmentShaderSource, NULL);
    glCompileShader(fs);
    glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
    if (!success)
    {
        glGetShaderInfoLog(fs, 512, NULL, error_msg);
        std::cout << "Fragment Shader Failed: " << error_msg << std::endl;
    }
    unsigned int shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vs);
    glAttachShader(shaderProgram, fs);
    glLinkProgram(shaderProgram);
    glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
    if (!success)
    {
        glGetProgramInfoLog(shaderProgram, 512, NULL, error_msg);
        std::cout << "Program Link Error: " << error_msg << std::endl;
    }
    glDeleteShader(vs);
    glDeleteShader(fs);

    // Cube vertex array
    // Each vertex: x, y, z, r, g, b
    float cube_verts[] = {

        // FRONT: z = 1, fuchsia (1, 0, 1)
        -1, 1, 1, 1, 0, 1,
        -1, -1, 1, 1, 0, 1,
        1, 1, 1, 1, 0, 1,

        1, 1, 1, 1, 0, 1,
        -1, -1, 1, 1, 0, 1,
        1, -1, 1, 1, 0, 1,

        // BACK: z = -1, yellow (1, 1, 0)
        -1, 1, -1, 1, 1, 0,
        -1, -1, -1, 1, 1, 0,
        1, 1, -1, 1, 1, 0,

        1, 1, -1, 1, 1, 0,
        -1, -1, -1, 1, 1, 0,
        1, -1, -1, 1, 1, 0,

        // LEFT: x = -1, red (1, 0, 0)
        -1, 1, 1, 1, 0, 0,
        -1, -1, 1, 1, 0, 0,
        -1, 1, -1, 1, 0, 0,

        -1, 1, -1, 1, 0, 0,
        -1, -1, 1, 1, 0, 0,
        -1, -1, -1, 1, 0, 0,

        // RIGHT: x = 1, green (0, 1, 0)
        1, 1, 1, 0, 1, 0,
        1, -1, 1, 0, 1, 0,
        1, 1, -1, 0, 1, 0,

        1, 1, -1, 0, 1, 0,
        1, -1, 1, 0, 1, 0,
        1, -1, -1, 0, 1, 0,

        // TOP: y = 1, blue (0, 0, 1)
        -1, 1, -1, 0, 0, 1,
        -1, 1, 1, 0, 0, 1,
        1, 1, -1, 0, 0, 1,

        1, 1, -1, 0, 0, 1,
        -1, 1, 1, 0, 0, 1,
        1, 1, 1, 0, 0, 1,

        // BOTTOM: y = -1, aqua (0, 1, 1)
        -1, -1, 1, 0, 1, 1,
        -1, -1, -1, 0, 1, 1,
        1, -1, 1, 0, 1, 1,

        1, -1, 1, 0, 1, 1,
        -1, -1, -1, 0, 1, 1,
        1, -1, -1, 0, 1, 1};

    unsigned int VBO, VAO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glBindVertexArray(VAO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(cube_verts), cube_verts, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(float), (void *)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    glm::mat4 model = glm::mat4(1.0f);

    glm::mat4 view = glm::lookAt(
        glm::vec3(-70.0f, 80.0f, 90.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f));

    glm::mat4 projection = glm::perspective(
        glm::radians(30.0f),
        4.0f / 3.0f,
        0.1f,
        1000.0f);

    int modelLoc = glGetUniformLocation(shaderProgram, "model");
    int viewLoc = glGetUniformLocation(shaderProgram, "view");
    int projectionLoc = glGetUniformLocation(shaderProgram, "projection");

    glEnable(GL_DEPTH_TEST);

    while (!glfwWindowShouldClose(window))
    {
        processInput(window);

        glClearColor(0.3f, 0.4f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glUseProgram(shaderProgram);

        glUniformMatrix4fv(
            viewLoc, 1, GL_FALSE,
            glm::value_ptr(view));

        glUniformMatrix4fv(
            projectionLoc, 1, GL_FALSE,
            glm::value_ptr(projection));

        glBindVertexArray(VAO);

        // ----- SUN -----

        glm::mat4 sunModel = glm::mat4(1.0f);

        sunModel = glm::scale(
            sunModel,
            glm::vec3(6.0f));

        glUniformMatrix4fv(
            modelLoc, 1, GL_FALSE,
            glm::value_ptr(sunModel));

        glDrawArrays(GL_TRIANGLES, 0, 36);

        // ----- EARTH -----

        glm::mat4 earthModel = glm::mat4(1.0f);

        earthModel = glm::translate(
            earthModel,
            glm::vec3(24.0f, 0.0f, 0.0f));

        earthModel = glm::rotate(
            earthModel,
            glm::radians(-23.4f),
            glm::vec3(0.0f, 0.0f, 1.0f));

        earthModel = glm::scale(
            earthModel,
            glm::vec3(3.0f));

        glUniformMatrix4fv(
            modelLoc, 1, GL_FALSE,
            glm::value_ptr(earthModel));

        glDrawArrays(GL_TRIANGLES, 0, 36);

        // ----- MOON -----

        glm::mat4 moonModel = glm::mat4(1.0f);

        moonModel = glm::translate(
            moonModel,
            glm::vec3(36.0f, 0.0f, 0.0f));

        moonModel = glm::scale(
            moonModel,
            glm::vec3(1.5f));

        glUniformMatrix4fv(
            modelLoc, 1, GL_FALSE,
            glm::value_ptr(moonModel));

        glDrawArrays(GL_TRIANGLES, 0, 36);

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // release resource
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteProgram(shaderProgram);
    glfwTerminate();
    return 0;
}
