#include <iostream>
#include <sys/stat.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <GL/glew.h>

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

int main(int argc, char** argv)
{
	// Up to 16 attributes per vertex is allowed so any value between 0 and 15 will do.
	const int POSITION_ATTRIBUTE_INDEX = 12;
	const int TEXCOORD_ATTRIBUTE_INDEX = 7;

	const int width = 800;
	const int height = 600;
	const float aspectRatio = 1.0f * width / height;

	SDL_Init(SDL_INIT_EVERYTHING);
	TTF_Init();
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
	SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
	SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	SDL_Window *win = SDL_CreateWindow("Mix Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 800, 600, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);

	SDL_GLContext ctx = SDL_GL_CreateContext(win);
	glewInit(); // must be called AFTER the OpenGL context has been created
    glViewport(0, 0, width, height);

	SDL_Renderer* renderer = SDL_CreateRenderer(win, -1, 0);
	TTF_Font* font = TTF_OpenFont("arial.ttf", 128);
	SDL_Color text_color = {255, 255, 255};
	SDL_Surface* text = TTF_RenderText_Solid(font, "Hello, SDL!", text_color);

	//
	// create the shader program
	//

	// compile the vertex shader
    const GLchar* vertexShaderSource = readTextFile("mix.vert");
    int vertexShaderSourceLength = strlen(vertexShaderSource);
    GLuint vertexShaderId = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShaderId, 1, &vertexShaderSource, &vertexShaderSourceLength);
    glCompileShader(vertexShaderId);

	// compile the fragment shader
    const GLchar* fragmentShaderSource = readTextFile("mix.frag");
    int fragmentShaderSourceLength = strlen(fragmentShaderSource);
    GLuint fragmentShaderId = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShaderId, 1, &fragmentShaderSource, &fragmentShaderSourceLength);
    glCompileShader(fragmentShaderId);

	// link the shader program
	GLuint programId = glCreateProgram();
    glAttachShader(programId, vertexShaderId);
    glAttachShader(programId, fragmentShaderId);
    // associates the "pos" variable from the vertex shader with the position attribute
    // the variable and the attribute must be bound before the program is linked
    glBindAttribLocation(programId, POSITION_ATTRIBUTE_INDEX, "pos");
    glBindAttribLocation(programId, TEXCOORD_ATTRIBUTE_INDEX, "texCoord");
    glLinkProgram(programId);

	//
	// create quad buffer
	//

	// create positions
    float positions[] = {
        1.4f, 1.0f, 0.0f,
        1.4f, -1.0f, 0.0f,
        -1.4f, -1.0f, 0.0f,
        -1.4f, 1.0f, 0.0f
    };
	GLuint quadId;
    glGenBuffers(1, &quadId);
    glBindBuffer(GL_ARRAY_BUFFER, quadId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(positions), positions, GL_STATIC_DRAW);

	// create texture coordinates
	float texcoords[] = {
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 1.0f
	};
	GLuint quadTexId;
	glGenBuffers(1, &quadTexId);
	glBindBuffer(GL_ARRAY_BUFFER, quadTexId);
    glBufferData(GL_ARRAY_BUFFER, sizeof(texcoords), texcoords, GL_STATIC_DRAW);

	//
	// create the texture
	//

	if (text->format->BitsPerPixel != 8) {
		std::cout << "Pixel format not supported (" << text->format->BitsPerPixel << ")" << std::endl;
		exit(EXIT_FAILURE);
	}
	SDL_Palette* palette = text->format->palette;
	char* p = (char*) text->pixels;
    GLubyte* textureData = new GLubyte[text->w * text->h * 4];
	GLubyte* t = textureData;
	//for (int i=0; i < text->h; i++) {
	for (int i=text->h; i > 0; i--) {
		for (int j=0; j < text->w; j++) {
			SDL_Color color = palette->colors[p[i*text->pitch+j]];
			*t++ = color.r;
			*t++ = color.g;
			*t++ = color.b;
			*t++ = 255;
		}
	}

	GLuint textureId;
    glGenTextures(1, &textureId);
    glBindTexture(GL_TEXTURE_2D, textureId);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, text->w, text->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, textureData);
	SDL_FreeSurface(text);

	//
	// defines the orthographic projection matrix
	//
	float m[16];
	const float left = -1.5f;
	const float right = 1.5f;
	const float bottom = -1.5f;
	const float top = 1.5f;
	const float nearp = 1.0f;
	const float farp = -1.0f;
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
    m[10] = 2 / (farp - nearp);
    m[11] = 0.0f;
    m[12] = -(right + left) / (right - left);
    m[13] = -(top + bottom) / (top - bottom);
    m[14] = -(farp + nearp) / (farp - nearp);
    m[15] = 1.0f;

	//
	// SDL main loop
	//
	SDL_Event event;
	while (1) {

		//
		// handle events
		//
		if (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				break;
			}
		}

		//
		// rendering
		//

		glUseProgram(programId);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, textureId);

		GLuint matrixUniform = glGetUniformLocation(programId, "mvpMatrix");
		glUniformMatrix4fv(matrixUniform, 1, false, m);
		GLuint textureUniform = glGetUniformLocation(programId, "texture");
		glUniform1i(textureUniform, 0);

		glEnableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);
		glBindBuffer(GL_ARRAY_BUFFER, quadId);
		glVertexAttribPointer(POSITION_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(TEXCOORD_ATTRIBUTE_INDEX);
		glBindBuffer(GL_ARRAY_BUFFER, quadTexId);
		glVertexAttribPointer(TEXCOORD_ATTRIBUTE_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glDrawArrays(GL_QUADS, 0, 4);
		glDisableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);
		glDisableVertexAttribArray(TEXCOORD_ATTRIBUTE_INDEX);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		SDL_GL_SwapWindow(win);
	}

	SDL_DestroyRenderer(renderer);
	SDL_GL_DeleteContext(ctx);
	SDL_DestroyWindow(win);

	TTF_Quit();
	SDL_Quit();
    
	return 0;    
}
