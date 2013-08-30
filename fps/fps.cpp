#include <iostream>
#include <string>
#include <string.h>
#include <SDL.h>
#include <GL/glew.h>

template <class T>
struct Matrix44
{
	T m[16];

	static Matrix44 Ortho(T right, T left, T top, T bottom, T nearp, T farp) {
		Matrix44 mat;
		mat.m[0] = 2 / (right - left);
		mat.m[1] = 0.0f;
		mat.m[2] = 0.0f;
		mat.m[3] = 0.0f;
		mat.m[4] = 0.0f;
		mat.m[5] = 2 / (top - bottom);
		mat.m[6] = 0.0f;
		mat.m[7] = 0.0f;
		mat.m[8] = 0.0f;
		mat.m[9] = 0.0f;
		mat.m[10] = 2 / (farp - nearp);
		mat.m[11] = 0.0f;
		mat.m[12] = -(right + left) / (right - left);
		mat.m[13] = -(top + bottom) / (top - bottom);
		mat.m[14] = -(farp + nearp) / (farp - nearp);
		mat.m[15] = 1.0f;
		return mat;
	}

};

template <int type>
struct Shader {
    GLuint id;
	bool compiled;
	Shader(std::string& source) {
		id = glCreateShader(type);
		const GLchar* str = source.c_str();
		const GLint length = source.length();
		glShaderSource(id, 1, &str, &length);
		glCompileShader(id);
	}
};

int main(int argc, char **argv)
{
	const int width = 800;
	const int height = 600;
    const float aspectRatio = 1.0f * width / height;

	// Up to 16 attributes per vertex is allowed so any value between 0 and 15 will do.
	const int POSITION_ATTRIBUTE_INDEX = 0;

	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_Window* win = SDL_CreateWindow("FPS Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	SDL_GLContext ctx = SDL_GL_CreateContext(win);
	glewInit(); // must be called AFTER the OpenGL context has been created

	glViewport(0, 0, width, height);

	//
	// create shader program
	//

	// compile vertex shader source
    std::string vertexShaderSource =
		"#version 330\n\
		 uniform mat4 mvpMatrix;\
         uniform vec4 color;\
         in vec3 vpos;\
         out vec4 vcolor;\
		 void main(void) {\
			gl_Position = mvpMatrix * vec4(vpos, 1.0f);\
			vcolor = color;\
		}";
	Shader<GL_VERTEX_SHADER> vertexShader(vertexShaderSource);

	// compile fragment shader source
    std::string fragmentShaderSource = 
		"#version 330\n\
		 in vec4 vcolor;\
         out vec4 fcolor;\
		 void main(void) {\
			fcolor = vcolor;\
		 }";
	Shader<GL_FRAGMENT_SHADER> fragmentShader(fragmentShaderSource);

	// link shader program
    GLuint programId = glCreateProgram();
    glAttachShader(programId, vertexShader.id);
    glAttachShader(programId, fragmentShader.id);
    // associates the "inPosition" variable from the vertex shader with the position attribute
    // the variable and the attribute must be bound before the program is linked
    glBindAttribLocation(programId, POSITION_ATTRIBUTE_INDEX, "position");
    glLinkProgram(programId);

	//
	// create the triangle vertex buffer
	//
	GLuint linesId;
    float linesVertices[] = {
            -0.9f, -0.8f, 0.0f,
            -0.9f, -0.9f, 0.0f,
            -0.9f, -0.9f, 0.0f,
            -0.8f, -0.9f, 0.0f,

            0.9f, -0.8f, 0.0f,
            0.9f, -0.9f, 0.0f,
            0.9f, -0.9f, 0.0f,
            0.8f, -0.9f, 0.0f,

            -0.9f, 0.8f, 0.0f,
            -0.9f, 0.9f, 0.0f,
            -0.9f, 0.9f, 0.0f,
            -0.8f, 0.9f, 0.0f,

            0.9f, 0.8f, 0.0f,
            0.9f, 0.9f, 0.0f,
            0.9f, 0.9f, 0.0f,
            0.8f, 0.9f, 0.0f
	};
    glGenBuffers(1, &linesId);
    glBindBuffer(GL_ARRAY_BUFFER, linesId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(linesVertices), linesVertices, GL_STATIC_DRAW);

	//
	// defines the orthographic projection matrix
	//
	Matrix44<float> mat = Matrix44<float>::Ortho(width, 0, height, 0, 1.0f, -1.0f);

	//
	// SDL main loop
	//
    SDL_Event event;
    bool done = false;
	bool fullscreen = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT: 
				done = true;
				break;
			case SDL_KEYDOWN:
				SDL_KeyboardEvent* key = &event.key;
				std::cout << (char)key->keysym.sym << std::endl;
				if ((char)key->keysym.sym == 'f') {
					fullscreen = !fullscreen;
					if (fullscreen) {
						SDL_SetWindowFullscreen(win, SDL_WINDOW_FULLSCREEN_DESKTOP);
						int w, h;
						SDL_GetWindowSize(win, &w, &h);
						glViewport(0, 0, w, h);
					} else {
						SDL_SetWindowFullscreen(win, 0);
						SDL_SetWindowSize(win, width, height);
						glViewport(0, 0, width, height);
					}
				}
				break;
            }
        }

		//
		// rendering
		//

		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(programId);

		GLuint matrixUniform = glGetUniformLocation(programId, "mvpMatrix");
		glUniformMatrix4fv(matrixUniform, 1, false, mat.m);

		// we need the location of the uniform in order to set its value
		GLuint color = glGetUniformLocation(programId, "color");

		// render the triangle in yellow
		glUniform4f(color, 1.0f, 1.0f, 0.0f, 0.7f);
		glEnableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);
		glBindBuffer(GL_ARRAY_BUFFER, linesId);
		glVertexAttribPointer(POSITION_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glDrawArrays(GL_LINES, 0, 16);
		glDisableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);

		SDL_GL_SwapWindow(win);
    }

	SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}
