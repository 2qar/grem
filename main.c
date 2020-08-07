#include <stdio.h>
#include <unistd.h>
#include <sys/inotify.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>

#define SONG_TITLE_FONT_SIZE 24
#define ARTIST_FONT_SIZE     16

#define WINDOW_MIN_WIDTH  400
#define WINDOW_MIN_HEIGHT 150

#define FONT_PATH "/usr/share/fonts/TTF/DejaVuSans.ttf"

int load_image(SDL_Renderer *r, char *path, SDL_Texture **t) {
	SDL_Surface *s = IMG_Load(path);
	if (!s) {
		fprintf(stderr, "error loading image: %s\n", IMG_GetError());
		return 1;
	}
	*t = SDL_CreateTextureFromSurface(r, s);
	if (!t) {
		SDL_Log("error loading image as texture: %s", SDL_GetError());
		return 1;
	}
	SDL_FreeSurface(s);
	return 0;
}

/* TODO: make the drawn image look a little less shit (color profile is off, aliased pixels) */
int draw_image(SDL_Renderer *r, SDL_Texture *img) {
	int w = 0, h = 0;
	SDL_GetRendererOutputSize(r, &w, &h);
	int min_dimension = w;
	if (h < min_dimension)
		min_dimension = h;

	SDL_Rect blit_area = { 0, 0, min_dimension, min_dimension };
	SDL_RenderCopy(r, img, NULL, &blit_area);
	return min_dimension;
}

struct text_prop {
	char *text;
	TTF_Font *font;
	int x, y;
};

void draw_text(SDL_Renderer *r, struct text_prop *tp) {
	SDL_Rect pos = { tp->x, tp->y, 0, 0 };
	SDL_Color white = { 255, 255, 255, 255 };
	SDL_Surface *text_surface = TTF_RenderText_Blended(tp->font, tp->text, white);
	pos.w = text_surface->w;
	pos.h = text_surface->h;
	pos.y -= pos.h / 2;
	SDL_Texture *text = SDL_CreateTextureFromSurface(r, text_surface);
	SDL_FreeSurface(text_surface);
	SDL_RenderCopy(r, text, NULL, &pos);
	SDL_DestroyTexture(text);
}

struct playing {
	char *song;
	char *artist;
	char *cover_path;
};

struct view {
	TTF_Font *song_font;
	TTF_Font *artist_font;
	SDL_Texture *cover;
};

void draw_everything(SDL_Renderer *r, struct view *v, struct playing *p) {
	SDL_RenderClear(r);
	struct text_prop tp;
	tp.x = draw_image(r, v->cover) + 10;
	int w = 0, h = 0;
	SDL_GetRendererOutputSize(r, &w, &h);

	tp.text = p->song;
	tp.font = v->song_font;
	tp.y = h *1/4;
	draw_text(r, &tp);

	tp.text = p->artist;
	tp.font = v->artist_font;
	tp.y = h * 3/4;
	draw_text(r, &tp);

	SDL_RenderPresent(r);
}

int read_playing(struct playing *p, char *path, int buf_len, char **buf) {
	FILE *f = fopen(path, "r");
	if (!f) {
		perror("error opening file");
		return 1;
	}
	fseek(f, 0L, SEEK_END);
	int f_len = ftell(f);
	if (f_len > buf_len || *buf == NULL)
		// possible undefined behaviour w/ cover_path's name, idk
		*buf = realloc(*buf, f_len);
	rewind(f);
	if (fread(*buf, sizeof(char), f_len, f) < (size_t)f_len) {
		perror("fread");
		return 0;
	}
	fclose(f);

	p->song = strtok(*buf, "\n");
	p->artist = strtok(NULL, "\n");
	p->cover_path = strtok(NULL, "\n");
	if (p->cover_path == NULL) {
		fprintf(stderr, "song file has missing lines\n");
		return 0;
	}

	return f_len;
}

int watch_file(void *ifd) {
	int *in_fd = (int *) ifd;
	struct inotify_event e;
	int n;
	while ((n = read(*in_fd, &e, sizeof(struct inotify_event))) > 0) {
		SDL_Event event;
		event.type = SDL_USEREVENT;
		SDL_PushEvent(&event);
	}
	return 0;
}

int main(int argc, char **argv) {
	if (argc != 2) {
		fprintf(stderr, "usage: grem [PLAYING_FILE]\n");
		return 1;
	}

	struct playing p = {0};
	char *buf = NULL;
	int buf_len = read_playing(&p, argv[1], 0, &buf);
	if (buf_len == 0)
		return 1;
	struct view v = {0};

	if (TTF_Init() < 0) {
		fprintf(stderr, "error initializing SDL_ttf: %s\n", TTF_GetError());
		return 1;
	}
	v.song_font = TTF_OpenFont(FONT_PATH, SONG_TITLE_FONT_SIZE);
	if (!v.song_font) {
		fprintf(stderr, "error opening font: %s\n", TTF_GetError());
		return 1;
	}
	v.artist_font = TTF_OpenFont(FONT_PATH, ARTIST_FONT_SIZE);
	if (!v.artist_font) {
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

	if (!IMG_Init(IMG_INIT_JPG)) {
		fprintf(stderr, "error initializing SDL_image: %s\n", IMG_GetError());
		return 1;
	}
	if (load_image(r, p.cover_path, &v.cover))
		return 1;

	SDL_SetRenderDrawColor(r, 34, 34, 34, 255);
	draw_everything(r, &v, &p);

	int ifd = inotify_init();
	int wd = inotify_add_watch(ifd, argv[1], IN_MODIFY);
	if (wd == -1)
		perror("error adding watch to file, song info won't update");
	SDL_CreateThread(watch_file, "watch_song", &ifd);

	SDL_Event e;
	while (1) {
		if (SDL_WaitEvent(&e)) {
			if (e.type == SDL_QUIT) {
				break;
			} else if (e.type == SDL_WINDOWEVENT && (e.window.event == SDL_WINDOWEVENT_RESIZED || e.window.event == SDL_WINDOWEVENT_EXPOSED)) {
				draw_everything(r, &v, &p);
			} else if (e.type == SDL_USEREVENT) {
				int old_name_len = strlen(p.cover_path);
				char *old_name = malloc(sizeof(char) * old_name_len);
				snprintf(old_name, old_name_len, "%s", p.cover_path);
				buf_len = read_playing(&p, argv[1], buf_len, &buf);
				if (buf_len == 0)
					break;
				if (strncmp(p.cover_path, old_name, old_name_len) != 0) {
					SDL_DestroyTexture(v.cover);
					if (load_image(r, p.cover_path, &v.cover))
						break;
				}
				draw_everything(r, &v, &p);
				free(old_name);
			}
		}
	}

	close(ifd);
	free(buf);
	SDL_DestroyTexture(v.cover);
	SDL_DestroyRenderer(r);
	SDL_DestroyWindow(w);
	IMG_Quit();
	TTF_CloseFont(v.song_font);
	TTF_CloseFont(v.artist_font);
	TTF_Quit();
	SDL_Quit();
	return 0;
}
