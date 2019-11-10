#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

int main(int argc, char *argv[])
{
	unsigned int *pic = NULL;
	int f=0;
	int aw=0,ah=0;
	
	while(1) {
		char fn[1024];
		sprintf(fn,"input/%s_png/%s_%06d.png",argv[1],argv[1],f+1);
		puts(fn);
		FILE *fp=fopen(fn,"rb");
		if(!fp) {
			break;
		}
		int w,h,dmy;
		unsigned int *inp = stbi_load_from_file(fp,&w,&h,&dmy,4);
		fclose(fp);
		if(!pic) {
			aw = w * 8;
		}
		if(f % 64 == 0) {
			ah += h * 8;
		}
		pic=realloc(pic,aw*ah*4);
		for(int y=0;y<h;y++) {
			for(int x=0;x<w;x++) {
				int yofs = (f / 64) * (h * 8);
				pic[(yofs+y*8+((f%64)/8))*aw+(x*8+(f%8))] = inp[y*w+x];
			}
		}
		f++;
	}
	
	stbi_write_jpg("movie.jpg",aw,ah,4,pic,atoi(argv[2]));
	
	return 0;
}
