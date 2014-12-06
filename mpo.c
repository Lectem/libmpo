#include <assert.h>
#include "include/mpo.h"


inline char isLittleEndian()
{
    short int number = 0x1;
    char *numPtr = (char*)&number;
    return (numPtr[0] == 1);
}


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

INT32 jpeg_getint32 (j_decompress_ptr cinfo, int swapEndian)
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


void destroyMP_Data(MPExt_Data *data)
{
    if(data->MPentry)
    {
        free(data->MPentry);
        data->MPentry=NULL;
    }
}

boolean print_APP02_MPF (MPExt_Data *data)
{
    int i;
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
        printf("Byte order:\tlittle endian\n");
    else if(data->byte_order == BIG_ENDIAN)
        printf("Byte order:\tbig endian\n");
    else printf("Couldn't recognize byte order : 0x%x\n",data->byte_order);
    printf("First IFD offset:\t0x%x\n",(unsigned int)data->first_IFD_offset);
    printf("---MP Index IFD---\n");
    printf("Count:\t\t\t%d(0x%x)\n",data->count,data->count);
    printf("Number of images:\t%ld\n",data->numberOfImages);
    if(data->currentEntry>0)printf("%ld entries listed\n",data->currentEntry);
    for(i=0;i<data->currentEntry;++i)
    {
        printf("Size:\t\t\t%ld\nOffset:\t\t\t%ld\n",data->MPentry[i].size,data->MPentry[i].offset);
        if(data->MPentry[i].individualImgAttr.data.dependentChild)printf("Dependent child image\n");
        if(data->MPentry[i].individualImgAttr.data.dependentParent)printf("Dependent parent image\n");
        if(data->MPentry[i].individualImgAttr.data.representativeImage)printf("Representative image\n");
        if(data->MPentry[i].individualImgAttr.data.imgType == 0)printf("Image Type : JPEG\n");
    }


    printf("-----------------End of MPF-----------------\n\n\n");
    return TRUE;
}


int MPExtReadValueIFD (j_decompress_ptr cinfo,MPExt_Data *data, int swapEndian)
{
    int read_bytes=0;
    data->MPentry=(MPExt_MPEntry*)calloc(data->numberOfImages,sizeof(MPExt_MPEntry));
    for(data->currentEntry=0;data->currentEntry < data->numberOfImages;data->currentEntry++)
    {
        data->MPentry[data->currentEntry].individualImgAttr.value=jpeg_getint32(cinfo,swapEndian);
        read_bytes+=4;
        data->MPentry[data->currentEntry].size=jpeg_getint32(cinfo,swapEndian);
        read_bytes+=4;
        data->MPentry[data->currentEntry].offset=jpeg_getint32(cinfo,swapEndian);
        read_bytes+=4;
        data->MPentry[data->currentEntry].dependentImageEntry1=jpeg_getint16(cinfo,swapEndian);
        read_bytes+=2;
        data->MPentry[data->currentEntry].dependentImageEntry2=jpeg_getint16(cinfo,swapEndian);
        read_bytes+=2;
    }
    return read_bytes;
}



int MPExtReadTag (j_decompress_ptr cinfo,MPExt_Data *data, int swapEndian)
{
    int i;
    unsigned int tag=0;
    tag=jpeg_getint16(cinfo,swapEndian);
    int read_bytes=2;


    switch(tag)
    {
    case MPTag_MPFVersion :
        /*Specification says that the version count = 4 but that the total length of TAG+DATA is 12 */
        /*Retrieve only the 4 lasts characters, should be equal to 0100                             */
        jpeg_getint16(cinfo,swapEndian);/*Type? 07->Undefined?*/
        jpeg_getint32(cinfo,swapEndian);/*String size(Count)*/
        for(i=0;i<4;++i)data->version[i]=jpeg_getc(cinfo);
        read_bytes+=10;
        break;
    case MPTag_NumberOfImages :
        /*NumberOfImages block size is 12bytes*/
        jpeg_getint16(cinfo,swapEndian);read_bytes+=2;/*Type ? 04-> LONG?*/
        jpeg_getint32(cinfo,swapEndian);read_bytes+=4;/*Count = 1 ?*/
        /*Long value = last for 4 bytes ?*/
        data->numberOfImages=jpeg_getint32(cinfo,swapEndian);read_bytes+=4;
        break;
    case MPTag_MPEntry:
            data->EntryIndex.type=jpeg_getint16(cinfo,swapEndian);read_bytes+=2;/*Type? 07 = undefined?*/
            data->EntryIndex.EntriesTabLength=jpeg_getint32(cinfo,swapEndian);read_bytes+=4;
            data->EntryIndex.FirstEntryOffset=jpeg_getint32(cinfo,swapEndian);read_bytes+=4;
        break;
    /*Non mandatory*/
    default:
        if(tag >>8 == 0xB0)
        printf("----------------Ignoring Index IFD TAG : 0x%x----------------\n",tag);
        else if(tag>>8 == 0xB1)
        printf("-------------Ignoring Individual IFD TAG : 0x%x--------------\n",tag);
        else if(tag>>8 == 0xB2)
        printf("----------------Ignoring Attr IFD TAG : 0x%x-----------------\n",tag);
        else
        printf("-----------------------Unknown TAG : 0x%x--------------------\n",tag);
        break;
    }
    return read_bytes;
}




int MPExtReadIndexIFD (j_decompress_ptr cinfo,MPExt_Data *data, int swapEndian)
{
    int read_bytes=0;
    int i;
    data->count=jpeg_getint16(cinfo,swapEndian);/*Number of tags*/
    read_bytes+=2;
    for(i=0;i<data->count;++i)
    {
        read_bytes+=MPExtReadTag(cinfo,data,swapEndian);
    }
    data->nextIFDOffset=jpeg_getint32(cinfo,swapEndian);
    read_bytes+=4;

    read_bytes+=MPExtReadValueIFD(cinfo,data,swapEndian);
    return read_bytes;
}

int isFirstImage=0;

boolean MPExtReadAPP02AsFirstImage(j_decompress_ptr cinfo)
{
    isFirstImage=1;
    int res=MPExtReadAPP02(cinfo);
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

    int OFFSET_START = length;

    data.byte_order= jpeg_getint32(cinfo,1);
    length-=4;
    int endiannessSwap=isLittleEndian() ^ (data.byte_order == LITTLE_ENDIAN);
    /*TODO : Take the endianess into account...*/
    printf("ENDIANNESSSWAP=%d\n",endiannessSwap);
    data.first_IFD_offset=jpeg_getint32(cinfo,endiannessSwap);
    length-=4;
    while(length > OFFSET_START - data.first_IFD_offset ) //While we didn't reach the IFD...
    {
        jpeg_getc(cinfo);
        length--;
    }
    if(isFirstImage)
    {
        length-=MPExtReadIndexIFD(cinfo,&data,endiannessSwap);
        assert(OFFSET_START - data.nextIFDOffset <= length);
    }

    /**ASSUMING MP ATTRIBUTES IFD TO BE RIGHT AFTER THE VALUE OF MP INDEX IFD**/
    //TODO : use offset (nextIFD of First IFD)
    if(isFirstImage &&
       OFFSET_START-data.nextIFDOffset == length||
       OFFSET_START-data.first_IFD_offset == length)
    {
        data.count_attr_IFD=jpeg_getint16(cinfo,endiannessSwap);
        length-=2;
        for(i=0;i<data.count_attr_IFD;++i)
        {
            //TODO: add parsing from attribute tags
            length-=MPExtReadTag(cinfo,&data,endiannessSwap);
        }
    }

    printf("bytes remaining : %d\n",length);
    while(length-- >0)
    {
        printf("0x%.2x ",jpeg_getc(cinfo));
    }
    printf("\n");
    print_APP02_MPF(&data);





    /*****************************************************************************************/
    /************************************DONT FORGET IT !*************************************/
    /************************Will probably be moved somewhere else****************************/
    destroyMP_Data(&data);
    /*****************************************************************************************/
    /*****************************************************************************************/

    return 1;
}

