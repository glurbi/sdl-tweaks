#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <map>
#include <memory>
#include <vector>
#include <numeric>
#include <string.h>
#include <sys/stat.h>
#include <SDL.h>
#include <SDL_ttf.h>
#include <GL/glew.h>

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
		TTF_Quit();
	    SDL_Quit();
	}
};

struct Win {
	SDL_Window* w;
	SDL_GLContext ctx;
	Win(std::string title, int width, int height) {
		w = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, width, height, SDL_WINDOW_OPENGL);
		ctx = SDL_GL_CreateContext(w);
		glewInit(); // must be called AFTER the OpenGL context has been created
		glViewport(0, 0, width, height);
	}
	void Show() {
		SDL_ShowWindow(w);
	}
	void MakeCurrent() {
		SDL_GL_MakeCurrent(w, ctx);
	}
	~Win() {
		SDL_GL_DeleteContext(ctx);
		SDL_DestroyWindow(w);
	}
};


int main(int argc, char **argv)
{
	App app;
	Win win1("Double Context 1", 640, 480);
	Win win2("Double Context 2", 800, 600);
	win1.Show();
	win2.Show();

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
		win1.MakeCurrent();
		glClearColor(1.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		win2.MakeCurrent();
		glClearColor(0.0f, 1.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		SDL_GL_SwapWindow(win1.w);
		SDL_GL_SwapWindow(win2.w);
    }

    return 0;
}
