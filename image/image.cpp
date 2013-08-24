#include <SDL.h>
#include <SDL_image.h>

// image: www.freeimages.co.uk

int main(int argc, char** argv)
{
	SDL_Init(SDL_INIT_EVERYTHING);
	IMG_Init(IMG_INIT_JPG);
	SDL_Window *win = SDL_CreateWindow("Image Test", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1024, 768, SDL_WINDOW_SHOWN);
	SDL_Renderer* renderer = SDL_CreateRenderer(win, -1, 0);
	SDL_RWops* rwop = SDL_RWFromFile("beach.jpg", "rb");
	SDL_Surface* image = IMG_LoadJPG_RW(rwop);
    SDL_Texture* tex = SDL_CreateTextureFromSurface(renderer, image);
	SDL_FreeSurface(image);

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

	SDL_Quit();
    
	return 0;    
}
