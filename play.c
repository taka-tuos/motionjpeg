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

typedef struct {
	char *name;
	size_t siz;
	void *dat;
} ar_entry_t;

ar_entry_t *ar_read(char *name)
{
	ar_entry_t *list = NULL;
	int last = 0;
	
	FILE *fp = fopen(name,"rb");
	
	char *sig = fgets(malloc(16),16,fp);
	
	int invaild = strcmp(sig,"!<arch>\x0a");
	
	if(!invaild) {
		char dmy[32];
		char name[16];
		char ssiz[10];
		
		while(!feof(fp)) {
			printf("\nEntry %d:\n",last);
			
			int left = fread(name,16,1,fp);
			
			if(!left) {
				printf("none.\n");
				break;
			}
			
			fread(dmy,12,1,fp);
			fread(dmy,6,1,fp);
			fread(dmy,6,1,fp);
			fread(dmy,8,1,fp);
			fread(ssiz,10,1,fp);
			fread(dmy,2,1,fp);
			
			for(char *p=name;*p;p++) if(*p==' ') *p=0;
			for(char *p=ssiz;*p;p++) if(*p==' ') *p=0;
			
			printf("filename %s\n",name);
			
			int siz = atoi(ssiz);
			
			printf("siz \"%s\"=%d\n",ssiz,siz);
			
			void *dat = malloc(siz);
			
			fread(dat,siz,1,fp);
			
			if(siz & 1) fread(dmy,1,1,fp);
			
			list = realloc(list,(last+1)*sizeof(ar_entry_t));
			
			list[last].name = strdup(name);
			list[last].dat = dat;
			list[last].siz = siz;
			
			last++;
		}
	}
	
	list = realloc(list,(last+1)*sizeof(ar_entry_t));
	
	list[last].name = NULL;
	list[last].dat = NULL;
	list[last].siz = 0;
	
	return list;
}

char *sgets(char *s, int n, const char **strp){
	if(**strp == '\0')return NULL;
	int i;
	for(i=0;i<n-1;++i, ++(*strp)){
		s[i] = **strp;
		if(**strp == '\0')
			break;
		if(**strp == '\n'){
			s[i+1]='\0';
			++(*strp);
			break;
		}
	}
	if(i==n-1)
		s[i] = '\0';
	return s;
}

ar_entry_t *ar_search(ar_entry_t *list, char *name)
{
	for(ar_entry_t *p=list;p->name;p++) {
		//printf("%s == %s\n",p->name,name);
		if(!strcmp(p->name,name)) {
			return p;
		}
	}
	return NULL;
}

int main(int argc, char *argv[])
{
	if(argc <= 1) {
		fprintf(stderr, "Usage: %s <input file>\n", argv[0]);
		exit(0);
	}
	
	ar_entry_t *list = ar_read(argv[1]);
	
	SDL_Surface* sdl_surface = NULL;
	
	SDL_Event sdl_event;

	SDL_Init(SDL_INIT_VIDEO);
	
	int w,h,mpn;
	
	ar_entry_t *fp = ar_search(list, "movie.info");
	
	w = atoi(sgets(malloc(256),256,&(fp->dat)));
	h = atoi(sgets(malloc(256),256,&(fp->dat)));
	fps = atoi(sgets(malloc(256),256,&(fp->dat)));
	mpn = atoi(sgets(malloc(256),256,&(fp->dat)));
	
	jpegs = (void **)malloc(sizeof(void *) * mpn);
	
	for(int i=0;i<mpn;i++) {
		char fn[1024];
		sprintf(fn,"movie_%d.jpg",i);
		int aw,ah,dmy;
		
		ar_entry_t *jpgar = ar_search(list,fn);
		
		printf("%s reading(jpgar->dat=%p,jpgar->siz=%d)...",fn,jpgar->dat,jpgar->siz);
		jpegs[i] = stbi_load_from_memory(jpgar->dat,jpgar->siz,&aw,&ah,&dmy,4);//stbi_load(fn,&aw,&ah,&dmy,4);
		printf("done.(ptr=%p)\n",jpegs[i]);
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
