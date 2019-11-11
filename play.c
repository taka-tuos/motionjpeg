#include <SDL/SDL.h>
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

int fps;

void **jpegs;

void adjustFPS(void) {
	static long maetime = -1;
	static int frame = 0;
	long sleeptime;
	frame++;
	if(maetime == -1) maetime = SDL_GetTicks();
	sleeptime = (frame<fps) ?
		(maetime + (long)((double)frame*(1000.0 / fps)) - SDL_GetTicks()) :
		(maetime + 1000 - SDL_GetTicks());
	if (sleeptime>0)SDL_Delay(sleeptime);
	if (frame >= fps) {
		frame = 0;
		maetime = SDL_GetTicks();
	}
}

int poll_event(SDL_Event *sdl_event)
{
	while(SDL_PollEvent(sdl_event)) {
		switch (sdl_event->type) {
		case SDL_QUIT:
			return 1;
		}
	}

	return 0;
}

int main(int argc, char *argv[])
{
	SDL_Surface* sdl_surface = NULL;
	
	SDL_Event sdl_event;

	SDL_Init(SDL_INIT_VIDEO);
	
	int w,h,mpn;
	
	FILE *fp = fopen("movie.info","rt");
	
	w = atoi(fgets(malloc(256),256,fp));
	h = atoi(fgets(malloc(256),256,fp));
	fps = atoi(fgets(malloc(256),256,fp));
	mpn = atoi(fgets(malloc(256),256,fp));
	
	fclose(fp);
	
	jpegs = (void **)malloc(sizeof(void *) * mpn);
	
	for(int i=0;i<mpn;i++) {
		char fn[1024];
		sprintf(fn,"movie_%d.jpg",i);
		int aw,ah,dmy;
		jpegs[i] = stbi_load(fn,&aw,&ah,&dmy,4);
	}

	SDL_SetVideoMode(w,h,32,SDL_SWSURFACE);
	
	sdl_surface = SDL_GetVideoSurface();
	
	int f=0;
	unsigned int *pic=0;
	int pn=0;
	
	while(!poll_event(&sdl_event)){
		if((f % 64) == 0) {
			if(pn >= mpn) break;
			if(pic) stbi_image_free(pic);
			pic = jpegs[pn];
			pn++;
		}
		
		for(int y=0;y<h;y++) {
			for(int x=0;x<w;x++) {
				unsigned int rgb = pic[(y*8+((f%64)/8))*(w*8)+(x*8+(f%8))];
				
				int r,g,b;
				
				r = (rgb >> 16) & 0xff;
				g = (rgb >>  8) & 0xff;
				b = (rgb >>  0) & 0xff;
				
				((unsigned int *)sdl_surface->pixels)[y*w+x] = (b << 16) | (g << 8) | (r << 0);
			}
		}
		SDL_UpdateRect(sdl_surface,0,0,0,0);
		f++;
		adjustFPS();
	}
	
	SDL_Quit();

	return 0;
}
