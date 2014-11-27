#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>
#include "include/mpo.h"


/*Change this depending on your platform endianness*/

char isLittleEndian()
{
    short int number = 0x1;
    char *numPtr = (char*)&number;
    return (numPtr[0] == 1);
}

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
jpeg_getint16 (j_decompress_ptr cinfo, int swapEndian)
{
    if(swapEndian)
        return  jpeg_getc(cinfo)<<8 |
                jpeg_getc(cinfo);
    else
        return  jpeg_getc(cinfo)|
                jpeg_getc(cinfo)<<8;
}

LOCAL(INT32)
jpeg_getint32 (j_decompress_ptr cinfo, int swapEndian)
{
    if(swapEndian)
        return  jpeg_getc(cinfo)<<24|
                jpeg_getc(cinfo)<<16|
                jpeg_getc(cinfo)<<8 |
                jpeg_getc(cinfo);
    else
        return  jpeg_getc(cinfo)    |
                jpeg_getc(cinfo)<<8 |
                jpeg_getc(cinfo)<<16|
                jpeg_getc(cinfo)<<24;
}



/* See the DC-007_E Specification. */
/* 5.2.2.1  Table 3, page 13 */
typedef enum{ LITTLE_ENDIAN = 0x49492A00,
    BIG_ENDIAN = 0x4D4D002A }MPExt_ByteOrder;

/* See the DC-007_E Specification. */
/* 5.2.2.3  Table 3, page 13 */
typedef enum
{
    /*Mandatory*/
    MPTag_MPFVersion        = 0xB000,
    MPTag_NumberOfImages    = 0xB001,
    MPTag_MPEntry           = 0xB002,
    /*Optional*/
    MPTag_ImageUIDList      = 0xB003,
    MPTag_TotalFrames       = 0xB004
}MPExt_MPIndexIFDTags;

typedef union
{
    long value;
    struct {
        unsigned int dependentParent:1;
        unsigned int dependentChild:1;
        unsigned int representativeImage:1;
        unsigned int reserved:2;
        unsigned int imgType:3;
        unsigned int MPTypeCode:24;
    } data;
}
MPExt_IndividualImageAttr;

typedef enum
{
    MPType_Baseline =0x030000,
    MPType_LargeThumbnail_Mask =0x010000,
    MPType_LargeThumbnail_Class1 =0x010001, /*VGA Equivalent*/
    MPType_LargeThumbnail_Class2 =0x010002, /*Full HD Equivalent*/
    MPType_MultiFrame_Mask =0x030000,
    MPType_MultiFrame_Panorama =0x030001,
    MPType_MultiFrame_Disparity =0x030002,
    MPType_MultiFrame_MultiAngle =0x030003
}
MPExt_MPType;

typedef struct
{
    MPExt_IndividualImageAttr individualImgAttr;
    long size;
    long offset;
    INT16 dependentImageEntry1;
    INT16 dependentImageEntry2;
}
MPExt_MPEntry;

typedef struct
{
    char MPF_identifier[4];
    MPExt_ByteOrder byte_order;
    INT32 first_IFD_offset;
    /*MP Index IFD*/
    INT16 count;
    char version[4];
    long numberOfImages;

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
    printf("\n\n-------------MPF extension data-------------\n");
    printf("MPF version:\t\t%.4s\n",data->version);
    if(data->byte_order == LITTLE_ENDIAN)
        printf("Byte order:\t\tlittle endian\n");
    else if(data->byte_order == BIG_ENDIAN)
        printf("Byte order:\tbig endian\n");
    else printf("Couldn't recognize byte order : 0x%x\n",data->byte_order);
    printf("First FID offset:\t0x%x\n",(unsigned int)data->first_IFD_offset);
    printf("---MP Index IFD---\n");
    printf("Count:\t\t\t%d(0x%x)\n",data->count,data->count);
    printf("Number of images:\t%ld\n",data->numberOfImages);
    printf("-----------------End of MPF-----------------\n\n\n");
    return TRUE;
}



METHODDEF(void)
MPExtReadTag (j_decompress_ptr cinfo,MPExt_Data *data,int* length, int swapEndian)
{
    int i;
    unsigned int tag=0;
    tag=jpeg_getint16(cinfo,swapEndian);
    *length-=2;
    switch(tag)
    {
    case MPTag_MPFVersion :
        /*Specification says that the version count = 4 but that the total length of TAG+DATA is 12 */
        /*Retrieve only the 4 lasts characters, should be equal to 0100                             */
        for(i=0;i<6;++i)(void)jpeg_getc(cinfo);//Discard the 6 first bytes : not used by the specs
        for(i=0;i<4;++i)data->version[i]=jpeg_getc(cinfo);
        break;
    case MPTag_NumberOfImages :
        /*NumberOfImages block size is 12bytes*/
        jpeg_getint16(cinfo,swapEndian);*length-=2;
        jpeg_getint32(cinfo,swapEndian);*length-=4;
        /*Long value = last for 4 bytes ?*/
        data->numberOfImages=jpeg_getint32(cinfo,swapEndian);*length-=4;
        break;
    case MPTag_MPEntry:

        break;
    /*Non mandatory*/
    default:
        printf("-----------------------TAG : 0x%x--------------------\n",tag);
        break;
    }
}
METHODDEF(boolean)
MPExtReadAPP02 (j_decompress_ptr cinfo)
{
    int i=0;
    MPExt_Data data=
    {
        .MPF_identifier={0},
        .byte_order=LITTLE_ENDIAN,
        .version={0},
    };
    int length;
    unsigned int ch=0;
    length = jpeg_getc(cinfo) << 8;
    length += jpeg_getc(cinfo);
    printf( "APP02, length %d:\n",length);
    length -= 2;
    for(i=0;i<4;++i)data.MPF_identifier[i]=jpeg_getc(cinfo);
    length-=4;
    if(data.MPF_identifier[0] != 'M'||
       data.MPF_identifier[1] != 'P'||
       data.MPF_identifier[2] != 'F'||
       data.MPF_identifier[3] != 0)
    {
        /*Ignore this block*/
        while(length-- >0){jpeg_getc(cinfo);}
        return 1;
    }
    data.byte_order= jpeg_getint32(cinfo,1);
    length-=4;
    int endiannessSwap=isLittleEndian() ^ (data.byte_order == LITTLE_ENDIAN);
    /*TODO : Take the endianess into account...*/

    data.first_IFD_offset=jpeg_getint32(cinfo,endiannessSwap);
    length-=4;
    data.count=jpeg_getint16(cinfo,endiannessSwap);
    length-=2;
    MPExtReadTag(cinfo,&data,&length,endiannessSwap);
    MPExtReadTag(cinfo,&data,&length,endiannessSwap);
    print_APP02_MPF(&data);
    return 1;
}




METHODDEF(boolean)
print_text_marker (j_decompress_ptr cinfo);



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
    printf("Finishing decompression...\n\n\n");
    jpeg_finish_decompress(&cinfo);
    jpeg_destroy_decompress(&cinfo);
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


