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
		TTF_Init();
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

struct Geometry {
	GLuint positionsId;
	GLuint texCoordsId;
	Geometry() {
		positionsId = 0;
        texCoordsId = 0;
	}
	void SetVertexPositions(void* data, long size) {
		glGenBuffers(1, &positionsId);
		glBindBuffer(GL_ARRAY_BUFFER, positionsId);
		glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
	}
    void SetVertexTexCoords(void* data, long size) {
		glGenBuffers(1, &texCoordsId);
		glBindBuffer(GL_ARRAY_BUFFER, texCoordsId);
		glBufferData(GL_ARRAY_BUFFER, size, data, GL_STATIC_DRAW);
    }
	~Geometry() {
		glDeleteBuffers(1, &positionsId);
		glDeleteBuffers(1, &texCoordsId);
	}
};

struct Texture {
	GLuint id;
	Texture() {}
	Texture(SDL_Surface* s) {

        	if (s->format->BitsPerPixel != 8) {
		std::cout << "Pixel format not supported (" << s->format->BitsPerPixel << ")" << std::endl;
		exit(EXIT_FAILURE);
	}

		SDL_Palette* palette = s->format->palette;
		char* p = (char*) s->pixels;
		GLubyte* data = new GLubyte[s->w * s->h * 4];
		GLubyte* t = data;
		for (int i=s->h; i > 0; i--) {
			for (int j=0; j < s->w; j++) {
				SDL_Color color = palette->colors[p[i*s->pitch+j]];
				*t++ = 255;
				*t++ = 255;
				*t++ = 255;
				//*t++ = color.r;
				//*t++ = color.g;
				//*t++ = color.b;
				*t++ = 255;
			}
		}
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

template <int type>
struct Shader {
    GLuint id;
	Shader(const std::string& source) {
		id = glCreateShader(type);
		const GLchar* str = source.c_str();
		const GLint length = source.length();
		glShaderSource(id, 1, &str, &length);
		glCompileShader(id);
	}
	~Shader() {
		glDeleteShader(id);
	}
private:
    Shader(const Shader&);
};

struct Program {
    GLuint id;
    Shader<GL_VERTEX_SHADER> vertexShader;
    Shader<GL_FRAGMENT_SHADER> fragmentShader;
	Program(const std::string&  vertexShaderSource,
			const std::string&   fragmentShaderSource,
            const std::map<int, std::string>& attributeIndices)
    :
        vertexShader(vertexShaderSource),
        fragmentShader(fragmentShaderSource)
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
private:
    Program(const Program& that);
};

struct MonochromeProgram : public Program {
	void Render(const Geometry& geometry, const Matrix44<float>& mat) {
		glUseProgram(id);
		GLuint matrixUniform = glGetUniformLocation(id, "mvpMatrix");
		glUniformMatrix4fv(matrixUniform, 1, false, mat.m);
		GLuint color = glGetUniformLocation(id, "color");
		glUniform4f(color, 1.0f, 1.0f, 0.0f, 0.7f);
		glEnableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);
		glBindBuffer(GL_ARRAY_BUFFER, geometry.positionsId);
		glVertexAttribPointer(POSITION_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glDrawArrays(GL_LINES, 0, 4);
		glDisableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);
	}
    static std::shared_ptr<MonochromeProgram> Create() {
	    std::map<int, std::string> monochromeAttributeIndices;
	    monochromeAttributeIndices[POSITION_ATTRIBUTE_INDEX] = "vpos";
        return std::shared_ptr<MonochromeProgram>(new MonochromeProgram(monochromeAttributeIndices));
    }
private:
    MonochromeProgram(const std::map<int, std::string>& attributeIndices)
	: Program(readTextFile("monochrome.vert"), readTextFile("monochrome.frag"), attributeIndices) {}
};

struct TextureProgram : public Program {
	void Render(const Geometry& geometry, const Texture& texture, const Matrix44<float>& mat) {
		glUseProgram(id);
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, texture.id);
		GLuint matrixUniform = glGetUniformLocation(id, "mvpMatrix");
		glUniformMatrix4fv(matrixUniform, 1, false, mat.m);
		GLuint textureUniform = glGetUniformLocation(id, "texture");
		glUniform1i(textureUniform, 0); // we pass the texture unit
		glEnableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);
        glBindBuffer(GL_ARRAY_BUFFER, geometry.positionsId);
		glVertexAttribPointer(POSITION_ATTRIBUTE_INDEX, 3, GL_FLOAT, GL_FALSE, 0, 0);
		glEnableVertexAttribArray(TEXCOORD_ATTRIBUTE_INDEX);
        glBindBuffer(GL_ARRAY_BUFFER, geometry.texCoordsId);
		glVertexAttribPointer(TEXCOORD_ATTRIBUTE_INDEX, 2, GL_FLOAT, GL_FALSE, 0, 0);
		glDrawArrays(GL_QUADS, 0, 4);
		glDisableVertexAttribArray(POSITION_ATTRIBUTE_INDEX);
		glDisableVertexAttribArray(TEXCOORD_ATTRIBUTE_INDEX);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
//                if (int err = glGetError() != GL_NO_ERROR) { std::cout << "error:" << err << std::endl; }

	}
    static std::shared_ptr<TextureProgram> Create() {
	    std::map<int, std::string> textureAttributeIndices;
	    textureAttributeIndices[POSITION_ATTRIBUTE_INDEX] = "pos";
	    textureAttributeIndices[TEXCOORD_ATTRIBUTE_INDEX] = "texCoord";
        return std::shared_ptr<TextureProgram>(new TextureProgram(textureAttributeIndices));
    }
private:
	TextureProgram(std::map<int, std::string>& attributeIndices)
	: Program(readTextFile("texture.vert"), readTextFile("texture.frag"), attributeIndices) {}
};

struct Font {
	std::vector<Texture> letters;
	Font(const std::string& filename) {
		letters.reserve(128);
		letters.resize(128);
		TTF_Font* font = TTF_OpenFont(filename.c_str(), 32);
		SDL_Color text_color = { 255, 255, 255 };
		for (char c=32; c<127; c++){
			char str[2] = { ' ', '\0' };
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
	Font font("arial.ttf");
	win.Show();

	Matrix44<float> mat = Ortho<float>(width, 0, height, 0, 1.0f, -1.0f);
    std::shared_ptr<MonochromeProgram> monochromeProgram = MonochromeProgram::Create();
    std::shared_ptr<TextureProgram> textureProgram = TextureProgram::Create();

	//
	// create the geometry
	//
    Geometry myGeometry;
    float linesVertices[] = {
            0.0f, height/2, 0.0f,
            width, height/2, 0.0f,
            width/2, 0.0f, 0.0f,
            width/2, height, 0.0f,
	};
	myGeometry.SetVertexPositions(linesVertices, sizeof(linesVertices));

    Geometry myTextBox;
    float positions[] = {
        10.0f, 10.0f, 0.0f,
        50.0f, 10.0f, 0.0f,
        50.0f, 100.0f, 0.0f,
        10.0f, 100.0f, 0.0f
    };
	float texcoords[] = {
		1.0f, 1.0f,
		1.0f, 0.0f,
		0.0f, 0.0f,
		0.0f, 1.0f
	};
    myTextBox.SetVertexPositions(positions, sizeof(positions));
    myTextBox.SetVertexTexCoords(texcoords, sizeof(texcoords));

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
		monochromeProgram->Render(myGeometry, mat);
        textureProgram->Render(myTextBox, font.letters['d'], mat);
		SDL_GL_SwapWindow(win.w);
    }

    return 0;
}
