#include <stdio.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#define WINDOW_MIN_WIDTH  400
#define WINDOW_MIN_HEIGHT 150

#define FONT_PATH "/usr/share/fonts/TTF/DejaVuSans.ttf"

/* TODO: make the drawn image look a little less shit (color profile is off, aliased pixels) */
int draw_image(SDL_Texture *img, SDL_Renderer *r) {
	int w = 0, h = 0;
	SDL_GetRendererOutputSize(r, &w, &h);
	int min_dimension = w;
	if (h < min_dimension)
		min_dimension = h;

	SDL_Rect blit_area = { 0, 0, min_dimension, min_dimension };
	SDL_RenderCopy(r, img, NULL, &blit_area);
	return min_dimension;
}

void draw_text(TTF_Font *font, char *s, SDL_Renderer *r, int x) {
	int h = 0;
	SDL_GetRendererOutputSize(r, NULL, &h);
	SDL_Rect pos = {0};
	pos.x = x;
	pos.y = h / 2;
	SDL_Color white = { 255, 255, 255, 255 };
	SDL_Surface *text_surface = TTF_RenderText_Blended(font, s, white);
	pos.w = text_surface->w;
	pos.h = text_surface->h;
	SDL_Texture *text = SDL_CreateTextureFromSurface(r, text_surface);
	SDL_RenderCopy(r, text, NULL, &pos);
	SDL_DestroyTexture(text);
	SDL_FreeSurface(text_surface);
}

void draw_everything(SDL_Renderer *r, SDL_Texture *img, TTF_Font *font, char *s) {
	SDL_RenderClear(r);
	int x = draw_image(img, r);
	draw_text(font, s, r, x + 10);
	SDL_RenderPresent(r);
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
	SDL_Surface *image_surface = IMG_Load(argv[1]);
	if (!image_surface) {
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

	SDL_Window *w;
	SDL_Renderer *r;
	if (SDL_CreateWindowAndRenderer(WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT, SDL_WINDOW_RESIZABLE, &w, &r)) {
		SDL_Log("error creating window: %s", SDL_GetError());
		return 1;
	}
	SDL_SetWindowMinimumSize(w, WINDOW_MIN_WIDTH, WINDOW_MIN_HEIGHT);

	SDL_Texture *image = SDL_CreateTextureFromSurface(r, image_surface);
	if (!image) {
		SDL_Log("error loading image as texture: %s", SDL_GetError());
		return 1;
	}

	SDL_SetRenderDrawColor(r, 34, 34, 34, 255);
	draw_everything(r, image, font, argv[1]);

	SDL_Event e;
	while (1) {
		if (SDL_WaitEvent(&e)) {
			if (e.type == SDL_QUIT) {
				break;
			} else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_RESIZED) {
				draw_everything(r, image, font, argv[1]);
			} else if (e.type == SDL_WINDOWEVENT && e.window.event == SDL_WINDOWEVENT_EXPOSED) {
				draw_everything(r, image, font, argv[1]);
			}
		}
	}

	SDL_DestroyRenderer(r);
	SDL_DestroyWindow(w);
	SDL_DestroyTexture(image);
	SDL_FreeSurface(image_surface);
	IMG_Quit();
	TTF_CloseFont(font);
	TTF_Quit();
	SDL_Quit();
	return 0;
}
