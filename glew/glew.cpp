#include <string.h>
#include <SDL.h>
#include <GL/glew.h>

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
    SDL_Window* win = SDL_CreateWindow("GLEW Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	SDL_GLContext ctx = SDL_GL_CreateContext(win);
	glewInit(); // must be called AFTER the OpenGL context has been created

	glViewport(0, 0, width, height);

	//
	// create shader program
	//

	// compile vertex shader source
    const GLchar* vertexShaderSource =
		"#version 330\n\
		 uniform mat4 mvpMatrix;\
         uniform vec4 color;\
         in vec3 vpos;\
         out vec4 vcolor;\
		 void main(void) {\
			gl_Position = mvpMatrix * vec4(vpos, 1.0f);\
			vcolor = color;\
		}";
    int vertexShaderSourceLength = strlen(vertexShaderSource);
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShaderId, 1, &vertexShaderSource, &vertexShaderSourceLength);
    glCompileShader(vertexShaderId);

	// compile fragment shader source
    const GLchar* fragmentShaderSource = 
		"#version 330\n\
		 in vec4 vcolor;\
         out vec4 fcolor;\
		 void main(void) {\
			fcolor = vcolor;\
		 }";
    int fragmentShaderSourceLength = strlen(fragmentShaderSource);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShaderId, 1, &fragmentShaderSource, &fragmentShaderSourceLength);
    glCompileShader(fragmentShaderId);

	// link shader program
    GLuint programId = glCreateProgram();
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);
    // associates the "inPosition" variable from the vertex shader with the position attribute
    // the variable and the attribute must be bound before the program is linked
    glBindAttribLocation(programId, POSITION_ATTRIBUTE_INDEX, "position");
    glLinkProgram(programId);

	//
	// create the triangle vertex buffer
	//
	GLuint trianglesId;
    float triangleVertices[] = {
            -0.5f, -0.5f, 0.0f,
            1.0f, -0.5f, 0.0f,
            -0.5f, 1.0f, 0.0f
    };
    glGenBuffers(1, &trianglesId);
    glBindBuffer(GL_ARRAY_BUFFER, trianglesId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(triangleVertices), triangleVertices, GL_STATIC_DRAW);

	//
	// create the quad vertex buffer
	//
	GLuint quadId;
    float quadVertices[] = {
            0.5f, 0.5f, 0.0f,
            -1.0f, 0.5f, 0.0f,
            0.5f, -1.0f, 0.0f,
            0.5f, -1.0f, 0.0f,
            -1.0f, -1.0f, 0.0f,
            -1.0f, 0.5f, 0.0f
    };
    glGenBuffers(1, &quadId);
    glBindBuffer(GL_ARRAY_BUFFER, quadId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), quadVertices, GL_STATIC_DRAW);

	//
	// defines the orthographic projection matrix
	//
	const float left = -1.5f;
	const float right = 1.5f;
	const float bottom = -1.5f / aspectRatio;
	const float top = 1.5f / aspectRatio;
	const float nearPlane = 1.0f;
	const float farPlane = -1.0f;

	float m[16];
	m[0] = 2 / (right - left);
    m[1] = 0.0f;
    m[2] = 0.0f;
    m[3] = 0.0f;
    m[4] = 0.0f;
    m[5] = 2 / (top - bottom);
    m[6] = 0.0f;
    m[7] = 0.0f;
    m[8] = 0.0f;
    m[9] = 0.0f;
    m[10] = 2 / (farPlane - nearPlane);
    m[11] = 0.0f;
    m[12] = -(right + left) / (right - left);
    m[13] = -(top + bottom) / (top - bottom);
    m[14] = -(farPlane + nearPlane) / (farPlane - nearPlane);
    m[15] = 1.0f;

	//
	// SDL main loop
	//
    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                done = true;
            }
        }

		//
		// rendering
		//

		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(programId);

		GLuint matrixUniform = glGetUniformLocation(programId, "mvpMatrix");
		glUniformMatrix4fv(matrixUniform, 1, false, m);

		// we need the location of the uniform in order to set its value
		GLuint color = glGetUniformLocation(programId, "color");

		// render the triangle in yellow
		glUniform4f(color, 1.0f, 1.0f, 0.0f, 0.7f);
		glEnableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);
		glBindBuffer(GL_ARRAY_BUFFER, trianglesId);
		glVertexAttribPointer(POSITION_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glDrawArrays(GL_TRIANGLES, 0, 3);
		glDisableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);

		// render the quad in blue
		glUniform4f(color, 0.2f, 0.2f, 1.0f, 0.7f);
		glEnableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);
		glBindBuffer(GL_ARRAY_BUFFER, quadId);
		glVertexAttribPointer(POSITION_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glDrawArrays(GL_TRIANGLES, 0, 6);
		glDisableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);

		SDL_GL_SwapWindow(win);
    }

	SDL_GL_DeleteContext(ctx);
    SDL_DestroyWindow(win);
    SDL_Quit();

    return 0;
}
