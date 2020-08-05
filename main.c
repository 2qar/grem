#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#define WINDOW_MIN_WIDTH  400
#define WINDOW_MIN_HEIGHT 150

#define FONT_PATH "/usr/share/fonts/TTF/DejaVuSans.ttf"

int draw_image(SDL_Surface *img, SDL_Window *w) {
	SDL_Surface *window_surface = SDL_GetWindowSurface(w);

	int min_dimension = window_surface->w;
	if (window_surface->h < min_dimension)
		min_dimension = window_surface->h;

	SDL_Rect blit_area = { 0, 0, min_dimension, min_dimension };
	SDL_BlitScaled(img, NULL, window_surface, &blit_area);
	SDL_UpdateWindowSurface(w);
	return min_dimension;
}

void draw_text(TTF_Font *font, char *s, SDL_Window *w, int x) {
	SDL_Surface *window_surface = SDL_GetWindowSurface(w);
	SDL_Rect pos = {0};
	pos.x = x;
	pos.y = window_surface->h / 2;
	SDL_Color white = { 255, 255, 255, 255 };
	SDL_Surface *text = TTF_RenderText_Solid(font, s, white);
	SDL_BlitSurface(text, NULL, window_surface, &pos);
	SDL_UpdateWindowSurface(w);
}

int main(int argc, char **argv) {
	/* TODO: watch a file for artist, track name and image path, 3 lines total 
	 *       maybe use inotify(7)? not portable tho */
	if (argc != 2) {
		fprintf(stderr, "usage: grem [IMAGE_PATH]\n");
		return 1;
	}
	puts(argv[1]);

	if (!IMG_Init(IMG_INIT_JPG)) {
		fprintf(stderr, "error initializing SDL_image: %s\n", IMG_GetError());
		return 1;
	}
	SDL_Surface *image = IMG_Load(argv[1]);
	if (!image) {
		fprintf(stderr, "error loading image: %s\n", IMG_GetError());
		return 1;
	}

	if (TTF_Init() < 0) {
		fprintf(stderr, "error initializing SDL_ttf: %s\n", TTF_GetError());
		return 1;
	}
	TTF_Font *font = TTF_OpenFont(FONT_PATH, 16);
	if (!font) {
		fprintf(stderr, "error opening font: %s\n", TTF_GetError());
		return 1;
	}

	if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
		SDL_Log("error initializing SDL: %s", SDL_GetError());
		return 1;
	}

	SDL_Window *w = SDL_CreateWindow("grem", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT, SDL_WINDOW_RESIZABLE);
	if (!w) {
		SDL_Log("error creating window: %s", SDL_GetError());
		return 1;
	}
	SDL_SetWindowMinimumSize(w, WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT);

	int x = draw_image(image, w);
	draw_text(font, argv[1], w, x + 10);

	SDL_Event e;
	while (1) {
		if (SDL_WaitEvent(&e)) {
			if (e.type == SDL_QUIT) {
				break;
			} else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED) {
				x = draw_image(image, w);
				draw_text(font, argv[1], w, x + 10);
			} else if (e.type == SDL_KEYDOWN && e.key.keysym.scancode == SDL_SCANCODE_SPACE) {
				SDL_Surface *ws = SDL_GetWindowSurface(w);
				SDL_SetWindowSize(w, ws->w, ws->w);
				draw_image(image, w);
			}
		}
	}

	SDL_DestroyWindow(w);
	SDL_FreeSurface(image);
	IMG_Quit();
	TTF_CloseFont(font);
	TTF_Quit();
	SDL_Quit();
	return 0;
}
