/* Hello Triangle - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para as disciplinas de Processamento Gráfico/Computação Gráfica - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização em 07/03/2025
 */

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>

// Instancia a biblioteca de imagens
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

using namespace std;

// GLAD
#include <glad/glad.h>
// GLFW
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// --- ESTRUTURA DO OBJETO 3D ---
struct Modelo3D {
    GLuint VAO;
    int nVertices;
    GLuint texID; // Guarda a textura específica deste objeto
    glm::vec3 posicao;
    glm::vec3 rotacao; // Angulos em graus para X, Y, Z
    glm::vec3 escala;
    glm::vec3 cor;
};

// --- VARIÁVEIS GLOBAIS DA CENA ---
std::vector<Modelo3D> cena;
int objetoSelecionado = 0; 

// Velocidades de transformação
float moveSpeed = 0.05f;
float rotateSpeed = 2.0f;
float scaleSpeed = 0.05f;

// --- DECLARAÇÕES DE FUNÇÕES ---
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
int setupShader();
GLuint loadTexture(string filePath);
int loadSimpleOBJ(string filePATH, int &nVertices, glm::vec3 baseColor, GLuint &texID);

const GLuint WIDTH = 1000, HEIGHT = 1000;

// --- SHADERS ATUALIZADOS PARA TEXTURA  ---
const GLchar* vertexShaderSource = "#version 410\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec3 color;\n"
"layout (location = 2) in vec2 tex_coord;\n"
"uniform mat4 model;\n"
"out vec4 finalColor;\n"
"out vec2 texCoord;\n"
"void main()\n"
"{\n"
"    gl_Position = model * vec4(position, 1.0);\n"
"    finalColor = vec4(color, 1.0);\n"
"    texCoord = vec2(tex_coord.x, tex_coord.y);\n"
"}\0";

const GLchar* fragmentShaderSource = "#version 410\n"
"in vec4 finalColor;\n"
"in vec2 texCoord;\n"
"out vec4 color;\n"
"uniform sampler2D tex_buffer;\n"
"void main()\n"
"{\n"
"    // Agora apenas aplica a textura multiplicada pela cor base\n"
"    color = texture(tex_buffer, texCoord) * finalColor;\n"
"}\n\0";

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Modulo 3 Texturas - Felipe Eduardo Mossmann", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    glfwSetKeyCallback(window, key_callback);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    glViewport(0, 0, WIDTH, HEIGHT);
    glEnable(GL_DEPTH_TEST);

    GLuint shaderID = setupShader();
    glUseProgram(shaderID);
    
    GLint modelLoc = glGetUniformLocation(shaderID, "model");
    
    // Avisa o shader que a textura vai estar na unidade 0
    glUniform1i(glGetUniformLocation(shaderID, "tex_buffer"), 0);

    // --- CARREGANDO OS MODELOS ---
    Modelo3D obj1;
    obj1.VAO = loadSimpleOBJ("Suzanne.obj", obj1.nVertices, glm::vec3(1.0f, 1.0f, 1.0f), obj1.texID); 
    obj1.posicao = glm::vec3(-0.6f, 0.0f, 0.0f);
    obj1.rotacao = glm::vec3(0.0f, 0.0f, 0.0f);
    obj1.escala = glm::vec3(0.3f);
    cena.push_back(obj1);

    // Carregando o segundo objeto também como Suzanne para não dar problema de arquivo vazio
    Modelo3D obj2;
    obj2.VAO = loadSimpleOBJ("Suzanne.obj", obj2.nVertices, glm::vec3(1.0f, 1.0f, 1.0f), obj2.texID); 
    obj2.posicao = glm::vec3(0.6f, 0.0f, 0.0f);
    obj2.rotacao = glm::vec3(0.0f, 0.0f, 0.0f);
    obj2.escala = glm::vec3(0.3f);
    cena.push_back(obj2);

    // --- LOOP DE RENDERIZAÇÃO ---
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Garante que desenharemos preenchido
        glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        for (int i = 0; i < cena.size(); i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            
            // Transformações
            model = glm::translate(model, cena[i].posicao);
            model = glm::rotate(model, glm::radians(cena[i].rotacao.x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(cena[i].rotacao.y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(cena[i].rotacao.z), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, cena[i].escala);

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            
            // ATIVA A TEXTURA DO OBJETO ATUAL ANTES DE DESENHAR
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cena[i].texID);

            glBindVertexArray(cena[i].VAO);
            glDrawArrays(GL_TRIANGLES, 0, cena[i].nVertices);
            
            glBindTexture(GL_TEXTURE_2D, 0); // Desconecta a textura por segurança
        }

        glBindVertexArray(0);
        glfwSwapBuffers(window);
    }

    glfwTerminate();
    return 0;
}

// --- CONTROLES DO TECLADO ---
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
    {
        objetoSelecionado = (objetoSelecionado + 1) % cena.size();
    }

    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        if (key == GLFW_KEY_A) cena[objetoSelecionado].posicao.x -= moveSpeed;
        if (key == GLFW_KEY_D) cena[objetoSelecionado].posicao.x += moveSpeed;
        if (key == GLFW_KEY_W) cena[objetoSelecionado].posicao.y += moveSpeed;
        if (key == GLFW_KEY_S) cena[objetoSelecionado].posicao.y -= moveSpeed;
        if (key == GLFW_KEY_Q) cena[objetoSelecionado].posicao.z += moveSpeed;
        if (key == GLFW_KEY_E) cena[objetoSelecionado].posicao.z -= moveSpeed;

        if (key == GLFW_KEY_UP)    cena[objetoSelecionado].rotacao.x -= rotateSpeed;
        if (key == GLFW_KEY_DOWN)  cena[objetoSelecionado].rotacao.x += rotateSpeed;
        if (key == GLFW_KEY_LEFT)  cena[objetoSelecionado].rotacao.y -= rotateSpeed;
        if (key == GLFW_KEY_RIGHT) cena[objetoSelecionado].rotacao.y += rotateSpeed;
        if (key == GLFW_KEY_Z)     cena[objetoSelecionado].rotacao.z -= rotateSpeed;
        if (key == GLFW_KEY_C)     cena[objetoSelecionado].rotacao.z += rotateSpeed;

        if (key == GLFW_KEY_LEFT_BRACKET) {
            cena[objetoSelecionado].escala -= glm::vec3(scaleSpeed);
            if (cena[objetoSelecionado].escala.x < 0.05f) cena[objetoSelecionado].escala = glm::vec3(0.05f);
        }
        if (key == GLFW_KEY_RIGHT_BRACKET) {
            cena[objetoSelecionado].escala += glm::vec3(scaleSpeed);
        }
    }
}

// --- FUNÇÃO PARA CRIAR OS SHADERS ---
int setupShader()
{
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
    glCompileShader(vertexShader);

    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
    glCompileShader(fragmentShader);

    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader);
    glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);

    glDeleteShader(vertexShader);
    glDeleteShader(fragmentShader);

    return shaderProgram;
}

// --- FUNÇÃO QUE CARREGA A IMAGEM NA PLACA DE VÍDEO ---
GLuint loadTexture(string filePath)
{
    GLuint texID;
    glGenTextures(1, &texID); 
    glBindTexture(GL_TEXTURE_2D, texID); 

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 

    int width, height, nrChannels;
    
    // Corrige a textura invertida!
    stbi_set_flip_vertically_on_load(true);

    unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0); 

    if (data) 
    {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data); 
        glGenerateMipmap(GL_TEXTURE_2D); 
    }
    else 
    {
        std::cout << "Falha ao carregar a textura: " << filePath << std::endl; 
    }

    stbi_image_free(data); 
    glBindTexture(GL_TEXTURE_2D, 0); 

    return texID;
}

// --- LEITOR DE OBJETO ATUALIZADO ---
int loadSimpleOBJ(string filePATH, int &nVertices, glm::vec3 baseColor, GLuint &texID)
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords; 
    std::vector<glm::vec3> normals;
    std::vector<GLfloat> vBuffer;
    
    glm::vec3 color = baseColor; 
    texID = 0; // Inicializa com 0 para evitar bugs caso não encontre textura

    string diretorio = "";
    size_t lastSlash = filePATH.find_last_of('/');
    if (lastSlash != string::npos) {
        diretorio = filePATH.substr(0, lastSlash + 1);
    }

    std::ifstream arqEntrada(filePATH.c_str());
    if (!arqEntrada.is_open()) 
    {
        std::cerr << "Erro ao tentar ler o arquivo " << filePATH << std::endl;
        return -1;
    }

    std::string line;
    while (std::getline(arqEntrada, line)) 
    {
        std::istringstream ssline(line);
        std::string word;
        ssline >> word;

        if (word == "mtllib") {
            string mtlFile;
            ssline >> mtlFile;
            std::ifstream mtlArq((diretorio + mtlFile).c_str());
            
            if (mtlArq.is_open()) {
                string mLine, mWord;
                while (getline(mtlArq, mLine)) {
                    std::istringstream mss(mLine);
                    mss >> mWord;
                    if (mWord == "map_Kd") { 
                        string texName;
                        mss >> texName;
                        texID = loadTexture(diretorio + texName); 
                        break;
                    }
                }
                mtlArq.close();
            } else {
                std::cout << "Aviso: Nao foi possivel abrir MTL: " << (diretorio + mtlFile) << std::endl;
            }
        }
        else if (word == "v") 
        {
            glm::vec3 vertice;
            ssline >> vertice.x >> vertice.y >> vertice.z;
            vertices.push_back(vertice);
        } 
        else if (word == "vt") 
        {
            glm::vec2 vt;
            ssline >> vt.s >> vt.t;
            texCoords.push_back(vt);
        } 
        else if (word == "vn") 
        {
            glm::vec3 normal;
            ssline >> normal.x >> normal.y >> normal.z;
            normals.push_back(normal);
        } 
        else if (word == "f")
         {
            while (ssline >> word) 
            {
                int vi = 0, ti = 0, ni = 0;
                std::istringstream ss(word);
                std::string index;

                if (std::getline(ss, index, '/')) vi = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index, '/')) ti = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index)) ni = !index.empty() ? std::stoi(index) - 1 : 0;

                vBuffer.push_back(vertices[vi].x);
                vBuffer.push_back(vertices[vi].y);
                vBuffer.push_back(vertices[vi].z);
                
                vBuffer.push_back(color.r);
                vBuffer.push_back(color.g);
                vBuffer.push_back(color.b);
                
                if (!texCoords.empty() && ti >= 0) {
                    vBuffer.push_back(texCoords[ti].s);
                    vBuffer.push_back(texCoords[ti].t);
                } else {
                    vBuffer.push_back(0.0f);
                    vBuffer.push_back(0.0f);
                }
            }
        }
    }

    arqEntrada.close();

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO);
    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);
    
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);
    
    int stride = 8 * sizeof(GLfloat);
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);

    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(6 * sizeof(GLfloat)));
    glEnableVertexAttribArray(2);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = vBuffer.size() / 8; 

    return VAO;
}