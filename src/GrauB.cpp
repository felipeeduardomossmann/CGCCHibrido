/* Código Final - Avaliação (Diorama)
 Câmera FPS, Iluminação 3 Pontos (Phong), Curvas de Bézier, 
 Leitor de Cena Externo, Editor Ao Vivo e Highlight de Seleção.
 */

#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <assert.h>

using namespace std;

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

// =========================================================================
// CLASSE CÂMERA (FPS)
// =========================================================================
enum Camera_Movement { FORWARD, BACKWARD, LEFT, RIGHT };

class Camera {
public:
    glm::vec3 Position, Front, Up, Right, WorldUp;
    float Yaw, Pitch, MovementSpeed, MouseSensitivity, Zoom;

    Camera(glm::vec3 position = glm::vec3(0.0f, 1.0f, 4.0f), glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f), float yaw = -90.0f, float pitch = 0.0f) 
    : Front(glm::vec3(0.0f, 0.0f, -1.0f)), MovementSpeed(2.5f), MouseSensitivity(0.1f), Zoom(45.0f) {
        Position = position; WorldUp = up; Yaw = yaw; Pitch = pitch;
        updateCameraVectors();
    }

    glm::mat4 GetViewMatrix() { return glm::lookAt(Position, Position + Front, Up); }

    void ProcessKeyboard(Camera_Movement direction, float deltaTime) {
        float velocity = MovementSpeed * deltaTime;
        if (direction == FORWARD)  Position += Front * velocity;
        if (direction == BACKWARD) Position -= Front * velocity;
        if (direction == LEFT)     Position -= Right * velocity;
        if (direction == RIGHT)    Position += Right * velocity;
    }

    void ProcessMouseMovement(float xoffset, float yoffset, GLboolean constrainPitch = true) {
        xoffset *= MouseSensitivity; yoffset *= MouseSensitivity;
        Yaw += xoffset; Pitch += yoffset;
        if (constrainPitch) {
            if (Pitch > 89.0f) Pitch = 89.0f;
            if (Pitch < -89.0f) Pitch = -89.0f;
        }
        updateCameraVectors();
    }

    void ProcessMouseScroll(float yoffset) {
        Zoom -= (float)yoffset;
        if (Zoom < 1.0f) Zoom = 1.0f;
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
Camera camera;
float lastX = WIDTH / 2.0f, lastY = HEIGHT / 2.0f;
bool firstMouse = true;
float deltaTime = 0.0f, lastFrame = 0.0f;

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
    Material material;
    std::string caminhoArquivo;
    glm::vec3 posicao;
    glm::vec3 rotacao = glm::vec3(0.0f); 
    glm::vec3 escala = glm::vec3(1.0f);
    
    std::vector<glm::vec3> pontosDeControle;
    bool animando = false;
    float velocidadeAnimacao = 0.3f; 
};

std::vector<Modelo3D> cena;
int objetoSelecionado = 0; 
float rotateSpeed = 2.0f, scaleSpeed = 0.5f, moveSpeed = 0.005f;

// Flags de Apresentação
bool keyLightOn = true, fillLightOn = true, backLightOn = true;  
bool usaTextura = true; 
bool mostraSelecao = true;

// --- DECLARAÇÕES DE FUNÇÕES ---
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void scroll_callback(GLFWwindow* window, double xoffset, double yoffset);
void processInput(GLFWwindow *window); 
int setupShader();
GLuint loadTexture(string filePath);
int loadSimpleOBJ(string filePATH, int &nVertices, glm::vec3 baseColor, GLuint &texID, Material &mat);
void carregarCena(string filepath);
void salvarCena();
glm::vec3 calculaBezier(std::vector<glm::vec3> pontos, float t);

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

void main() {
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
uniform bool useTexture; 
uniform bool isSelected; 

uniform vec3 ka;
uniform vec3 kd;
uniform vec3 ks;
uniform float q;

struct Light { vec3 position; vec3 color; bool enabled; };
uniform Light lights[3];

void main() {
    vec3 N = normalize(vNormal);
    vec3 V = normalize(camPos - vec3(fragPos));

    vec4 objectColor;
    if (texCoord.x < -0.5) {
        objectColor = vColor;
    } else {
        objectColor = useTexture ? texture(tex_buffer, texCoord) : vec4(kd, 1.0);
        if(objectColor.r < 0.05 && objectColor.g < 0.05 && objectColor.b < 0.05) {
            objectColor = vec4(0.6, 0.6, 0.6, 1.0);
        }
    }

    vec3 finalLight = vec3(0.1) * vec3(objectColor);

    for(int i = 0; i < 3; i++) {
        if(!lights[i].enabled) continue;

        vec3 L = normalize(lights[i].position - vec3(fragPos));
        
        vec3 ambient = ka * lights[i].color;
        float diff = max(dot(N, L), 0.0);
        
        vec3 diffuseColor = (kd.r < 0.05 && kd.g < 0.05 && kd.b < 0.05) ? vec3(1.0) : kd;
        vec3 diffuse = diffuseColor * diff * lights[i].color;
        
        vec3 R = normalize(reflect(-L, N));
        float spec = max(dot(R, V), 0.0);
        spec = pow(spec, max(q, 1.0));
        vec3 specular = ks * spec * lights[i].color; 

        finalLight += (ambient + diffuse) * vec3(objectColor) + specular;
    }
    
    if (isSelected) {
        finalLight += vec3(0.3, 0.0, 0.0); 
    }

    color = vec4(finalLight, 1.0);
}
)";

// =========================================================================
// FUNÇÃO MAIN
// =========================================================================
int main() {
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "GrauB - Felipe Mossmann", nullptr, nullptr);
    glfwMakeContextCurrent(window);
    
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetScrollCallback(window, scroll_callback);
    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) return -1;

    glViewport(0, 0, WIDTH, HEIGHT);
    glEnable(GL_DEPTH_TEST);

    GLuint shaderID = setupShader();
    glUseProgram(shaderID);
    
    carregarCena("cena.txt");

    glUniform1i(glGetUniformLocation(shaderID, "tex_buffer"), 0);

    while (!glfwWindowShouldClose(window)) {
        float currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        processInput(window); 

        glClearColor(0.05f, 0.05f, 0.05f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        glm::mat4 projection = glm::perspective(glm::radians(camera.Zoom), (float)WIDTH/(float)HEIGHT, 0.1f, 1000.0f);
        glm::mat4 view = camera.GetViewMatrix();
        
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "projection"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shaderID, "view"), 1, GL_FALSE, glm::value_ptr(view));
        glUniform3fv(glGetUniformLocation(shaderID, "camPos"), 1, glm::value_ptr(camera.Position));
        glUniform1i(glGetUniformLocation(shaderID, "useTexture"), usaTextura);

        glm::vec3 basePos = cena.empty() ? glm::vec3(0.0f) : cena[objetoSelecionado].posicao;

        glm::vec3 keyPos = basePos + glm::vec3(500.0f, 600.0f, 500.0f);
        glUniform3fv(glGetUniformLocation(shaderID, "lights[0].position"), 1, glm::value_ptr(keyPos));
        glUniform3f(glGetUniformLocation(shaderID, "lights[0].color"), 0.8f, 0.8f, 0.8f);
        glUniform1i(glGetUniformLocation(shaderID, "lights[0].enabled"), keyLightOn);

        glm::vec3 fillPos = basePos + glm::vec3(-500.0f, 400.0f, 500.0f);
        glUniform3fv(glGetUniformLocation(shaderID, "lights[1].position"), 1, glm::value_ptr(fillPos));
        glUniform3f(glGetUniformLocation(shaderID, "lights[1].color"), 0.3f, 0.3f, 0.3f); 
        glUniform1i(glGetUniformLocation(shaderID, "lights[1].enabled"), fillLightOn);

        glm::vec3 backPos = basePos + glm::vec3(0.0f, 600.0f, -600.0f);
        glUniform3fv(glGetUniformLocation(shaderID, "lights[2].position"), 1, glm::value_ptr(backPos));
        glUniform3f(glGetUniformLocation(shaderID, "lights[2].color"), 0.2f, 0.2f, 0.2f);
        glUniform1i(glGetUniformLocation(shaderID, "lights[2].enabled"), backLightOn);

        for (int i = 0; i < cena.size(); i++) {
            if (cena[i].animando && cena[i].pontosDeControle.size() >= 3) {
                float tempoEscalado = currentFrame * cena[i].velocidadeAnimacao;
                float t = (sin(tempoEscalado) + 1.0f) / 2.0f; 
                cena[i].posicao = calculaBezier(cena[i].pontosDeControle, t);
            }

            glUniform1i(glGetUniformLocation(shaderID, "isSelected"), (i == objetoSelecionado) && mostraSelecao);

            glUniform3fv(glGetUniformLocation(shaderID, "ka"), 1, glm::value_ptr(cena[i].material.Ka * 0.5f));
            glUniform3fv(glGetUniformLocation(shaderID, "kd"), 1, glm::value_ptr(cena[i].material.Kd));
            glUniform3fv(glGetUniformLocation(shaderID, "ks"), 1, glm::value_ptr(cena[i].material.Ks));
            glUniform1f(glGetUniformLocation(shaderID, "q"), cena[i].material.Ns);

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
// FUNÇÕES MATEMÁTICAS E DE ARQUIVO
// =========================================================================
glm::vec3 calculaBezier(std::vector<glm::vec3> pontos, float t) {
    std::vector<glm::vec3> temp = pontos;
    int n = temp.size() - 1;
    for (int i = 1; i <= n; i++) {
        for (int j = 0; j <= n - i; j++) {
            temp[j] = (1.0f - t) * temp[j] + t * temp[j + 1];
        }
    }
    return temp[0];
}

void carregarCena(string filepath) {
    std::ifstream arq(filepath);
    if (!arq.is_open()) {
        std::cout << "ERRO: Nao foi possivel abrir o arquivo " << filepath << std::endl;
        return;
    }
    std::string linha;
    while (std::getline(arq, linha)) {
        std::istringstream ss(linha);
        std::string tipo;
        ss >> tipo;
        if (tipo == "OBJETO") {
            std::string caminho;
            float px, py, pz, esc;
            float rx = 0.0f, ry = 0.0f, rz = 0.0f;
            
            ss >> caminho >> px >> py >> pz >> esc;
            if(ss >> rx >> ry >> rz) {}
            
            Modelo3D obj;
            obj.VAO = loadSimpleOBJ(caminho, obj.nVertices, glm::vec3(1.0f), obj.texID, obj.material);
            if(obj.VAO != -1) {
                obj.caminhoArquivo = caminho;
                obj.posicao = glm::vec3(px, py, pz);
                obj.escala = glm::vec3(esc);
                obj.rotacao = glm::vec3(rx, ry, rz);
                cena.push_back(obj);
                std::cout << "Carregado: " << caminho << std::endl;
            }
        }
    }
    arq.close();
}

void salvarCena() {
    std::ofstream arq("cena.txt");
    if(arq.is_open()) {
        for(int i = 0; i < cena.size(); i++) {
            arq << "OBJETO " << cena[i].caminhoArquivo << " "
                << cena[i].posicao.x << " " << cena[i].posicao.y << " " << cena[i].posicao.z << " "
                << cena[i].escala.x << " "
                << cena[i].rotacao.x << " " << cena[i].rotacao.y << " " << cena[i].rotacao.z << "\n";
        }
        arq.close();
        std::cout << "\n==============================================" << std::endl;
        std::cout << "[SUCESSO] CENA ATUALIZADA E SALVA NO cena.txt!" << std::endl;
        std::cout << "==============================================\n" << std::endl;
    } else {
        std::cout << "ERRO: Nao foi possivel salvar no cena.txt" << std::endl;
    }
}

// =========================================================================
// CONTROLES DE INPUT
// =========================================================================
void processInput(GLFWwindow *window) {
    if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) camera.ProcessKeyboard(FORWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) camera.ProcessKeyboard(BACKWARD, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) camera.ProcessKeyboard(LEFT, deltaTime);
    if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) camera.ProcessKeyboard(RIGHT, deltaTime);

    if(!cena.empty()) {
        float velMovimento = moveSpeed;
        float velRotacao = rotateSpeed;
        float velEscala = scaleSpeed * deltaTime;

        if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS) {
            velMovimento /= 10.0f;
            velRotacao /= 10.0f;
            velEscala /= 10.0f;
        }

        if (glfwGetKey(window, GLFW_KEY_I) == GLFW_PRESS) cena[objetoSelecionado].posicao.z -= velMovimento;
        if (glfwGetKey(window, GLFW_KEY_K) == GLFW_PRESS) cena[objetoSelecionado].posicao.z += velMovimento;
        if (glfwGetKey(window, GLFW_KEY_J) == GLFW_PRESS) cena[objetoSelecionado].posicao.x -= velMovimento;
        if (glfwGetKey(window, GLFW_KEY_L) == GLFW_PRESS) cena[objetoSelecionado].posicao.x += velMovimento;
        if (glfwGetKey(window, GLFW_KEY_Y) == GLFW_PRESS) cena[objetoSelecionado].posicao.y += velMovimento;
        if (glfwGetKey(window, GLFW_KEY_H) == GLFW_PRESS) cena[objetoSelecionado].posicao.y -= velMovimento;

        if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)    cena[objetoSelecionado].rotacao.x -= velRotacao;
        if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)  cena[objetoSelecionado].rotacao.x += velRotacao;
        if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)  cena[objetoSelecionado].rotacao.y -= velRotacao;
        if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS) cena[objetoSelecionado].rotacao.y += velRotacao;
        if (glfwGetKey(window, GLFW_KEY_Z) == GLFW_PRESS)     cena[objetoSelecionado].rotacao.z -= velRotacao;
        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS)     cena[objetoSelecionado].rotacao.z += velRotacao;

        if (glfwGetKey(window, GLFW_KEY_LEFT_BRACKET) == GLFW_PRESS) {
            cena[objetoSelecionado].escala -= glm::vec3(velEscala);
            if (cena[objetoSelecionado].escala.x < 0.001f) cena[objetoSelecionado].escala = glm::vec3(0.001f);
        }
        if (glfwGetKey(window, GLFW_KEY_RIGHT_BRACKET) == GLFW_PRESS) cena[objetoSelecionado].escala += glm::vec3(velEscala);
    }
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode) {
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS) glfwSetWindowShouldClose(window, GL_TRUE);
    if (key == GLFW_KEY_TAB && action == GLFW_PRESS && !cena.empty()) {
        objetoSelecionado = (objetoSelecionado + 1) % cena.size();
    }

    if (key == GLFW_KEY_ENTER && action == GLFW_PRESS) {
        salvarCena();
    }

    if (key == GLFW_KEY_T && action == GLFW_PRESS) {
        usaTextura = !usaTextura;
        std::cout << "Modo Textura: " << (usaTextura ? "LIGADO (Imagem)" : "DESLIGADO (Lendo .MTL)") << std::endl;
    }

    if (key == GLFW_KEY_O && action == GLFW_PRESS) {
        mostraSelecao = !mostraSelecao;
        std::cout << "Brilho de Selecao: " << (mostraSelecao ? "LIGADO" : "DESLIGADO") << std::endl;
    }

    if (key == GLFW_KEY_1 && action == GLFW_PRESS) keyLightOn = !keyLightOn;
    if (key == GLFW_KEY_2 && action == GLFW_PRESS) fillLightOn = !fillLightOn;
    if (key == GLFW_KEY_3 && action == GLFW_PRESS) backLightOn = !backLightOn;
    
    if(!cena.empty()) {
        if (key == GLFW_KEY_P && action == GLFW_PRESS) {
            cena[objetoSelecionado].pontosDeControle.push_back(camera.Position);
            std::cout << "PONTO BEZIER ADICIONADO: X=" << camera.Position.x << " Y=" << camera.Position.y << std::endl;
        }
        if (key == GLFW_KEY_SPACE && action == GLFW_PRESS) {
            if (cena[objetoSelecionado].pontosDeControle.size() >= 3) {
                cena[objetoSelecionado].animando = !cena[objetoSelecionado].animando;
            } else {
                std::cout << "Crie pelo menos 3 pontos (P) para formar a curva de Bezier!" << std::endl;
            }
        }
        if (key == GLFW_KEY_BACKSPACE && action == GLFW_PRESS) {
            cena[objetoSelecionado].pontosDeControle.clear();
            cena[objetoSelecionado].animando = false;
        }
    }
}

void mouse_callback(GLFWwindow* window, double xposIn, double yposIn) {
    float xpos = static_cast<float>(xposIn); float ypos = static_cast<float>(yposIn);
    if (firstMouse) { lastX = xpos; lastY = ypos; firstMouse = false; }
    camera.ProcessMouseMovement(xpos - lastX, lastY - ypos);
    lastX = xpos; lastY = ypos;
}

void scroll_callback(GLFWwindow* window, double xoffset, double yoffset) { camera.ProcessMouseScroll(static_cast<float>(yoffset)); }

int setupShader() {
    GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vertexShaderSource, NULL); glCompileShader(vertexShader);
    GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL); glCompileShader(fragmentShader);
    GLuint shaderProgram = glCreateProgram();
    glAttachShader(shaderProgram, vertexShader); glAttachShader(shaderProgram, fragmentShader);
    glLinkProgram(shaderProgram);
    glDeleteShader(vertexShader); glDeleteShader(fragmentShader);
    return shaderProgram;
}

GLuint loadTexture(string filePath) {
    GLuint texID; glGenTextures(1, &texID); glBindTexture(GL_TEXTURE_2D, texID); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT); 
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
    int width, height, nrChannels; stbi_set_flip_vertically_on_load(true);
    unsigned char *data = stbi_load(filePath.c_str(), &width, &height, &nrChannels, 0); 
    if (data) {
        GLenum format = (nrChannels == 4) ? GL_RGBA : GL_RGB;
        glTexImage2D(GL_TEXTURE_2D, 0, format, width, height, 0, format, GL_UNSIGNED_BYTE, data); 
        glGenerateMipmap(GL_TEXTURE_2D); 
    } else { std::cout << "Falha ao carregar a textura: " << filePath << std::endl; return 0; }
    stbi_image_free(data); glBindTexture(GL_TEXTURE_2D, 0); 
    return texID;
}

int loadSimpleOBJ(string filePATH, int &nVertices, glm::vec3 baseColor, GLuint &texID, Material &mat) {
    // Por-material: nome, tem textura, Kd
    std::vector<string> matNames;
    std::vector<bool>   matHasTex;
    std::vector<float>  matKdR, matKdG, matKdB;
    bool modelHasAnyTex = false;

    std::vector<glm::vec3> vertices; std::vector<glm::vec2> texCoords; std::vector<glm::vec3> normals; std::vector<GLfloat> vBuffer;
    texID = 0; string diretorio = ""; size_t lastSlash = filePATH.find_last_of('/');
    if (lastSlash != string::npos) diretorio = filePATH.substr(0, lastSlash + 1);
    std::ifstream arqEntrada(filePATH.c_str());
    if (!arqEntrada.is_open()) return -1;

    std::string line;
    bool curHasTex = true;
    float curVcR = baseColor.r, curVcG = baseColor.g, curVcB = baseColor.b;

    while (std::getline(arqEntrada, line)) {
        std::istringstream ssline(line); std::string word; ssline >> word;
        if (word == "mtllib") {
            string mtlFile; ssline >> mtlFile; std::ifstream mtlArq((diretorio + mtlFile).c_str());
            if (mtlArq.is_open()) {
                string mLine; int curIdx = -1; bool firstMat = true;
                while (getline(mtlArq, mLine)) {
                    std::istringstream mss(mLine); string mWord; mss >> mWord; // reset por linha
                    if (mWord == "newmtl") {
                        string mn; mss >> mn;
                        matNames.push_back(mn); matHasTex.push_back(false);
                        matKdR.push_back(0.5f); matKdG.push_back(0.5f); matKdB.push_back(0.5f);
                        curIdx = (int)matNames.size() - 1;
                        if (curIdx > 0) firstMat = false;
                    } else if (curIdx >= 0) {
                        if (mWord == "map_Kd") {
                            string texName; mss >> texName;
                            if (!texName.empty()) { GLuint tid = loadTexture(diretorio + texName); if (tid) { texID = tid; matHasTex[curIdx] = true; modelHasAnyTex = true; } }
                        } else if (mWord == "Kd") {
                            mss >> matKdR[curIdx] >> matKdG[curIdx] >> matKdB[curIdx];
                            if (firstMat) mat.Kd = glm::vec3(matKdR[curIdx], matKdG[curIdx], matKdB[curIdx]);
                        } else if (mWord == "Ka" && firstMat) { mss >> mat.Ka.r >> mat.Ka.g >> mat.Ka.b;
                        } else if (mWord == "Ks" && firstMat) { mss >> mat.Ks.r >> mat.Ks.g >> mat.Ks.b;
                        } else if (mWord == "Ns" && firstMat) { mss >> mat.Ns; }
                    }
                } mtlArq.close();
            }
        } else if (word == "usemtl") {
            if (!matNames.empty()) {
                string mname; ssline >> mname;
                curHasTex = false; curVcR = baseColor.r; curVcG = baseColor.g; curVcB = baseColor.b;
                for (int i = 0; i < (int)matNames.size(); i++) {
                    if (matNames[i] == mname) {
                        curHasTex = matHasTex[i];
                        if (!curHasTex) { curVcR = matKdR[i]; curVcG = matKdG[i]; curVcB = matKdB[i]; }
                        break;
                    }
                }
            }
        } else if (word == "v") { glm::vec3 vertice; ssline >> vertice.x >> vertice.y >> vertice.z; vertices.push_back(vertice);
        } else if (word == "vt") { glm::vec2 vt; ssline >> vt.s >> vt.t; texCoords.push_back(vt);
        } else if (word == "vn") { glm::vec3 normal; ssline >> normal.x >> normal.y >> normal.z; normals.push_back(normal);
        } else if (word == "f") {
            std::vector<GLfloat> faceData;
            while (ssline >> word) {
                int vi = 0, ti = 0, ni = 0; std::istringstream ss(word); std::string index;
                if (std::getline(ss, index, '/')) vi = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index, '/')) ti = !index.empty() ? std::stoi(index) - 1 : 0;
                if (std::getline(ss, index)) ni = !index.empty() ? std::stoi(index) - 1 : 0;

                faceData.push_back(vertices[vi].x); faceData.push_back(vertices[vi].y); faceData.push_back(vertices[vi].z);
                faceData.push_back(curVcR); faceData.push_back(curVcG); faceData.push_back(curVcB);
                if (!normals.empty()) { faceData.push_back(normals[ni].x); faceData.push_back(normals[ni].y); faceData.push_back(normals[ni].z); }
                else { faceData.push_back(0.0f); faceData.push_back(0.0f); faceData.push_back(1.0f); }
                // sentinel (-1,-1) so apenas em modelos multi-material onde esta face nao tem textura
                if (modelHasAnyTex && !curHasTex) { faceData.push_back(-1.0f); faceData.push_back(-1.0f); }
                else if (!texCoords.empty() && ti >= 0) { faceData.push_back(texCoords[ti].s); faceData.push_back(texCoords[ti].t); }
                else { faceData.push_back(0.0f); faceData.push_back(0.0f); }
            }
            int numVertices = faceData.size() / 11;
            if (numVertices >= 3) {
                for (int i = 1; i < numVertices - 1; i++) {
                    for(int j=0; j<11; j++) vBuffer.push_back(faceData[j]);
                    for(int j=0; j<11; j++) vBuffer.push_back(faceData[i*11 + j]);
                    for(int j=0; j<11; j++) vBuffer.push_back(faceData[(i+1)*11 + j]);
                }
            }
        }
    }
    arqEntrada.close();

    GLuint VBO, VAO; glGenBuffers(1, &VBO); glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, vBuffer.size() * sizeof(GLfloat), vBuffer.data(), GL_STATIC_DRAW);
    glGenVertexArrays(1, &VAO); glBindVertexArray(VAO);
    int stride = 11 * sizeof(GLfloat);
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)0); glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(3 * sizeof(GLfloat))); glEnableVertexAttribArray(1);
    glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(6 * sizeof(GLfloat))); glEnableVertexAttribArray(2);
    glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, stride, (GLvoid*)(9 * sizeof(GLfloat))); glEnableVertexAttribArray(3);
    glBindBuffer(GL_ARRAY_BUFFER, 0); glBindVertexArray(0);
    nVertices = vBuffer.size() / 11; return VAO;
}