#include <sys/stat.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <SDL.h>
#include <GL/glew.h>

typedef struct {
	float m[16];
} matrix44;

// Up to 16 attributes per vertex is allowed so any value between 0 and 15 will do.
const int POSITION_ATTRIBUTE_INDEX = 0;

GLuint trianglesId;
GLuint quadId;
GLuint programId;

matrix44 ortho(float left, float right, float bottom, float top, float nearp, float farp) {
	matrix44 m;
    m.m[0] = 2 / (right - left);
    m.m[1] = 0.0f;
    m.m[2] = 0.0f;
    m.m[3] = 0.0f;
    m.m[4] = 0.0f;
    m.m[5] = 2 / (top - bottom);
    m.m[6] = 0.0f;
    m.m[7] = 0.0f;
    m.m[8] = 0.0f;
    m.m[9] = 0.0f;
    m.m[10] = 2 / (farp - nearp);
    m.m[11] = 0.0f;
    m.m[12] = -(right + left) / (right - left);
    m.m[13] = -(top + bottom) / (top - bottom);
    m.m[14] = -(farp + nearp) / (farp - nearp);
    m.m[15] = 1.0f;
	return m;
}

char* readTextFile(const char* filename) {
    struct stat st;
    stat(filename, &st);
    int size = st.st_size;
    char* content = (char*) malloc((size+1)*sizeof(char));
    content[size] = 0;
    // we need to read as binary, not text, otherwise we are screwed on Windows
    FILE *file = fopen(filename, "rb");
    fread(content, 1, size, file);
    return content;
}

void checkShaderCompileStatus(GLuint shaderId) {
    GLint compileStatus;
    glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compileStatus);
    if (compileStatus == GL_FALSE) {
        GLint infoLogLength;
        glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &infoLogLength);
        printf("Shader compilation failed...\n");
        char* log = (char*) malloc((1+infoLogLength)*sizeof(char));
        glGetShaderInfoLog(shaderId, infoLogLength, NULL, log);
        log[infoLogLength] = 0;
        printf("%s", log);
    }
}

void checkProgramLinkStatus(GLuint programId) {
    GLint linkStatus;
    glGetProgramiv(programId, GL_LINK_STATUS, &linkStatus);
    if (linkStatus == GL_FALSE) {
        GLint infoLogLength;
        glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &infoLogLength);
        printf("Program link failed...\n");
        char* log = (char*) malloc((1+infoLogLength)*sizeof(char));
        glGetProgramInfoLog(programId, infoLogLength, NULL, log);
        log[infoLogLength] = 0;
        printf("%s", log);
    }
}

void createProgram() {
    const GLchar* vertexShaderSource = readTextFile("glewtest.vert");
    int vertexShaderSourceLength = strlen(vertexShaderSource);
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShaderId, 1, &vertexShaderSource, &vertexShaderSourceLength);
    glCompileShader(vertexShaderId);
	checkShaderCompileStatus(vertexShaderId);

    const GLchar* fragmentShaderSource = readTextFile("glewtest.frag");
    int fragmentShaderSourceLength = strlen(fragmentShaderSource);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShaderId, 1, &fragmentShaderSource, &fragmentShaderSourceLength);
    glCompileShader(fragmentShaderId);
	checkShaderCompileStatus(fragmentShaderId);

    programId = glCreateProgram();
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);
    // associates the "inPosition" variable from the vertex shader with the position attribute
    // the variable and the attribute must be bound before the program is linked
    glBindAttribLocation(programId, POSITION_ATTRIBUTE_INDEX, "position");
    glLinkProgram(programId);
    checkProgramLinkStatus(programId);
}

int main(int argc, char **argv)
{
	const int width = 800;
	const int height = 600;
    const float aspectRatio = 1.0f * width / height;

	SDL_Init(SDL_INIT_EVERYTHING);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
    SDL_Window* win = SDL_CreateWindow("GLEW Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
	SDL_GLContext ctx = SDL_GL_CreateContext(win);
	glewInit(); // must be called AFTER the OpenGL context has been created

	glViewport(0, 0, width, height);
    // we keep track of the aspect ratio to adjust the projection volume

    createProgram();

	//
	// create the triangle vertex buffer
	//
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

	// defines the orthographic projection volume
	const float left = -1.5f;
	const float right = 1.5f;
	const float bottom = -1.5f;
	const float top = 1.5f;
	const float nearPlane = 1.0f;
	const float farPlane = -1.0f;

    // defines the model view projection matrix and set the corresponding uniform
    // NB: bottom and top are adjusted with the aspect ratio
    matrix44 mvp = ortho(left, right, bottom / aspectRatio, top / aspectRatio, nearPlane, farPlane);

    SDL_Event event;
    bool done = false;
    while (!done) {
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT) {
                done = true;
            }
        }
		glEnable(GL_BLEND);
		glBlendFunc(GL_ONE, GL_ONE_MINUS_SRC_ALPHA);
		glClear(GL_COLOR_BUFFER_BIT);
		glUseProgram(programId);

		GLuint matrixUniform = glGetUniformLocation(programId, "mvpMatrix");
		glUniformMatrix4fv(matrixUniform, 1, false, mvp.m);

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
