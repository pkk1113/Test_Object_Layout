#include <gl/glew.h>
#include <GLFW/glfw3.h>
#include <glm/gtx/transform.hpp>
#include "GLObject.h"
#include <cstdio>

int g_width = 1000;
int g_height = 800;
float g_aspect = (float)g_width / (float)g_height;

void initContext(bool useDefault, int major = 3, int minor = 3, bool useCompatibility = false);
void framebufferSizeCallback(GLFWwindow*, int w, int h);

int main() {
	// �ʱ�ȭ, ���� �ڵ鸵 ���
	glfwInit();
	glfwSetErrorCallback([](int err, const char* desc) { puts(desc); });
	initContext(/*use dafault = */ true);
	GLFWwindow* window = glfwCreateWindow(g_width, g_height, "Default!", nullptr, nullptr);
	glfwMakeContextCurrent(window);
	glewInit();

	// ��ü
	VAO vao;
	VAO vao2;
	FBO geomFBO;
	FBO edgeFBO;
	FBO combFBO;
	Shader geomShader;
	Shader quadShader;
	Shader edgeShader;
	Shader combShader;
	Texture skyTexture;

	// ����
	vao.load("resources/objects/fish.obj");
	vao2.load("resources/objects/monkey.obj");
	geomFBO.create(512, 512, 2, /*has depth texture*/ true); // position, normal
	edgeFBO.create(512, 512, 1, /*has depth texture*/ false); // edge
	combFBO.create(512, 512, 1, /*has depth texture*/ false); // comb
	quadShader.load("resources/shaders/quad.vert", "resources/shaders/quad.frag");
	geomShader.load("resources/shaders/geom.vert", "resources/shaders/geom.frag");
	edgeShader.load("resources/shaders/quad.vert", "resources/shaders/edge.frag");
	combShader.load("resources/shaders/quad.vert", "resources/shaders/comb.frag");
	skyTexture.load("resources/textures/sky.jpg");

	// �˻�
	printAllErrors("��ü ���� �� ���� üũ");

	// ����
	while (!glfwWindowShouldClose(window)) {
		float angle = (float)glfwGetTime();

		/*
			geomFBO �� ����Ͽ�
			VAO �� ��°�(0), ��ְ�(1) �� ����ϴ�.
		*/
		geomFBO.bind();
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);
		glClearDepth(1.f);
		glClearColor(1, 1, 1, 0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		{
			geomShader.use();
			glm::mat4 pmat = glm::perspective(45.f, g_aspect, 1.f, 3.f);
			glUniformMatrix4fv(0, 1, GL_FALSE, &pmat[0][0]);
			glm::mat4 vmat = glm::lookAt(glm::vec3(0, 0, 2), glm::vec3(0, 0, 0), glm::vec3(0, 1, 0));
			glUniformMatrix4fv(1, 1, GL_FALSE, &vmat[0][0]);

			// vao 1
			glUniform1f(3, 0.f);
			glm::mat4 mmat = glm::rotate(angle, glm::vec3(0, 1, 0));
			glUniformMatrix4fv(2, 1, GL_FALSE, &mmat[0][0]);
			vao.bind_render();

			// vao 2
			glUniform1f(3, 0.f);
			mmat = glm::translate(glm::vec3(0.5f, 0, 0))
				* glm::scale(glm::vec3(0.6f, 0.6f, 0.6f))
				* glm::rotate(angle, glm::vec3(0, 0, 1));
			glUniformMatrix4fv(2, 1, GL_FALSE, &mmat[0][0]);
			vao2.bind_render();

			// vao 3
			glUniform1f(3, 1.f);
			mmat = glm::translate(glm::vec3(-0.5f, 0, 0))
				* glm::scale(glm::vec3(0.6f, 0.6f, 0.6f))
				* glm::rotate(angle, glm::vec3(0, 1, 0));
			glUniformMatrix4fv(2, 1, GL_FALSE, &mmat[0][0]);
			vao2.bind_render();

			geomShader.unuse();
		}
		geomFBO.unbind();

		/*
			edgeFBO �� ����Ͽ�
			���� ��°��� ���� ����(0)�� ����ϴ�.
		*/
		edgeFBO.bind();
		glDisable(GL_DEPTH_TEST);
		glClearColor(0, 0, 0, 0);
		{
			// ������ ���� ������ ����
			edgeShader.use();
			{
				glm::mat4 mat = glm::mat4(1.f);
				glUniformMatrix4fv(0, 1, GL_FALSE, &mat[0][0]);
				geomFBO.bindColorTexture(/*texture*/0);
				glDrawArrays(GL_QUADS, 0, 4);
			}
			edgeShader.unuse();
		}
		edgeFBO.unbind();

		/*
			combFBO �� ����Ͽ�
			���� ������ ��ָ� ���մϴ�.
		*/
		combFBO.bind();
		glDisable(GL_DEPTH_TEST);
		glClearColor(0, 0, 0, 0);
		{
			// ��ü
			combShader.use();
			{
				glm::mat4 mat = glm::mat4(1.f);
				glUniformMatrix4fv(0, 1, GL_FALSE, &mat[0][0]);
				geomFBO.bindColorTexture(/*texture*/1, /*unit*/0);
				edgeFBO.bindColorTexture(/*texture*/0, /*unit*/1);
				glDrawArrays(GL_QUADS, 0, 4);
			}
			combShader.unuse();
		}
		combFBO.unbind();


		/*
			����� �������մϴ�.
			(�»�:) ��ġ��		(���:) �븻
			(����:) ����			(����:) ��ü
		*/
		glViewport(0, 0, g_width, g_height);
		glDisable(GL_DEPTH_TEST);
		glClearColor(1, 1, 1, 0);
		glClear(GL_COLOR_BUFFER_BIT);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		quadShader.use();
		glm::mat4 mat;
		{
			// ��ְ�
			mat = glm::translate(glm::vec3(-0.5f, 0.5f, 0)) *
				glm::scale(glm::vec3(0.4f, 0.4f, 1.f));
			glUniformMatrix4fv(0, 1, GL_FALSE, &mat[0][0]);
			geomFBO.bindColorTexture(/*texture*/1);
			glDrawArrays(GL_QUADS, 0, 4);

			// ����
			mat = glm::translate(glm::vec3(0.5f, 0.5f, 0)) *
				glm::scale(glm::vec3(0.4f, 0.4f, 1.f));
			glUniformMatrix4fv(0, 1, GL_FALSE, &mat[0][0]);
			edgeFBO.bindColorTexture();
			glDrawArrays(GL_QUADS, 0, 4);

			// ��� + ��ְ�
			mat = glm::translate(glm::vec3(-0.5f, -0.5f, 0)) *
				glm::scale(glm::vec3(0.4f, 0.4f, 1.f));
			glUniformMatrix4fv(0, 1, GL_FALSE, &mat[0][0]);
			skyTexture.bind();
			glDrawArrays(GL_QUADS, 0, 4);
			geomFBO.bindColorTexture(/*texture*/1);
			glDrawArrays(GL_QUADS, 0, 4);

			// ��� + ��� + ����
			mat = glm::translate(glm::vec3(0.5f, -0.5f, 0)) *
				glm::scale(glm::vec3(0.4f, 0.4f, 0.4f));
			glUniformMatrix4fv(0, 1, GL_FALSE, &mat[0][0]);
			skyTexture.bind();
			glDrawArrays(GL_QUADS, 0, 4);
			combFBO.bindColorTexture();
			glDrawArrays(GL_QUADS, 0, 4);
		}
		quadShader.unuse();
		glDisable(GL_BLEND);

		// ���� ����, �̺�Ʈ ��
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// ����
	vao.unload();
	vao2.unload();
	geomFBO.destroy();
	edgeFBO.destroy();
	combFBO.destroy();
	geomShader.unload();
	quadShader.unload();
	edgeShader.unload();
	combShader.unload();
	skyTexture.unload();

	// ����
	glfwTerminate();
}

void initContext(bool useDefault, int major, int minor, bool useCompatibility) {
	if (useDefault) {
		// �⺻������  Default�� �������ָ�
		// ������ �ֽ��� OpenGL Vesion�� �����ϸ� (������ ��ǻ�ʹ� 4.5)
		// Profile�� Legacy Function�� ����� �� �ְ� ���ݴϴ�.(Compatibility ���)
		glfwDefaultWindowHints();
	} else {
		// Major �� Minor ����
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);

		// Core �Ǵ� Compatibility ����
		if (useCompatibility) {
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
		} else {
			glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
		}
	}
}

void framebufferSizeCallback(GLFWwindow*, int w, int h) {
	g_width = w;
	g_height = h;
	g_aspect = (float)g_width / (float)g_height;
	printf("%d, %d\n", w, h);
}