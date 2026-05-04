/* Hello Triangle - código adaptado de https://learnopengl.com/#!Getting-started/Hello-Triangle
 *
 * Adaptado por Rossana Baptista Queiroz
 * para as disciplinas de Processamento Gráfico/Computação Gráfica - Unisinos
 * Versão inicial: 7/4/2017
 * Última atualização em 07/03/2025
 */

#include <iostream>
#include <string>
#include <assert.h>

using namespace std;

// GLAD
#include <glad/glad.h>

// GLFW
#include <GLFW/glfw3.h>

//GLM
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);

int setupShader();
int setupGeometry();

const GLuint WIDTH = 1000, HEIGHT = 1000;

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
"void main()\n"
"{\n"
"color = finalColor;\n"
"}\n\0";

bool rotateX=false, rotateY=false, rotateZ=false;
glm::vec3 cubePos = glm::vec3(-0.7f, 0.0f, 0.0f);
float moveSpeed = 0.20f;
float cubeScale = 0.5f;
float scaleSpeed = 0.05f;

int main()
{
	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

	GLFWwindow* window = glfwCreateWindow(WIDTH, HEIGHT, "Olá 3D (Cubo) -- Felipe Mossmann!", nullptr, nullptr);
	glfwMakeContextCurrent(window);

	glfwSetKeyCallback(window, key_callback);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
	{
		std::cout << "Failed to initialize GLAD" << std::endl;
	}

	const GLubyte* renderer = glGetString(GL_RENDERER);
	const GLubyte* version = glGetString(GL_VERSION);
	cout << "Renderer: " << renderer << endl;
	cout << "OpenGL version supported " << version << endl;

	int width, height;
	glfwGetFramebufferSize(window, &width, &height);
	glViewport(0, 0, width, height);

	GLuint shaderID = setupShader();
	GLuint VAO = setupGeometry();

	glUseProgram(shaderID);

	glm::mat4 model = glm::mat4(1);
	GLint modelLoc = glGetUniformLocation(shaderID, "model");

	model = glm::rotate(model, glm::radians(90.0f), glm::vec3(1.0f, 0.0f, 0.0f));
	glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

	glEnable(GL_DEPTH_TEST);

	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();

		glClearColor(1.0f, 1.0f, 1.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		glLineWidth(10);
		glPointSize(20);

		float angle = (GLfloat)glfwGetTime();

		model = glm::mat4(1);

		model = glm::translate(model, cubePos);
		model = glm::scale(model, glm::vec3(cubeScale));

		if (rotateX)
		{
			model = glm::rotate(model, angle, glm::vec3(1.0f, 0.0f, 0.0f));
		}
		else if (rotateY)
		{
			model = glm::rotate(model, angle, glm::vec3(0.0f, 1.0f, 0.0f));
		}
		else if (rotateZ)
		{
			model = glm::rotate(model, angle, glm::vec3(0.0f, 0.0f, 1.0f));
		}

		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model));

		glBindVertexArray(VAO);

		// Cubo principal
		glDrawArrays(GL_TRIANGLES, 0, 36);

		// Cubo secundário
		glm::mat4 model2 = glm::mat4(1.0f);
		model2 = glm::translate(model2, glm::vec3(0.6f, 0.0f, -0.5f));
		model2 = glm::scale(model2, glm::vec3(0.5f, 0.5f, 0.5f));
		model2 = glm::rotate(model2, angle, glm::vec3(0.0f, 1.0f, 0.0f));
		glUniformMatrix4fv(modelLoc, 1, GL_FALSE, glm::value_ptr(model2));
		glDrawArrays(GL_TRIANGLES, 0, 36);

		glBindVertexArray(0);

		glfwSwapBuffers(window);
	}

	glDeleteVertexArrays(1, &VAO);
	glfwTerminate();

	return 0;
}

void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
		glfwSetWindowShouldClose(window, GL_TRUE);

	if (key == GLFW_KEY_X && action == GLFW_PRESS)
	{
		rotateX = true;
		rotateY = false;
		rotateZ = false;
	}

	if (key == GLFW_KEY_Y && action == GLFW_PRESS)
	{
		rotateX = false;
		rotateY = true;
		rotateZ = false;
	}

	if (key == GLFW_KEY_Z && action == GLFW_PRESS)
	{
		rotateX = false;
		rotateY = false;
		rotateZ = true;
	}

	if (key == GLFW_KEY_W && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		cubePos.y += moveSpeed;
	}

	if (key == GLFW_KEY_A && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		cubePos.x -= moveSpeed;
	}

	if (key == GLFW_KEY_S && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		cubePos.y -= moveSpeed;
	}

	if (key == GLFW_KEY_D && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		cubePos.x += moveSpeed;
	}

	if (key == GLFW_KEY_I && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		cubePos.y += moveSpeed;
	}

	if (key == GLFW_KEY_J && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		cubePos.y -= moveSpeed;
	}

	if (key == GLFW_KEY_LEFT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		cubeScale -= scaleSpeed;

		if (cubeScale < 0.1f)
		{
			cubeScale = 0.1f;
		}
	}

	if (key == GLFW_KEY_RIGHT_BRACKET && (action == GLFW_PRESS || action == GLFW_REPEAT))
	{
		cubeScale += scaleSpeed;
	}
}

int setupShader()
{
	GLuint vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertexShaderSource, NULL);
	glCompileShader(vertexShader);

	GLint success;
	GLchar infoLog[512];

	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::VERTEX::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragmentShaderSource, NULL);
	glCompileShader(fragmentShader);

	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);

	if (!success)
	{
		glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::FRAGMENT::COMPILATION_FAILED\n" << infoLog << std::endl;
	}

	GLuint shaderProgram = glCreateProgram();

	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);

	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);

	if (!success)
	{
		glGetProgramInfoLog(shaderProgram, 512, NULL, infoLog);
		std::cout << "ERROR::SHADER::PROGRAM::LINKING_FAILED\n" << infoLog << std::endl;
	}

	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	return shaderProgram;
}

int setupGeometry()
{
	GLfloat vertices[] = {

		// x      y      z        r     g     b

		// Face frontal - vermelho suave
		-0.5f, -0.5f,  0.5f,   0.9f, 0.3f, 0.3f,
		 0.5f, -0.5f,  0.5f,   0.9f, 0.3f, 0.3f,
		-0.5f,  0.5f,  0.5f,   0.9f, 0.3f, 0.3f,
		 0.5f, -0.5f,  0.5f,   0.9f, 0.3f, 0.3f,
		 0.5f,  0.5f,  0.5f,   0.9f, 0.3f, 0.3f,
		-0.5f,  0.5f,  0.5f,   0.9f, 0.3f, 0.3f,

		// Face traseira - verde suave
		-0.5f, -0.5f, -0.5f,   0.3f, 0.8f, 0.4f,
		-0.5f,  0.5f, -0.5f,   0.3f, 0.8f, 0.4f,
		 0.5f, -0.5f, -0.5f,   0.3f, 0.8f, 0.4f,
		 0.5f, -0.5f, -0.5f,   0.3f, 0.8f, 0.4f,
		-0.5f,  0.5f, -0.5f,   0.3f, 0.8f, 0.4f,
		 0.5f,  0.5f, -0.5f,   0.3f, 0.8f, 0.4f,

		// Face esquerda - azul
		-0.5f, -0.5f, -0.5f,   0.3f, 0.5f, 1.0f,
		-0.5f, -0.5f,  0.5f,   0.3f, 0.5f, 1.0f,
		-0.5f,  0.5f, -0.5f,   0.3f, 0.5f, 1.0f,
		-0.5f, -0.5f,  0.5f,   0.3f, 0.5f, 1.0f,
		-0.5f,  0.5f,  0.5f,   0.3f, 0.5f, 1.0f,
		-0.5f,  0.5f, -0.5f,   0.3f, 0.5f, 1.0f,

		// Face direita - amarelo suave
		 0.5f, -0.5f, -0.5f,   1.0f, 0.8f, 0.2f,
		 0.5f,  0.5f, -0.5f,   1.0f, 0.8f, 0.2f,
		 0.5f, -0.5f,  0.5f,   1.0f, 0.8f, 0.2f,
		 0.5f, -0.5f,  0.5f,   1.0f, 0.8f, 0.2f,
		 0.5f,  0.5f, -0.5f,   1.0f, 0.8f, 0.2f,
		 0.5f,  0.5f,  0.5f,   1.0f, 0.8f, 0.2f,

		// Face superior - roxo
		-0.5f,  0.5f, -0.5f,   0.6f, 0.3f, 0.9f,
		-0.5f,  0.5f,  0.5f,   0.6f, 0.3f, 0.9f,
		 0.5f,  0.5f, -0.5f,   0.6f, 0.3f, 0.9f,
		 0.5f,  0.5f, -0.5f,   0.6f, 0.3f, 0.9f,
		-0.5f,  0.5f,  0.5f,   0.6f, 0.3f, 0.9f,
		 0.5f,  0.5f,  0.5f,   0.6f, 0.3f, 0.9f,

		// Face inferior - laranja
		-0.5f, -0.5f, -0.5f,   1.0f, 0.5f, 0.2f,
		 0.5f, -0.5f, -0.5f,   1.0f, 0.5f, 0.2f,
		-0.5f, -0.5f,  0.5f,   1.0f, 0.5f, 0.2f,
		-0.5f, -0.5f,  0.5f,   1.0f, 0.5f, 0.2f,
		 0.5f, -0.5f, -0.5f,   1.0f, 0.5f, 0.2f,
		 0.5f, -0.5f,  0.5f,   1.0f, 0.5f, 0.2f
	};

	GLuint VBO, VAO;

	glGenBuffers(1, &VBO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);

	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glGenVertexArrays(1, &VAO);

	glBindVertexArray(VAO);

	// Atributo posição
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)0);
	glEnableVertexAttribArray(0);

	// Atributo cor
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 6 * sizeof(GLfloat), (GLvoid*)(3 * sizeof(GLfloat)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindVertexArray(0);

	return VAO;
}