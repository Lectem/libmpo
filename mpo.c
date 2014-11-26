#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include "include/mpo.h"

//
//struct my_error_mgr {
//  struct jpeg_error_mgr pub;	/* "public" fields */
//
//  jmp_buf setjmp_buffer;	/* for return to caller */
//};
//
//typedef struct my_error_mgr * my_error_ptr;
//
//


LOCAL(unsigned int)
jpeg_getc (j_decompress_ptr cinfo)
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

LOCAL(INT32)
jpeg_getint16 (j_decompress_ptr cinfo)
{
    return  jpeg_getc(cinfo)<<8 |
            jpeg_getc(cinfo);
}

LOCAL(INT32)
jpeg_getint32 (j_decompress_ptr cinfo)
{
    return  jpeg_getc(cinfo)<<24|
            jpeg_getc(cinfo)<<16|
            jpeg_getc(cinfo)<<8 |
            jpeg_getc(cinfo);
}



/* See the DC-007_E Specification. */
/* 5.2.2.1  Table 3, page 13 */
typedef enum{ LITTLE_ENDIAN = 0x49492A,
    BIG_ENDIAN = 0x4D4D002A }MPExt_ByteOrder;

/* See the DC-007_E Specification. */
/* 5.2.2.3  Table 3, page 13 */
typedef enum
{
    MPTag_MPFVersion        = 0xB000,
    MPTag_NumberOfImages    = 0xB001,
    MPTag_MPEntry           = 0xB002,
    MPTag_ImageUIDList      = 0xB003,
    MPTag_TotalFrames       = 0xB004
}MPExt_MPIndexIFDTags;

typedef struct
{
    char MPF_identifier[4];
    MPExt_ByteOrder byte_order;
    INT32 first_IFD_offset;
    /*MP Index IFD*/
    INT16 count;
    char version[12];

}
MPExt_Data;

METHODDEF(boolean)
print_APP02_MPF (MPExt_Data *data)
{
    if(data->MPF_identifier[0] != 'M'||
       data->MPF_identifier[1] != 'P'||
       data->MPF_identifier[2] != 'F'||
       data->MPF_identifier[3] != 0)
    {
        perror("Not an MP extended file.");
        return 0;
    }
    printf("\nMPF version :%12s\n",data->version);
    if(data->byte_order == LITTLE_ENDIAN)
        printf("Byte order : little endian\n");
    else if(data->byte_order == BIG_ENDIAN)
        printf("Byte order : big endian\n");
    else printf("Couldn't recognize byte order : 0x%x\n",data->byte_order);
    printf("First FID offset : 0x%x\n",data->first_IFD_offset);
    printf("\n");
}



METHODDEF(boolean)
MPExtReadAPP02 (j_decompress_ptr cinfo)
{
    int i=0;
    MPExt_Data data;
    int length;
    unsigned int ch=0;
    length = jpeg_getc(cinfo) << 8;
    length += jpeg_getc(cinfo);
    length -= 2;
    for(i=0;i<4;++i)data.MPF_identifier[i]=jpeg_getc(cinfo);
    length-=4;
    if(data.MPF_identifier[0] != 'M'||
       data.MPF_identifier[1] != 'P'||
       data.MPF_identifier[2] != 'F'||
       data.MPF_identifier[3] != 0)
    {
        perror("Not an MP extended file.\n");
        while(length-- >0){jpeg_getc(cinfo);}
        return 0;
    }
    data.byte_order= jpeg_getint32(cinfo);
    length-=4;

    /*TODO : Take the endianess into account...*/

    data.first_IFD_offset=jpeg_getint32(cinfo);
    length-=4;
    data.count=jpeg_getint16(cinfo);
    length-=2;
/*TODO move TAG|Value parsing into another func*/
/*
    for(i=0;i<12;++i)
        data.version[i]=jpeg_getc(cinfo);
    length-=12;
*/

    print_APP02_MPF(&data);
    return 1;
}




METHODDEF(boolean)
print_text_marker (j_decompress_ptr cinfo)
{
    boolean traceit = (cinfo->err->trace_level >= 1);
    INT32 length;
    unsigned int ch;
    unsigned int lastch = 0;

    length = jpeg_getc(cinfo) << 8;
    length += jpeg_getc(cinfo);
    length -= 2;			/* discount the length word itself */
    if (traceit||1)
    {
        if (cinfo->unread_marker == JPEG_COM)
            printf( "Comment, length %ld:\n", (long) length);
        else			/* assume it is an APPn otherwise */
            printf( "APP%d, length %ld:\n",
                    cinfo->unread_marker - JPEG_APP0, (long) length);

    }

    while (--length >= 0)
    {
        ch = jpeg_getc(cinfo);
        if (traceit)
        {
            /* Emit the character in a readable form.
             * Nonprintables are converted to \nnn form,
             * while \ is converted to \\.
             * Newlines in CR, CR/LF, or LF form will be printed as one newline.
             */
            if (ch == '\r')
            {
                printf( "\n");
            }
            else if (ch == '\n')
            {
                if (lastch != '\r')
                    printf( "\n");
            }
            else if (ch == '\\')
            {
                printf( "\\\\");
            }
            else if (isprint(ch))
            {
                printf("%c",ch);
            }
            else
            {
                printf( "\\%03o", ch);
            }
            lastch = ch;
        }
    }

    if (traceit)
        printf( "\n");

    return TRUE;
}






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
            decompress_mpo_from_mem(src_buffer,size);

            int index=0;
            int i;
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
                decompress_mpo_from_mem(src_buffer+index,size-index);
            }
            else printf("couldn't find second SOI marker.\n");

            free(src_buffer);
        }
        fclose(in);
    }

}



void decompress_mpo_from_mem(unsigned char* src_buffer,long size)
{
    struct jpeg_decompress_struct cinfo;
    struct jpeg_error_mgr jerr;

    JSAMPARRAY buffer; /*output*/

    int row_stride;		/* physical row width in output buffer */



    cinfo.err = jpeg_std_error(&jerr);
    printf("Creating decompress object\n");

    jpeg_create_decompress(&cinfo);
    int i=3;
    for(; i<13; ++i)
        jpeg_set_marker_processor(&cinfo, JPEG_APP0+i, print_text_marker);
    jpeg_set_marker_processor(&cinfo, JPEG_APP0+2, MPExtReadAPP02);

    jpeg_mem_src(&cinfo, src_buffer,size);
    printf("Reading header...\n");
    jpeg_read_header(&cinfo, TRUE);
    printf("Size of Jpeg buffer: %d(0x%x)\n",((struct jpeg_source_mgr*)cinfo.src)->bytes_in_buffer,((struct jpeg_source_mgr*)cinfo.src)->bytes_in_buffer);
    printf("Starting decompression...\n");

    jpeg_start_decompress(&cinfo);
    printf("Image is of size %dx%d\n",cinfo.output_width,cinfo.output_height);
    //The size of a line
    row_stride = cinfo.output_width * cinfo.output_components;
    /* Make a one-row-high sample array that will go away when done with image */
    buffer = (*cinfo.mem->alloc_sarray)
             ((j_common_ptr) &cinfo, JPOOL_IMAGE, row_stride, 1);

    while (cinfo.output_scanline < cinfo.output_height)
    {
        //printf("Reading line %d\n",cinfo.output_scanline);
        jpeg_read_scanlines(&cinfo, buffer, 1);

    }
//    printf("offset%d\n",cinfo.src.next_input_byte()-in);
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
}




