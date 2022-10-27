#include <iostream>
#include <cmath>
#include <vector>

#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/random.hpp>

#include <Shader.hpp>
#include <Agent.hpp>

// #include <config.hpp>

const unsigned short OPENGL_MAJOR_VERSION = 4;
const unsigned short OPENGL_MINOR_VERSION = 6;

const bool vSync = true;

Agent *createAgents() {
	Agent *agents = new Agent[AGENT_COUNT];
	for (int i = 0; i < AGENT_COUNT; i++) {
		agents[i] = Agent();
	}
	return agents;
}

GLfloat *getAgentVertices(Agent *agents) {
	GLfloat *vertices = new GLfloat[AGENT_COUNT * 2];
	for (int i = 0; i < AGENT_COUNT; i++) {
		vertices[i*2] = agents[i].position.x;
		vertices[i*2 + 1] = agents[i].position.y;
	}
	return vertices;
}

int main() {
	// ------------------------------------------------------------------------
	// WINDOW

	glfwInit();

	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, OPENGL_MAJOR_VERSION);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, OPENGL_MINOR_VERSION);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
	glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, true);

	GLFWwindow* window = glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, "OpenGL Compute Shaders", NULL, NULL);
	if (!window) {
		std::cout << "Failed to create the GLFW window\n";
		glfwTerminate();
	}

	glfwMakeContextCurrent(window);
	glfwSwapInterval(vSync);

	if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {

		std::cout << "Failed to initialize OpenGL context\n";
	}

	glViewport(0, 0, WINDOW_WIDTH, WINDOW_HEIGHT);

	// ---------------- BEGIN SHADER DATA ----------------

	// We generate a bunch of positions
	Agent *agents = createAgents();
    std::vector<glm::vec4> agentData(AGENT_COUNT);

	// initial position
    for(int i = 0; i < AGENT_COUNT; i++) {
        glm::vec2 position = agents[i].position;
        glm::vec2 direction = agents[i].direction;
		agentData[i] = glm::vec4(position.x, position.y, direction.x, direction.y);
    }

	GLuint agentDataBuffer;
	glCreateBuffers(1, &agentDataBuffer);

    // fill with initial data
    glBindBuffer(GL_ARRAY_BUFFER, agentDataBuffer);
    glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec4)*AGENT_COUNT, &agentData[0], GL_STATIC_DRAW);

    // set up generic attrib pointers: do I need these?
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(0, 4, GL_FLOAT, GL_FALSE, 4*sizeof(GLfloat), (char*)0 + 0*sizeof(GLfloat));

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, agentDataBuffer);

	// ---------------- END SHADER DATA ----------------

	// ------------------------------------------------------------------------
	// SCREEN TEXTURE

	GLuint screenTexture;
	glCreateTextures(GL_TEXTURE_2D, 1, &screenTexture);
	glTextureParameteri(screenTexture, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTextureParameteri(screenTexture, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTextureParameteri(screenTexture, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTextureParameteri(screenTexture, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTextureStorage2D(screenTexture, 1, GL_RGBA32F, WINDOW_WIDTH, WINDOW_HEIGHT);
	glBindImageTexture(0, screenTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

	// ------------------------------------------------------------------------
	// SCREEN SHADER

	GLfloat screenVertices[] = {
		-1.0, -1.0,
		-1.0,  1.0,
		 1.0,  1.0,
		 1.0, -1.0,
	};

	GLuint screenIndices[] = {
		0, 2, 1,
		0, 2, 3
	};

	Shader screenShaderProgram("./shaders/screen.vert", "./shaders/screen.frag");

	GLuint screenVAO, screenVBO, screenEBO;
	glCreateVertexArrays(1, &screenVAO);
	glCreateBuffers(1, &screenVBO);
	glCreateBuffers(1, &screenEBO);

	glNamedBufferData(screenVBO, sizeof(screenVertices), screenVertices, GL_STATIC_DRAW);
	glNamedBufferData(screenEBO, sizeof(screenIndices), screenIndices, GL_STATIC_DRAW);

	glEnableVertexArrayAttrib(screenVAO, 0);
	glVertexArrayAttribBinding(screenVAO, 0, 0);
	glVertexArrayAttribFormat(screenVAO, 0, 2, GL_FLOAT, GL_FALSE, 0);

	glVertexArrayVertexBuffer(screenVAO, 0, screenVBO, 0, 2 * sizeof(GLfloat));
	glVertexArrayElementBuffer(screenVAO, screenEBO);

	// ------------------------------------------------------------------------
	// AGENT SHADER

	GLfloat *agentVertices = getAgentVertices(agents);
	Shader agentShaderProgram("./shaders/agent.vert", "./shaders/agent.frag");

	GLuint agentVAO, agentVBO;
	glCreateVertexArrays(1, &agentVAO);
	glCreateBuffers(1, &agentVBO);

	glNamedBufferData(agentVBO, sizeof(GLfloat) * 2 * AGENT_COUNT, agentVertices, GL_STATIC_DRAW);

	glEnableVertexArrayAttrib(agentVAO, 0);
	glVertexArrayAttribBinding(agentVAO, 0, 0);
	glVertexArrayAttribFormat(agentVAO, 0, 2, GL_FLOAT, GL_FALSE, 0);

	glVertexArrayVertexBuffer(agentVAO, 0, agentVBO, 0, 2 * sizeof(GLfloat));

	// ------------------------------------------------------------------------
	// BOX-BLUR SHADER

	Shader blurShaderProgram("./shaders/box-blur.vert", "./shaders/box-blur.frag");

	// glCreateVertexArrays(1, &screenVAO);
	// glCreateBuffers(1, &screenVBO);
	// glCreateBuffers(1, &screenEBO);

	// glNamedBufferData(screenVBO, sizeof(screenVertices), screenVertices, GL_STATIC_DRAW);
	// glNamedBufferData(screenEBO, sizeof(screenIndices), screenIndices, GL_STATIC_DRAW);

	// glEnableVertexArrayAttrib(screenVAO, 0);
	// glVertexArrayAttribBinding(screenVAO, 0, 0);
	// glVertexArrayAttribFormat(screenVAO, 0, 2, GL_FLOAT, GL_FALSE, 0);

	// glVertexArrayVertexBuffer(screenVAO, 0, screenVBO, 0, 2 * sizeof(GLfloat));
	// glVertexArrayElementBuffer(screenVAO, screenEBO);

	Shader computeShader("./shaders/blur.comp", GL_COMPUTE_SHADER);

	GLuint computeProgram = glCreateProgram();
	glAttachShader(computeProgram, computeShader.ID);
	glLinkProgram(computeProgram);

	glDeleteShader(computeShader.ID);

	// ------------------------------------------------------------------------
	// MAIN WHILE LOOP
	
	while (!glfwWindowShouldClose(window)) {
		// BIND TEXTURE IMAGE
		glBindImageTexture(0, screenTexture, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);

		// SCREEN SHADER
		glUseProgram(screenShaderProgram.ID);
		glBindVertexArray(screenVAO);
		glDrawElements(GL_TRIANGLES, sizeof(screenIndices) / sizeof(screenIndices[0]), GL_UNSIGNED_INT, 0);

		// AGENT SHADER
		glUseProgram(agentShaderProgram.ID);
		glBindVertexArray(agentVAO);
		glDrawArrays(GL_POINTS, 0, AGENT_COUNT);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		// BLUR SHADER
		glUseProgram(blurShaderProgram.ID);
		glBindVertexArray(screenVAO);
		glDrawElements(GL_TRIANGLES, sizeof(screenIndices) / sizeof(screenIndices[0]), GL_UNSIGNED_INT, 0);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		glUseProgram(computeProgram);
		glUniform1f(glGetUniformLocation(computeProgram, "agentSpeed"), AGENT_SPEED);
		glDispatchCompute(ceil(WINDOW_WIDTH / 8), ceil(WINDOW_HEIGHT / 4), 1);
		glMemoryBarrier(GL_ALL_BARRIER_BITS);

		glfwSwapBuffers(window);
		glfwPollEvents();

		std::cout << glGetError() << "\n";
	}
	
	glfwDestroyWindow(window);
	glfwTerminate();
	return 0;
}