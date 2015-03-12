#include "libmpo/mpo.h"


unsigned int jpeg_getc (j_decompress_ptr cinfo)
/* Read next byte */
{
    struct jpeg_source_mgr * datasrc = cinfo->src;

    if (datasrc->bytes_in_buffer == 0)
    {
        if (! (*datasrc->fill_input_buffer) (cinfo))
            exit(-1);//ERREXIT(cinfo, JERR_CANT_SUSPEND);
    }
    datasrc->bytes_in_buffer--;
    return GETJOCTET(*datasrc->next_input_byte++);
}

int isFirstImage=0;

boolean MPExtReadAPP02AsFirstImage(j_decompress_ptr b)
{
    isFirstImage=1;
    int res=MPExtReadAPP02(b);
    isFirstImage=0;
    return res;
}

boolean MPExtReadAPP02 (j_decompress_ptr cinfo)
{
    int i=0;
    MPExt_Data data=
    {
        .MPF_identifier={0},
        .byte_order=LITTLE_ENDIAN,
        .version={0},
    };
    int length;
    length = jpeg_getc(cinfo) << 8;
    length += jpeg_getc(cinfo);
    mpo_printf( "APP02, length %d:\n",length);
    length -= 2;
    for(i=0; i<4; ++i)data.MPF_identifier[i]=jpeg_getc(cinfo);
    length-=4;
    if(data.MPF_identifier[0] != 'M'||
            data.MPF_identifier[1] != 'P'||
            data.MPF_identifier[2] != 'F'||
            data.MPF_identifier[3] != 0)
    {
        /*Ignore this block*/
        while(length-- >0)
        {
            jpeg_getc(cinfo);
        }
        return 1;
    }
    MPFbuffer buf;
    buf.buffer  = calloc(length,sizeof(MPFByte));
    buf._cur=0;
    buf._size=length;
    for(i=0;i<length;i++)
    {
        buf.buffer[i]=jpeg_getc(cinfo);
    }

   int ret= MPExtReadMPF(&buf,&data,isFirstImage);


    /*****************************************************************************************/
    /************************************DONT FORGET IT !*************************************/
    /************************Will probably be moved somewhere else****************************/
    destroyMPF_Data(&data);
    /*****************************************************************************************/
    /*****************************************************************************************/
    return ret;
}



void decompress_mpo(char* filename)
{
    FILE* in;
    in=fopen(filename,"rb");
    if(in)
    {
        mpo_printf("%s file opened.\n", filename);
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
                mpo_printf("Second SOI offset : %d(0x%x)\n",index,index);
                decompress_mpo_from_mem(src_buffer+index,size-index,0);
            }
            else mpo_printf("couldn't find second SOI marker.\n");

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
    mpo_printf("Creating decompress object\n");

    jpeg_create_decompress(&cinfo);


    jpeg_mem_src(&cinfo, src_buffer,size);
        mpo_printf("Reading header...\n");
        jpeg_set_marker_processor(&cinfo, JPEG_APP0+2,isFirstImage?MPExtReadAPP02AsFirstImage:MPExtReadAPP02);
        jpeg_read_header(&cinfo, TRUE);
        mpo_printf("Size of Jpeg buffer: %d(0x%x)\n",((struct jpeg_source_mgr*)cinfo.src)->bytes_in_buffer,((struct jpeg_source_mgr*)cinfo.src)->bytes_in_buffer);
        mpo_printf("Starting decompression...\n");
        jpeg_start_decompress(&cinfo);
        mpo_printf("Image is of size %dx%d\n",cinfo.output_width,cinfo.output_height);
        row_stride = cinfo.output_width * cinfo.output_components;
        buffer = (*cinfo.mem->alloc_sarray)
                 ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

        while (cinfo.output_scanline < cinfo.output_height)
        {
            jpeg_read_scanlines(&cinfo, buffer, 1);

        }
        mpo_printf("Finishing decompression...\n\n\n");
        jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
}
