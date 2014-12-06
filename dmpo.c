#include "include/mpo.h"


void decompress_mpo(char* filename)
{
    FILE* in;
    in=fopen(filename,"rb");
    if(in)
    {
        printf("%s file opened.\n", filename);
        fseek(in,0,SEEK_END);
        long size = ftell(in);
        rewind(in);
        unsigned char * src_buffer=calloc(size,sizeof(*src_buffer));
        if(src_buffer)
        {
            fread(src_buffer,size,sizeof(*src_buffer),in);
            decompress_mpo_from_mem(src_buffer,size,1);

            int index=0;
            int i;
            /*Quick and dirty hack to reach 2nd image*/
            for(i=1; i<size-3; i++)
            {
                if(src_buffer[i] ==0xFF && src_buffer[i+1] ==0xD8
                        && src_buffer[i+2] ==0xFF && src_buffer[i+3] ==0xE1)
                {
                    index = i;
                    break;
                }
            }
            if(index)
            {
                printf("Second SOI offset : %d(0x%x)\n",index,index);
                decompress_mpo_from_mem(src_buffer+index,size-index,0);
            }
            else printf("couldn't find second SOI marker.\n");

            free(src_buffer);
        }
        fclose(in);
    }

}



void decompress_mpo_from_mem(unsigned char* src_buffer,long size,int isFirstImage)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    JSAMPARRAY buffer; /*output*/

    int row_stride;		/* physical row width in output buffer */



    cinfo.err = jpeg_std_error(&jerr);
    printf("Creating decompress object\n");

    jpeg_create_decompress(&cinfo);


    jpeg_mem_src(&cinfo, src_buffer,size);
        printf("Reading header...\n");
        jpeg_set_marker_processor(&cinfo, JPEG_APP0+2,isFirstImage?MPExtReadAPP02AsFirstImage:MPExtReadAPP02);
        jpeg_read_header(&cinfo, TRUE);
        printf("Size of Jpeg buffer: %d(0x%x)\n",((struct jpeg_source_mgr*)cinfo.src)->bytes_in_buffer,((struct jpeg_source_mgr*)cinfo.src)->bytes_in_buffer);
        printf("Starting decompression...\n");
        jpeg_start_decompress(&cinfo);
        printf("Image is of size %dx%d\n",cinfo.output_width,cinfo.output_height);
        row_stride = cinfo.output_width * cinfo.output_components;
        buffer = (*cinfo.mem->alloc_sarray)
                 ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

        while (cinfo.output_scanline < cinfo.output_height)
        {
            jpeg_read_scanlines(&cinfo, buffer, 1);

        }
        printf("Finishing decompression...\n\n\n");
        jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
}
