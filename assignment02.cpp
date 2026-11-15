#define TINYOBJLOADER_DISABLE_FAST_FLOAT
#define TINYOBJLOADER_IMPLEMENTATION
#include "tiny_obj_loader.h"
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <iostream>
#include <cassert>
#include <fstream>
#include <string>
#include <vector>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void dump_framebuffer_to_ppm(const char *fileName, unsigned int width, unsigned int height)
{
    constexpr unsigned int pixelChannel = 3;
    std::vector<GLubyte> pixels(pixelChannel * width * height);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, pixels.data());
    std::ofstream fout(fileName);
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
    fout.close();
    if (fout)
        std::cout << "Saved " << fileName << '\n';
    else
        std::cerr << "Could not write " << fileName << '\n';
}

void framebuffer_size_callback(GLFWwindow *window, int width, int height)
{
    glViewport(0, 0, width, height);
}

// shader
const char *vertexShaderSource = "#version 330 core\n"
                                 "layout (location = 0) in vec3 aPos;\n"
                                 "layout (location = 1) in vec3 aNormal;\n"
                                 "\n"
                                 "out vec3 Normal;\n"
                                 "\n"
                                 "uniform mat4 model;\n"
                                 "uniform mat4 view;\n"
                                 "uniform mat4 projection;\n"
                                 "\n"
                                 "void main()\n"
                                 "{\n"
                                 "   gl_Position = projection * view * model * vec4(aPos, 1.0);\n"
                                 "   Normal = aNormal;\n"
                                 "}\0";

const char *fragmentShaderSource = "#version 330 core\n"
                                   "in vec3 Normal;\n"
                                   "out vec3 FragColor;\n"
                                   "\n"
                                   "void main()\n"
                                   "{\n"
                                   "   float c = dot(Normal, vec3(0.8, 0.7, 0.6));\n"
                                   "   FragColor = vec3(c, c, c);\n"
                                   "}\n\0";

struct FaceMesh
{
    tinyobj::attrib_t attrib;
    std::vector<tinyobj::shape_t> shapes;
};

bool loadFaceMesh(const std::string &obj_path, FaceMesh &mesh)
{
    std::vector<tinyobj::material_t> materials;
    std::string warn, err;
    bool bTriangulate = true;
    bool bSuc = tinyobj::LoadObj(
        &mesh.attrib, &mesh.shapes, &materials, &warn, &err,
        obj_path.c_str(), nullptr, bTriangulate);
    if (!bSuc)
    {
        std::cout << "tinyobj error loading " << obj_path << ": " << err << std::endl;
        return false;
    }
    return true;
}

std::vector<float> blendPositions(const FaceMesh &base,
                                  const std::vector<FaceMesh> &targets,
                                  const std::vector<float> &weights)
{
    assert(weights.size() == targets.size());
    const auto &basePositions = base.attrib.vertices;
    std::vector<float> newShape = basePositions;

    for (std::size_t i = 0; i < targets.size(); ++i)
    {
        const auto &targetPositions = targets[i].attrib.vertices;
        assert(targetPositions.size() == basePositions.size());

        for (std::size_t j = 0; j < basePositions.size(); ++j)
        {
            newShape[j] += weights[i] * (targetPositions[j] - basePositions[j]);
        }
    }

    return newShape;
}

int main()
{
    FaceMesh base;
    if (!loadFaceMesh("data/faces/base.obj", base))
        return 1;

    std::vector<FaceMesh> targets(35);
    for (std::size_t i = 0; i < targets.size(); ++i)
    {
        if (!loadFaceMesh("data/faces/" + std::to_string(i) + ".obj", targets[i]))
            return 1;
    }

    const int expression = 1;
    const std::string number = std::to_string(expression);

    std::ifstream weightFile("data/weights/" + number + ".weights");
    const std::string ppmName = "blended" + number + ".ppm";

    if (!weightFile)
    {
        std::cerr << "Could not open " << number << ".weights\n";
        return 1;
    }
    std::vector<float> weights(targets.size());
    for (std::size_t i = 0; i < weights.size(); ++i)
    {
        if (!(weightFile >> weights[i]))
        {
            std::cerr << "Could not read weight " << i << " from " << number << ".weights\n";
            return 1;
        }
    }

    const std::vector<float> newShape = blendPositions(base, targets, weights);

    std::vector<float> vbuffer;
    std::vector<float> nbuffer;
    for (auto face : base.shapes[0].mesh.indices)
    {
        int vid = face.vertex_index;
        int nid = face.normal_index;

        vbuffer.push_back(newShape[vid * 3]);
        vbuffer.push_back(newShape[vid * 3 + 1]);
        vbuffer.push_back(newShape[vid * 3 + 2]);

        nbuffer.push_back(base.attrib.normals[nid * 3]);
        nbuffer.push_back(base.attrib.normals[nid * 3 + 1]);
        nbuffer.push_back(base.attrib.normals[nid * 3 + 2]);
    }

    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);
#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
#endif

    int windowWidth = 512;
    int windowHeight = 512;
    GLFWwindow *window = glfwCreateWindow(windowWidth, windowHeight, "Assignment02", NULL, NULL);

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

    int framebufferWidth, framebufferHeight;
    glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
    glViewport(0, 0, framebufferWidth, framebufferHeight);

    unsigned int VAO, positionVBO, normalVBO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    glGenBuffers(1, &positionVBO);
    glBindBuffer(GL_ARRAY_BUFFER, positionVBO);
    glBufferData(GL_ARRAY_BUFFER, vbuffer.size() * sizeof(float), vbuffer.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(0);

    glGenBuffers(1, &normalVBO);
    glBindBuffer(GL_ARRAY_BUFFER, normalVBO);
    glBufferData(GL_ARRAY_BUFFER, nbuffer.size() * sizeof(float), nbuffer.data(), GL_STATIC_DRAW);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
    glEnableVertexAttribArray(1);

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

    glm::mat4 projection = glm::perspective(
        glm::radians(30.0f),
        1.0f,
        0.1f,
        1000.0f);

    const glm::mat4 model(1.0f);
    const glm::mat4 view = glm::lookAt(glm::vec3(20, 100, 200), glm::vec3(0, 110, 0), glm::vec3(0, 1, 0));
    glUseProgram(shaderProgram);
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "model"), 1, GL_FALSE, glm::value_ptr(model));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "view"), 1, GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram, "projection"), 1, GL_FALSE, glm::value_ptr(projection));

    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    bool pWasDown = false;
    while (!glfwWindowShouldClose(window))
    {
        if (glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
            glfwSetWindowShouldClose(window, true);

        glClearColor(0.3f, 0.4f, 0.5f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glBindVertexArray(VAO);
        glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(vbuffer.size() / 3));

        const bool pIsDown = glfwGetKey(window, GLFW_KEY_P) == GLFW_PRESS;
        if (pIsDown && !pWasDown)
        {
            glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
            dump_framebuffer_to_ppm(ppmName.c_str(), framebufferWidth, framebufferHeight);
        }
        pWasDown = pIsDown;

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    // Release resources
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &positionVBO);
    glDeleteBuffers(1, &normalVBO);
    glDeleteProgram(shaderProgram);

    glfwTerminate();
    return 0;
}
