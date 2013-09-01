#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <memory>
#include <vector>
#include <string.h>
#include <sys/stat.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <GL/glew.h>

// Up to 16 attributes per vertex is allowed so any value between 0 and 15 will do.
const int POSITION_ATTRIBUTE_INDEX = 12;
const int TEXCOORD_ATTRIBUTE_INDEX = 7;

struct App {
	App() {
		SDL_Init(SDL_INIT_EVERYTHING);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 1);
		SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_DEBUG_FLAG);
		SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
	}
	~App() {
	    SDL_Quit();
	}
};

struct Win {
	SDL_Window* w;
	SDL_GLContext ctx;
	Win(std::string title, int width, int height) {
		w = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);
		ctx = SDL_GL_CreateContext(w);
		glewExperimental=GL_TRUE;
		glewInit(); // must be called AFTER the OpenGL context has been created
		glViewport(0, 0, width, height);
	}
	void Show() {
		SDL_ShowWindow(w);
	}
	~Win() {
		SDL_GL_DeleteContext(ctx);
		SDL_DestroyWindow(w);
	}
};

std::string	readTextFile(const std::string& filename) {
	std::ifstream f(filename);
	std::stringstream buffer;
	buffer << f.rdbuf();
	return buffer.str();
}

template <class T>
struct Matrix44
{
	T m[16];
};

template <class T>
Matrix44<T> Ortho(T right, T left, T top, T bottom, T nearp, T farp) {
	Matrix44<T> mat;
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

template <int type>
struct Shader {
    GLuint id;
	Shader(std::string& source) {
		id = glCreateShader(type);
		const GLchar* str = source.c_str();
		const GLint length = source.length();
		glShaderSource(id, 1, &str, &length);
		glCompileShader(id);
	}
	~Shader() {
		glDeleteShader(id);
	}
};

struct Program {
    GLuint id;
	Program(const Shader<GL_VERTEX_SHADER>& vertexShader,
			const Shader<GL_FRAGMENT_SHADER>& fragmentShader,
			const std::map<int, std::string>& attributeIndices)
	{
		id = glCreateProgram();
		glAttachShader(id, vertexShader.id);
		glAttachShader(id, fragmentShader.id);
		for (auto it = attributeIndices.begin(); it != attributeIndices.end(); it++) {
			glBindAttribLocation(id, it->first, it->second.c_str());
		}
	    glLinkProgram(id);
	}
	~Program() {
		glDeleteProgram(id);
	}
};

struct Geometry {
	GLuint id;
	Geometry(void* data, long size) {
		glGenBuffers(1, &id);
		glBindBuffer(GL_ARRAY_BUFFER, id);
		glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
	}
	~Geometry() {
		glDeleteBuffers(1, &id);
	}
	void Render(const Program& program, const Matrix44<float>& mat) {
		glUseProgram(program.id);
		GLuint matrixUniform = glGetUniformLocation(program.id, "mvpMatrix");
		glUniformMatrix4fv(matrixUniform, 1, false, mat.m);

		// we need the location of the uniform in order to set its value
		GLuint color = glGetUniformLocation(program.id, "color");

		// render the triangle in yellow
		glUniform4f(color, 1.0f, 1.0f, 0.0f, 0.7f);
		glEnableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);
		glBindBuffer(GL_ARRAY_BUFFER, id);
		glVertexAttribPointer(POSITION_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glDrawArrays(GL_LINES, 0, 4);
		glDisableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);
	}
};

struct Texture {
	GLuint id;
	Texture(SDL_Surface* s) {
		SDL_Palette* palette = s->format->palette;
		char* p = (char*) s->pixels;
		GLubyte* data = new GLubyte[s->w * s->h * 4];
		GLubyte* t = data;
		for (int i=s->h; i > 0; i--) {
			for (int j=0; j < s->w; j++) {
				SDL_Color color = palette->colors[p[i*s->pitch+j]];
				*t++ = color.r;
				*t++ = color.g;
				*t++ = color.b;
				*t++ = 255;
			}
		}
		glEnable(GL_TEXTURE_2D);
		glGenTextures(1, &id);
		glBindTexture(GL_TEXTURE_2D, id);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, s->w, s->h, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);
	}
	~Texture() {
		glDeleteTextures(1, &id);
	}
};

struct Font {
	std::vector<Texture> letters;
	Font(const std::string filename) {
		TTF_Font* font = TTF_OpenFont(filename.c_str(), 32);
		SDL_Color text_color = { 255, 255, 255 };
		for (char c=32; c<127; c++){
			char str[2] = { ' ', ' ' };
			str[0] = c;
			SDL_Surface* letter = TTF_RenderText_Solid(font, str, text_color);
			letters[c] = Texture(letter);			
			SDL_FreeSurface(letter);
		}
	}
};

int main(int argc, char **argv)
{
	const int width = 800;
	const int height = 600;
	App app;
	Win win("FPS Test", width, height);
	win.Show();

	//
	// create monochrome shader program
	//
    std::string monochromeVertexShaderSource = readTextFile("monochrome.vert");
	Shader<GL_VERTEX_SHADER> monochromeVertexShader(monochromeVertexShaderSource);
    std::string monochromeFragmentShaderSource = readTextFile("monochrome.frag");
	Shader<GL_FRAGMENT_SHADER> monochromeFragmentShader(monochromeFragmentShaderSource);
	std::map<int, std::string> monochromeAttributeIndices;
	monochromeAttributeIndices[POSITION_ATTRIBUTE_INDEX] = "vpos";
	Program monochromeProgram(monochromeVertexShader, monochromeFragmentShader, monochromeAttributeIndices);

	//
	// create texture shader program
	//
    std::string textureVertexShaderSource = readTextFile("texture.vert");
	Shader<GL_VERTEX_SHADER> textureVertexShader(textureVertexShaderSource);
    std::string textureFragmentShaderSource = readTextFile("texture.frag");
	Shader<GL_FRAGMENT_SHADER> textureFragmentShader(textureFragmentShaderSource);
	std::map<int, std::string> textureAttributeIndices;
	textureAttributeIndices[POSITION_ATTRIBUTE_INDEX] = "pos";
	textureAttributeIndices[POSITION_ATTRIBUTE_INDEX] = "texCoord";
	Program textureProgram(textureVertexShader, textureFragmentShader, textureAttributeIndices);

	//
	// create the geometry
	//
    float linesVertices[] = {
            0.0f, height/2, 0.0f,
            width, height/2, 0.0f,
            width/2, 0.0f, 0.0f,
            width/2, height, 0.0f,
	};
    Geometry geometry(linesVertices, sizeof(linesVertices));

	//
	// defines the orthographic projection matrix
	//
	Matrix44<float> mat = Ortho<float>(width, 0, height, 0, 1.0f, -1.0f);

	//
	// SDL main loop
	//
    SDL_Event event;
    bool done = false;
    while (!done) {

		while (SDL_PollEvent(&event)) {
			switch (event.type) {
			case SDL_QUIT: 
				done = true;
				break;
            }
        }

		//
		// rendering
		//
		glClear(GL_COLOR_BUFFER_BIT);
		geometry.Render(monochromeProgram, mat);
		SDL_GL_SwapWindow(win.w);
    }

    return 0;
}
