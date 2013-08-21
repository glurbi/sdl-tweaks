#include <SDL.h>
#include <SDL_ttf.h>

int main(int argc, char** argv)
{
	SDL_Init(SDL_INIT_EVERYTHING);
	TTF_Init();
	SDL_Window *win = SDL_CreateWindow("TTF Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1000, 600, SDL_WINDOW_SHOWN);
	SDL_Renderer* renderer = SDL_CreateRenderer(win, -1, 0);
	TTF_Font* font = TTF_OpenFont("arial.ttf", 512);
	SDL_Color text_color = {255, 255, 255};
	SDL_Surface *text = TTF_RenderText_Solid(font, "Hello SDL!", text_color);
    SDL_Texture *tex = SDL_CreateTextureFromSurface(renderer, text);
	SDL_FreeSurface(text);

	SDL_Event event;
	while (1) {
		if (SDL_PollEvent(&event)) {
			if (event.type == SDL_QUIT) {
				break;
			}
		}
		SDL_RenderClear(renderer);
		SDL_RenderCopy(renderer, tex, NULL, NULL);
		SDL_RenderPresent(renderer);
   }

	SDL_DestroyTexture(tex);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(win);

	TTF_Quit();
	SDL_Quit();
    
	return 0;    
}
