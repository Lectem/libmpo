#include <stdio.h>
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




void decompress_mpo(char* buf)
{
    struct jpeg_decompress_struct cinfo;
//    struct my_error_mgr jerr;

    JSAMPARRAY buffer; /*output*/
    int row_stride;		/* physical row width in output buffer */

    FILE* in;
    in=fopen(buf,"rb");

    if(in)
    {
        jpeg_create_decompress(&cinfo);
        jpeg_stdio_src(&cinfo, infile);
        jpeg_read_header(&cinfo, TRUE);
    }
}




