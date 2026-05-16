/* * Código integrado para a Atividade Vivencial 1
 * Lê arquivos .OBJ, permite seleção (TAB) e transformações individuais.
 * Inclui a resolução do Desafio (Wireframe preto por cima do objeto selecionado).
 */

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <assert.h>

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
    glm::vec3 posicao;
    glm::vec3 rotacao; // Angulos em graus para X, Y, Z
    glm::vec3 escala;
    glm::vec3 cor;
};

// --- VARIÁVEIS GLOBAIS DA CENA ---
std::vector<Modelo3D> cena;
int objetoSelecionado = 0; // Índice do objeto atual

// Velocidades de transformação
float moveSpeed = 0.05f;
float rotateSpeed = 2.0f;
float scaleSpeed = 0.05f;

// --- DECLARAÇÕES DE FUNÇÕES ---
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
int setupShader();
int loadSimpleOBJ(string filePATH, int &nVertices, glm::vec3 baseColor);

const GLuint WIDTH = 1000, HEIGHT = 1000;

// Shaders básicos
const GLchar* vertexShaderSource = "#version 410\n"
"layout (location = 0) in vec3 position;\n"
"layout (location = 1) in vec3 color;\n"
"uniform mat4 model;\n"
"out vec4 finalColor;\n"
"void main()\n"
"{\n"
"gl_Position = model * vec4(position, 1.0);\n"
"finalColor = vec4(color, 1.0);\n"
"}\0";

const GLchar* fragmentShaderSource = "#version 410\n"
"in vec4 finalColor;\n"
"out vec4 color;\n"
"uniform bool isWireframe;\n" // <--- O nosso interruptor para o desafio
"void main()\n"
"{\n"
"    if(isWireframe)\n"
"        color = vec4(0.0, 0.0, 0.0, 1.0);\n" // Fica Preto
"    else\n"
"        color = finalColor;\n" // Fica com a cor normal
"}\n\0";

int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Atividade Vivencial 1 - Felipe Eduardo Mossmann", nullptr, nullptr);
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
    GLint isWireframeLoc = glGetUniformLocation(shaderID, "isWireframe"); // <--- Localização do interruptor

    // --- CARREGANDO OS MODELOS ---
    Modelo3D obj1;
    obj1.VAO = loadSimpleOBJ("Suzanne.obj", obj1.nVertices, glm::vec3(0.9f, 0.4f, 0.6f)); // Rosa
    obj1.posicao = glm::vec3(-0.6f, 0.0f, 0.0f);
    obj1.rotacao = glm::vec3(0.0f, 0.0f, 0.0f);
    obj1.escala = glm::vec3(0.3f);
    cena.push_back(obj1);

    Modelo3D obj2;
    obj2.VAO = loadSimpleOBJ("Suzanne.obj", obj2.nVertices, glm::vec3(0.2f, 0.8f, 0.8f)); // Ciano
    obj2.posicao = glm::vec3(0.6f, 0.0f, 0.0f);
    obj2.rotacao = glm::vec3(0.0f, 0.0f, 0.0f);
    obj2.escala = glm::vec3(0.3f);
    cena.push_back(obj2);

    // --- LOOP DE RENDERIZAÇÃO ---
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f); // Fundo cinza escuro para destacar as cores
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Renderiza cada objeto da cena
        for (int i = 0; i < cena.size(); i++)
        {
            glm::mat4 model = glm::mat4(1.0f);
            
            // 1. Translação
            model = glm::translate(model, cena[i].posicao);
            
            // 2. Rotação (aplicada nos eixos X, Y e Z individualmente)
            model = glm::rotate(model, glm::radians(cena[i].rotacao.x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(cena[i].rotacao.y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(cena[i].rotacao.z), glm::vec3(0.0f, 0.0f, 1.0f));
            
            // 3. Escala
            model = glm::scale(model, cena[i].escala);

            glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));
            glBindVertexArray(cena[i].VAO);
            
            // 1º Passo: Renderiza o preenchimento normal (colorido)
            glUniform1i(isWireframeLoc, GL_FALSE); // Desliga o preto
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
            glDrawArrays(GL_TRIANGLES, 0, cena[i].nVertices);

            // 2º Passo [DESAFIO]: Renderiza o wireframe preto por cima se for o objeto selecionado
            if (i == objetoSelecionado) {
                glUniform1i(isWireframeLoc, GL_TRUE); // Liga o preto
                glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
                
                // Um pequeno truque: escalar minimamente o wireframe para não dar "z-fighting" com o preenchimento
                glm::mat4 modelWire = glm::scale(model, glm::vec3(1.01f)); 
                glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(modelWire));
                
                glLineWidth(2.0f); // Deixa a linha preta um pouco mais grossa
                glDrawArrays(GL_TRIANGLES, 0, cena[i].nVertices);
                
                // Retorna ao modo normal para não afetar outras renderizações
                glPolygonMode(GL_FRONT_AND_BACK, GL_FILL); 
                glUniform1i(isWireframeLoc, GL_FALSE); 
            }
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

    // Selecionar próximo objeto (Tecla TAB)
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
    {
        objetoSelecionado = (objetoSelecionado + 1) % cena.size();
        std::cout << "Objeto selecionado: " << objetoSelecionado << std::endl;
    }

    // A partir daqui, as transformações se aplicam apenas ao objeto selecionado
    if (action == GLFW_PRESS || action == GLFW_REPEAT)
    {
        // Translação (W, A, S, D, Q, E)
        if (key == GLFW_KEY_A) cena[objetoSelecionado].posicao.x -= moveSpeed;
        if (key == GLFW_KEY_D) cena[objetoSelecionado].posicao.x += moveSpeed;
        if (key == GLFW_KEY_W) cena[objetoSelecionado].posicao.y += moveSpeed;
        if (key == GLFW_KEY_S) cena[objetoSelecionado].posicao.y -= moveSpeed;
        if (key == GLFW_KEY_Q) cena[objetoSelecionado].posicao.z += moveSpeed;
        if (key == GLFW_KEY_E) cena[objetoSelecionado].posicao.z -= moveSpeed;

        // Rotação (Setas direccionais + Z/C para o eixo Z)
        if (key == GLFW_KEY_UP)    cena[objetoSelecionado].rotacao.x -= rotateSpeed;
        if (key == GLFW_KEY_DOWN)  cena[objetoSelecionado].rotacao.x += rotateSpeed;
        if (key == GLFW_KEY_LEFT)  cena[objetoSelecionado].rotacao.y -= rotateSpeed;
        if (key == GLFW_KEY_RIGHT) cena[objetoSelecionado].rotacao.y += rotateSpeed;
        if (key == GLFW_KEY_Z)     cena[objetoSelecionado].rotacao.z -= rotateSpeed;
        if (key == GLFW_KEY_C)     cena[objetoSelecionado].rotacao.z += rotateSpeed;

        // Escala ([ e ])
        if (key == GLFW_KEY_LEFT_BRACKET) {
            cena[objetoSelecionado].escala -= glm::vec3(scaleSpeed);
            if (cena[objetoSelecionado].escala.x < 0.05f) cena[objetoSelecionado].escala = glm::vec3(0.05f);
        }
        if (key == GLFW_KEY_RIGHT_BRACKET) {
            cena[objetoSelecionado].escala += glm::vec3(scaleSpeed);
        }
    }
}

// --- FUNÇÕES AUXILIARES ---
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

// Função da professora com adição da variável baseColor no parâmetro
int loadSimpleOBJ(string filePATH, int &nVertices, glm::vec3 baseColor)
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords;
    std::vector<glm::vec3> normals;
    std::vector<GLfloat> vBuffer;
    
    // Agora a cor vem do parâmetro!
    glm::vec3 color = baseColor; 

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

        if (word == "v") 
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
                
                // Usando a cor baseada no parâmetro
                vBuffer.push_back(color.r);
                vBuffer.push_back(color.g);
                vBuffer.push_back(color.b);
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
    
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
    glEnableVertexAttribArray(0);

    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
    glEnableVertexAttribArray(1);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0);
    glBindVertexArray(0);

    nVertices = vBuffer.size() / 6;

    return VAO;
}