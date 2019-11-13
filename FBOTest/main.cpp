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
	// 초기화, 에러 핸들링 등록
	glfwInit();
	glfwSetErrorCallback([](int err, const char* desc) { puts(desc); });
	initContext(/*use dafault = */ true);
	GLFWwindow* window = glfwCreateWindow(g_width, g_height, "Default!", nullptr, nullptr);
	glfwMakeContextCurrent(window);
	glewInit();

	// 객체
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

	// 생성
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

	// 검사
	printAllErrors("객체 생성 후 문제 체크");

	// 루프
	while (!glfwWindowShouldClose(window)) {
		float angle = (float)glfwGetTime();

		/*
			geomFBO 를 사용하여
			VAO 의 출력값(0), 노멀값(1) 을 얻습니다.
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
			edgeFBO 를 사용하여
			위의 출력값을 통해 엣지(0)를 얻습니다.
		*/
		edgeFBO.bind();
		glDisable(GL_DEPTH_TEST);
		glClearColor(0, 0, 0, 0);
		{
			// 깊이을 통해 엣지를 구함
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
			combFBO 를 사용하여
			위의 엣지와 노멀를 더합니다.
		*/
		combFBO.bind();
		glDisable(GL_DEPTH_TEST);
		glClearColor(0, 0, 0, 0);
		{
			// 합체
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
			결과를 렌더링합니다.
			(좌상:) 위치값		(우상:) 노말
			(좌하:) 엣지			(우하:) 합체
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
			// 노멀값
			mat = glm::translate(glm::vec3(-0.5f, 0.5f, 0)) *
				glm::scale(glm::vec3(0.4f, 0.4f, 1.f));
			glUniformMatrix4fv(0, 1, GL_FALSE, &mat[0][0]);
			geomFBO.bindColorTexture(/*texture*/1);
			glDrawArrays(GL_QUADS, 0, 4);

			// 엣지
			mat = glm::translate(glm::vec3(0.5f, 0.5f, 0)) *
				glm::scale(glm::vec3(0.4f, 0.4f, 1.f));
			glUniformMatrix4fv(0, 1, GL_FALSE, &mat[0][0]);
			edgeFBO.bindColorTexture();
			glDrawArrays(GL_QUADS, 0, 4);

			// 배경 + 노멀값
			mat = glm::translate(glm::vec3(-0.5f, -0.5f, 0)) *
				glm::scale(glm::vec3(0.4f, 0.4f, 1.f));
			glUniformMatrix4fv(0, 1, GL_FALSE, &mat[0][0]);
			skyTexture.bind();
			glDrawArrays(GL_QUADS, 0, 4);
			geomFBO.bindColorTexture(/*texture*/1);
			glDrawArrays(GL_QUADS, 0, 4);

			// 배경 + 노멀 + 엣지
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

		// 버퍼 스왑, 이벤트 폴
		glfwSwapBuffers(window);
		glfwPollEvents();
	}

	// 제거
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

	// 종료
	glfwTerminate();
}

void initContext(bool useDefault, int major, int minor, bool useCompatibility) {
	if (useDefault) {
		// 기본적으로  Default로 설정해주면
		// 가능한 최신의 OpenGL Vesion을 선택하며 (연구실 컴퓨터는 4.5)
		// Profile은 Legacy Function도 사용할 수 있게 해줍니다.(Compatibility 사용)
		glfwDefaultWindowHints();
	} else {
		// Major 와 Minor 결정
		glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, major);
		glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, minor);

		// Core 또는 Compatibility 선택
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