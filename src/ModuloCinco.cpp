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
#include <assert.h>

using namespace std;

// Instancia a biblioteca de imagens
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// GLAD e GLFW
#include <glad/glad.h>
#include <GLFW/glfw3.h>

// GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// =========================================================================
// CLASSE CÂMERA (Orientada a Objetos - Módulo 5)
// =========================================================================
enum Camera_Movement { FORWARD, BACKWARD, LEFT, RIGHT };

class Camera {
public:
    // Atributos da Câmera
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 Up;
    glm::vec3 Right;
    glm::vec3 WorldUp;
    
    // Ângulos de Euler
    float Yaw;
    float Pitch;
    
    // Opções
    float MovementSpeed;
    float MouseSensitivity;
    float Zoom;

    // Construtor
    Camera(glm::vec3 position = glm::vec3(0.0f, 0.0f, 3.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = -90.0f, float pitch = 0.0f) 
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(2.5f), MouseSensitivity(0.1f), Zoom(45.0f) {
        Position = position;
        WorldUp = up;
        Yaw = yaw;
        Pitch = pitch;
        updateCameraVectors();
    }

    // Retorna a matriz de View usando LookAt
    glm::mat4 GetViewMatrix() {
        return glm::lookAt(Position, Position + Front, Up);
    }

    // Move a câmera baseado no teclado
    void ProcessKeyboard(Camera_Movement direction, float deltaTime) {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)  Position += Front * velocity;
        if (direction == BACKWARD) Position -= Front * velocity;
        if (direction == LEFT)     Position -= Right * velocity;
        if (direction == RIGHT)    Position += Right * velocity;
    }

    // Rotaciona a visão baseado no mouse
    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true) {
        xoffset *= MouseSensitivity;
        yoffset *= MouseSensitivity;

        Yaw   += xoffset;
        Pitch += yoffset;

        // Trava o pescoço para não dar cambalhotas
        if (constrainPitch) {
            if (Pitch > 89.0f)  Pitch = 89.0f;
            if (Pitch < -89.0f) Pitch = -89.0f;
        }
        updateCameraVectors();
    }

    // Zoom com o Scroll
    void ProcessMouseScroll(float yoffset) {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f)  Zoom = 1.0f;
        if (Zoom > 45.0f) Zoom = 45.0f;
    }

private:
    void updateCameraVectors() {
        glm::vec3 front;
        front.x = cos(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        front.y = sin(glm::radians(Pitch));
        front.z = sin(glm::radians(Yaw)) * cos(glm::radians(Pitch));
        Front = glm::normalize(front);
        Right = glm::normalize(glm::cross(Front, WorldUp));
        Up    = glm::normalize(glm::cross(Right, Front));
    }
};

// =========================================================================
// VARIÁVEIS GLOBAIS E ESTRUTURAS
// =========================================================================
const GLuint WIDTH = 1000, HEIGHT = 1000;

// Câmera Global
Camera camera(glm::vec3(0.0f, 0.0f, 4.0f));
float lastX = WIDTH / 2.0f;
float lastY = HEIGHT / 2.0f;
bool firstMouse = true;

// Temporizadores para movimento suave
float deltaTime = 0.0f; 
float lastFrame = 0.0f;

struct Material {
    glm::vec3 Ka = glm::vec3(0.1f);
    glm::vec3 Kd = glm::vec3(0.5f);
    glm::vec3 Ks = glm::vec3(0.5f);
    float Ns = 32.0f; 
};

struct Modelo3D {
    GLuint VAO;
    int nVertices;
    GLuint texID;
    Material material; // Será lido do .mtl, mas vamos ignorar o brilho para evitar o estouro
    glm::vec3 posicao;
    glm::vec3 rotacao; 
    glm::vec3 escala;
    glm::vec3 cor;
};

std::vector<Modelo3D> cena;
int objetoSelecionado = 0; 
float rotateSpeed = 2.0f;
float scaleSpeed = 0.05f;

// Controle das Luzes 
bool keyLightOn = true;   
bool fillLightOn = true;  
bool backLightOn = true;  

// --- DECLARAÇÕES DE FUNÇÕES ---
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window); // Processa segurar teclas
int setupShader();
GLuint loadTexture(string filePath);
int loadSimpleOBJ(string filePATH, int &nVertices, glm::vec3 baseColor, GLuint &texID, Material &mat);

// =========================================================================
// SHADERS
// =========================================================================
const GLchar* vertexShaderSource = R"(
#version 410
layout (location = 0) in vec3 position;
layout (location = 1) in vec3 color;
layout (location = 2) in vec3 normal;
layout (location = 3) in vec2 tex_coord;

uniform mat4 projection;
uniform mat4 view;
uniform mat4 model;

out vec2 texCoord;
out vec3 vNormal;
out vec4 fragPos; 
out vec4 vColor;

void main()
{
    gl_Position = projection * view * model * vec4(position, 1.0);
    fragPos = model * vec4(position, 1.0);
    mat3 normalMatrix = transpose(inverse(mat3(model)));
    vNormal = normalMatrix * normal;
    texCoord = vec2(tex_coord.x, tex_coord.y);
    vColor = vec4(color, 1.0);
}
)";

const GLchar* fragmentShaderSource = R"(
#version 410
in vec2 texCoord;
in vec4 fragPos;
in vec3 vNormal;
in vec4 vColor;

out vec4 color;

uniform sampler2D tex_buffer;
uniform vec3 camPos;

// Propriedades de Material (Recebidas da aplicação)
uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float q;

struct Light { vec3 position; vec3 color; bool enabled; };
uniform Light lights[3];

void main()
{
    vec3 N = normalize(vNormal);
    vec3 V = normalize(camPos - vec3(fragPos));
    
    vec4 objectColor = texture(tex_buffer, texCoord);
    if(objectColor.a < 0.1) objectColor = vColor; 

    vec3 finalLight = vec3(0.0);

    for(int i = 0; i < 3; i++) {
        if(!lights[i].enabled) continue;

        vec3 L = normalize(lights[i].position - vec3(fragPos));
        float dist = length(lights[i].position - vec3(fragPos));
        float attenuation = 1.0 / (1.0 + 0.09 * dist + 0.032 * (dist * dist));

        vec3 ambient = ka * lights[i].color;
        float diff = max(dot(N, L), 0.0);
        vec3 diffuse = kd * diff * lights[i].color * attenuation;
        
        vec3 R = normalize(reflect(-L, N));
        float spec = max(dot(R, V), 0.0);
        spec = pow(spec, max(q, 1.0));
        vec3 specular = ks * spec * lights[i].color; 

        finalLight += (ambient + diffuse) * vec3(objectColor) + specular;
    }
    color = vec4(finalLight, 1.0);
}
)";

// =========================================================================
// FUNÇÃO MAIN
// =========================================================================
int main()
{
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Modulo 5 - Felipe Mossmann", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    
    // Callbacks de Input
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);

    // Oculta e captura o cursor do mouse na janela
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    glViewport(0, 0, WIDTH, HEIGHT);
    glEnable(GL_DEPTH_TEST);

    GLuint shaderID = setupShader();
    glUseProgram(shaderID);
    
    // --- CARREGANDO OS MODELOS ---
    Modelo3D obj1;
    obj1.VAO = loadSimpleOBJ("Suzanne.obj", obj1.nVertices, glm::vec3(1.0f, 1.0f, 1.0f), obj1.texID, obj1.material); 
    obj1.posicao = glm::vec3(-1.0f, 0.0f, 0.0f);
    obj1.rotacao = glm::vec3(0.0f, 0.0f, 0.0f);
    obj1.escala = glm::vec3(0.4f);
    cena.push_back(obj1);

    Modelo3D obj2;
    obj2.VAO = loadSimpleOBJ("Suzanne.obj", obj2.nVertices, glm::vec3(1.0f, 1.0f, 1.0f), obj2.texID, obj2.material); 
    obj2.posicao = glm::vec3(1.0f, 0.0f, 0.0f);
    obj2.rotacao = glm::vec3(0.0f, 0.0f, 0.0f);
    obj2.escala = glm::vec3(0.4f);
    cena.push_back(obj2);

    glUniform1i(glGetUniformLocation(shaderID, "tex_buffer"), 0);

    // --- PROPRIEDADES FIXAS DO MATERIAL (Evita estouro da luz do .mtl) ---
    glm::vec3 fixKa = glm::vec3(0.1f);
    glm::vec3 fixKd = glm::vec3(0.6f);
    glm::vec3 fixKs = glm::vec3(0.5f);
    float fixQ = 32.0f;

    // --- LOOP DE RENDERIZAÇÃO ---
    while (!glfwWindowShouldClose(window))
    {
        // Lógica de tempo (DeltaTime) para a câmera se mover suavemente independente do FPS
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window); // Processa as teclas que ficam pressionadas (WASD)

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // --- CÂMERA MATRIZES ---
        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH/(float)HEIGHT, 0.1f, 100.0f);
        glm::mat4 view = camera.GetViewMatrix();
        
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(glGetUniformLocation(shaderID, "camPos"), 1, glm::value_ptr(camera.Position));

        // Luzes acompanham o objeto selecionado
        glm::vec3 basePos = cena[objetoSelecionado].posicao;

        // 1. Key Light (Intensidade ajustada para 0.8)
        glm::vec3 keyPos = basePos + glm::vec3(2.0f, 2.0f, 2.0f);
        glUniform3fv(glGetUniformLocation(shaderID, "lights[0].position"), 1, glm::value_ptr(keyPos));
        glUniform3f(glGetUniformLocation(shaderID, "lights[0].color"), 0.8f, 0.8f, 0.8f);
        glUniform1i(glGetUniformLocation(shaderID, "lights[0].enabled"), keyLightOn);

        // 2. Fill Light 
        glm::vec3 fillPos = basePos + glm::vec3(-2.0f, 1.0f, 2.0f);
        glUniform3fv(glGetUniformLocation(shaderID, "lights[1].position"), 1, glm::value_ptr(fillPos));
        glUniform3f(glGetUniformLocation(shaderID, "lights[1].color"), 0.3f, 0.3f, 0.4f); 
        glUniform1i(glGetUniformLocation(shaderID, "lights[1].enabled"), fillLightOn);

        // 3. Back Light 
        glm::vec3 backPos = basePos + glm::vec3(0.0f, 2.0f, -3.0f);
        glUniform3fv(glGetUniformLocation(shaderID, "lights[2].position"), 1, glm::value_ptr(backPos));
        glUniform3f(glGetUniformLocation(shaderID, "lights[2].color"), 0.6f, 0.6f, 0.6f);
        glUniform1i(glGetUniformLocation(shaderID, "lights[2].enabled"), backLightOn);

        for (int i = 0; i < cena.size(); i++)
        {
            // Substitui os valores instáveis lidos do .mtl pelas variáveis fixas perfeitas
            glUniform3fv(glGetUniformLocation(shaderID, "ka"), 1, glm::value_ptr(fixKa));
            glUniform3fv(glGetUniformLocation(shaderID, "kd"), 1, glm::value_ptr(fixKd));
            glUniform3fv(glGetUniformLocation(shaderID, "ks"), 1, glm::value_ptr(fixKs));
            glUniform1f(glGetUniformLocation(shaderID, "q"), fixQ);

            glm::mat4 model = glm::mat4(1.0f);
            model = glm::translate(model, cena[i].posicao);
            model = glm::rotate(model, glm::radians(cena[i].rotacao.x), glm::vec3(1.0f, 0.0f, 0.0f));
            model = glm::rotate(model, glm::radians(cena[i].rotacao.y), glm::vec3(0.0f, 1.0f, 0.0f));
            model = glm::rotate(model, glm::radians(cena[i].rotacao.z), glm::vec3(0.0f, 0.0f, 1.0f));
            model = glm::scale(model, cena[i].escala);

            glUniformMatrix4fv(glGetUniformLocation(shaderID, "model"), 1, GL_FALSE, glm::value_ptr(model));
            
            glActiveTexture(GL_TEXTURE0);
            glBindTexture(GL_TEXTURE_2D, cena[i].texID);

            glBindVertexArray(cena[i].VAO);
            glDrawArrays(GL_TRIANGLES, 0, cena[i].nVertices);
            glBindVertexArray(0);
        }

        glfwSwapBuffers(window);
        glfwPollEvents();
    }

    glfwTerminate();
    return 0;
}

// =========================================================================
// CONTROLES E FUNÇÕES AUXILIARES
// =========================================================================

// Teclas mantidas pressionadas (Câmera e Rotação do Objeto)
void processInput(GLFWwindow *window)
{
    // Câmera WASD
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT, deltaTime);

    // Rotação do Objeto Selecionado (Setas e ZX)
    if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    cena[objetoSelecionado].rotacao.x -= rotateSpeed;
    if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  cena[objetoSelecionado].rotacao.x += rotateSpeed;
    if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  cena[objetoSelecionado].rotacao.y -= rotateSpeed;
    if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) cena[objetoSelecionado].rotacao.y += rotateSpeed;
    if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)     cena[objetoSelecionado].rotacao.z -= rotateSpeed;
    if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)     cena[objetoSelecionado].rotacao.z += rotateSpeed;

    // Escala do Objeto Selecionado (Colchetes)
    if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) {
        cena[objetoSelecionado].escala -= glm::vec3(scaleSpeed);
        if (cena[objetoSelecionado].escala.x < 0.05f) cena[objetoSelecionado].escala = glm::vec3(0.05f);
    }
    if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) cena[objetoSelecionado].escala += glm::vec3(scaleSpeed);
}

// Teclas de apertar uma vez só (Luzes, Sair, Alternar Objeto)
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GL_TRUE);

    if (key == GLFW_KEY_TAB && action == GLFW_PRESS) objetoSelecionado = (objetoSelecionado + 1) % cena.size();

    if (key == GLFW_KEY_1 && action == GLFW_PRESS) keyLightOn = !keyLightOn;
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) fillLightOn = !fillLightOn;
    if (key == GLFW_KEY_3 && action == GLFW_PRESS) backLightOn = !backLightOn;
}

// Controle do Mouse para olhar ao redor
void mouse_callback(GLFWwindow* window, double xposIn, double yposIn)
{
    float xpos = static_cast<float>(xposIn);
    float ypos = static_cast<float>(yposIn);

    if (firstMouse) {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    float xoffset = xpos - lastX;
    float yoffset = lastY - ypos; // Invertido, pois o Y cresce de cima para baixo
    lastX = xpos;
    lastY = ypos;

    camera.ProcessMouseMovement(xoffset, yoffset);
}

// Controle do Scroll para dar Zoom
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset)
{
    camera.ProcessMouseScroll(static_cast<float>(yoffset));
}

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
    stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0); 

    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data); 
        glGenerateMipmap(GL_TEXTURE_2D); 
    } else {
        std::cout << "Falha ao carregar a textura: " << filePath << std::endl; 
        return 0;
    }
    stbi_image_free(data); 
    glBindTexture(GL_TEXTURE_2D, 0); 
    return texID;
}

int loadSimpleOBJ(string filePATH, int &nVertices, glm::vec3 baseColor, GLuint &texID, Material &mat)
{
    std::vector<glm::vec3> vertices;
    std::vector<glm::vec2> texCoords; 
    std::vector<glm::vec3> normals;
    std::vector<GLfloat> vBuffer;
    
    texID = 0; 
    string diretorio = "";
    size_t lastSlash = filePATH.find_last_of('/');
    if (lastSlash != string::npos) diretorio = filePATH.substr(0, lastSlash + 1);

    std::ifstream arqEntrada(filePATH.c_str());
    if (!arqEntrada.is_open()) return -1;

    std::string line;
    while (std::getline(arqEntrada, line)) 
    {
        std::istringstream ssline(line);
        std::string word;
        ssline >> word;

        if (word == "mtllib") {
            string mtlFile; ssline >> mtlFile;
            std::ifstream mtlArq((diretorio + mtlFile).c_str());
            if (mtlArq.is_open()) {
                string mLine, mWord;
                while (getline(mtlArq, mLine)) {
                    std::istringstream mss(mLine); mss >> mWord;
                    if (mWord == "map_Kd") { 
                        string texName; mss >> texName; texID = loadTexture(diretorio + texName); 
                    } else if (mWord == "Ka") { mss >> mat.Ka.r >> mat.Ka.g >> mat.Ka.b;
                    } else if (mWord == "Kd") { mss >> mat.Kd.r >> mat.Kd.g >> mat.Kd.b;
                    } else if (mWord == "Ks") { mss >> mat.Ks.r >> mat.Ks.g >> mat.Ks.b;
                    } else if (mWord == "Ns") { mss >> mat.Ns; }
                } mtlArq.close();
            }
        } else if (word == "v") { glm::vec3 vertice; ssline >> vertice.x >> vertice.y >> vertice.z; vertices.push_back(vertice);
        } else if (word == "vt") { glm::vec2 vt; ssline >> vt.s >> vt.t; texCoords.push_back(vt);
        } else if (word == "vn") { glm::vec3 normal; ssline >> normal.x >> normal.y >> normal.z; normals.push_back(normal);
        } else if (word == "f") {
            while (ssline >> word) {
                int vi = 0, ti = 0, ni = 0;
                std::istringstream ss(word); std::string index;
                if (std::getline(ss, index, '/')) vi = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index, '/')) ti = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index)) ni = !index.empty() ? std::stoi(index) - 1 : 0;

                vBuffer.push_back(vertices[vi].x); vBuffer.push_back(vertices[vi].y); vBuffer.push_back(vertices[vi].z);
                vBuffer.push_back(baseColor.r); vBuffer.push_back(baseColor.g); vBuffer.push_back(baseColor.b);
                if (!normals.empty()) { vBuffer.push_back(normals[ni].x); vBuffer.push_back(normals[ni].y); vBuffer.push_back(normals[ni].z); } 
                else { vBuffer.push_back(0.0f); vBuffer.push_back(0.0f); vBuffer.push_back(1.0f); }
                if (!texCoords.empty() && ti >= 0) { vBuffer.push_back(texCoords[ti].s); vBuffer.push_back(texCoords[ti].t); } 
                else { vBuffer.push_back(0.0f); vBuffer.push_back(0.0f); }
            }
        }
    }
    arqEntrada.close();

    GLuint VBO, VAO;
    glGenBuffers(1, &VBO); glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);
    glGenVertexArrays(1, &VAO); glBindVertexArray(VAO);
    
    int stride = 11 * sizeof(GLfloat);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(3 * sizeof(GLfloat))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(6 * sizeof(GLfloat))); glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(9 * sizeof(GLfloat))); glEnableVertexAttribArray(3);
    
    glBindBuffer(GL_ARRAY_BUFFER, 0); glBindVertexArray(0);
    nVertices = vBuffer.size() / 11; 
    return VAO;
}