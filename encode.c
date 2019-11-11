#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

FILE *arfp;

void *jpeg_buffer;
int jpeg_len;

void ar_createfile(char *name,int siz)
{
	fprintf(arfp,"%-16.16s1342943816  0     0     100644  %-10d`\x0a",name,siz);
}

void stbi_ar_write(void *context, void *data, int size)
{
	jpeg_buffer = realloc(jpeg_buffer,jpeg_len+size);
	memcpy(jpeg_buffer+jpeg_len,data,size);
	jpeg_len += size;
}

int main(int argc, char *argv[])
{
	unsigned int *pic = NULL;
	int f=0;
	int aw=0,ah=0;
	int pn=0;
	
	arfp = fopen("movie.ar","wb");
	
	fprintf(arfp,"!<arch>\x0a");
	
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
			ah = h * 8;
		}
		if((f % 64) == 0 && f != 0) {
			char fn2[1024];
			jpeg_len=0;
			jpeg_buffer=0;
			sprintf(fn2,"movie_%d.jpg",pn);
			stbi_write_jpg_to_func(stbi_ar_write,0,aw,ah,4,pic,atoi(argv[2]));
			ar_createfile(fn2,jpeg_len);
			fwrite(jpeg_buffer,jpeg_len,1,arfp);
			if(jpeg_len & 1) fputc(0x0a,arfp);
			pn++;
		}
		pic=realloc(pic,aw*ah*4);
		for(int y=0;y<h;y++) {
			for(int x=0;x<w;x++) {
				pic[(y*8+((f%64)/8))*aw+(x*8+(f%8))] = inp[y*w+x];
			}
		}
		f++;
	}
	
	char info[1024];
	sprintf(info,"%d\n%d\n%d\n%d\n",aw/8,ah/8,30,pn);
	ar_createfile("movie.info", strlen(info));
	fprintf(arfp,info);
	
	fclose(arfp);
	
	return 0;
}
