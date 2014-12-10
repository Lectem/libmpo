#include <assert.h>
#include "libmpo/mpo.h"


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


long mpf_tell(MPFbuffer_ptr b)
{
    return b->_cur;
}

void mpf_seek(MPFbuffer_ptr b,long offset, int from)
{
    if(from==SEEK_CUR)b->_cur+=offset;
    else if(from==SEEK_SET)b->_cur=offset;
    else if(from==SEEK_END)b->_cur=b->_size-1+offset;
}

unsigned int mpf_getbyte (MPFbuffer_ptr b)
/* Read next byte */
{
    assert(b->_cur <= b->_size);
    return b->buffer[b->_cur++];
}

void mpf_dc_rewindc(MPFbuffer_ptr b)
{
    b->_cur--;
}

uint16_t mpf_getint16 (MPFbuffer_ptr b, int swapEndian)
{
    if(swapEndian)
        return  mpf_getbyte(b)<<8 |
                mpf_getbyte(b);
    else
        return  mpf_getbyte(b)|
                mpf_getbyte(b)<<8;
}

uint32_t mpf_getint32 (MPFbuffer_ptr b, int swapEndian)
{
    if(swapEndian)
        return  mpf_getbyte(b)<<24|
                mpf_getbyte(b)<<16|
                mpf_getbyte(b)<<8 |
                mpf_getbyte(b);
    else
        return  mpf_getbyte(b)    |
                mpf_getbyte(b)<<8 |
                mpf_getbyte(b)<<16|
                mpf_getbyte(b)<<24;
}


int mpf_getLONG(MPFLong * value, int count,MPFbuffer_ptr b,int swapEndian)
{
    int read_bytes=0;
    read_bytes+=2;
    assert(mpf_getint16(b,swapEndian)==MPF_LONG);
    read_bytes+=4;
    assert(mpf_getint32(b,swapEndian)==(uint32_t)count);
    int i;
    for(i=0;i<count;++i)
    {
        value[i]=mpf_getint32(b,swapEndian);
        read_bytes+=4;
    }
    return read_bytes;
}


int mpf_getRATIONAL(MPFRational * value, int count,MPFbuffer_ptr b,int swapEndian)
{
    int read_bytes=0;
    read_bytes+=2;
    MPFLong type=mpf_getint16(b,swapEndian);
    assert(type==MPF_RATIONAL || type==MPF_SRATIONAL);
    read_bytes+=4;
    assert(mpf_getint32(b,swapEndian)==(uint32_t)count);
    int i;
    for(i=0;i<count;++i)
    {
        MPFLong offset=mpf_getint32(b,swapEndian);
        read_bytes+=4;
        long cur=mpf_tell(b);
        mpf_seek(b,offset,SEEK_SET);
        value[i].numerator=mpf_getint32(b,swapEndian);
        value[i].denominator=mpf_getint32(b,swapEndian);
        mpf_seek(b,cur,SEEK_SET);
    }
    return read_bytes;
}


#define mpf_getSRATIONAL(value,count,b,swapEndian) \
        mpf_getRATIONAL(((MPFRational*) value),(count),(b),(swapEndian))

void destroyMPF_Data(MPExt_Data *data)
{
    if(data->MPentry)
    {
        free(data->MPentry);
        data->MPentry=NULL;
    }
}

void print_MPFLong(MPFLong l)
{
    printf("%d",l);
}

void print_MPFRational(MPFRational r)
{
    if(r.denominator==0.0 || (r.numerator==0xFFFFFFFF&&r.denominator==0xFFFFFFFF) ) printf("Unknown");
    else printf("%f (%d/%d)",(double)r.numerator/(double)r.denominator,r.numerator,r.denominator);
}


void print_MPFSRational(MPFSRational r)
{
    if(r.denominator==0.0 || (r.numerator==(int32_t)0xFFFFFFFF&&r.denominator==(int32_t)0xFFFFFFFF) ) printf("Unknown");
    else printf("%f (%d/%d)",(double)r.numerator/(double)r.denominator,r.numerator,r.denominator);
}


boolean print_APP02_MPF (MPExt_Data *data)
{
    unsigned int i;
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
    printf("Number of images:\t%d\n",data->numberOfImages);
    if(data->currentEntry>0)printf("%d entries listed\n",data->currentEntry);

    printf("----------\n");
    for(i=0; i<data->currentEntry; ++i)
    {
        printf("\tSize:\t\t%d\n\tOffset:\t\t%d\n",data->MPentry[i].size,data->MPentry[i].offset);
        printf("\tDepImageEntry1:\t%d\n",data->MPentry[i].dependentImageEntry1);
        printf("\tDepImageEntry2:\t%d\n",data->MPentry[i].dependentImageEntry2);
        if(data->MPentry[i].individualImgAttr.data.imgType == 0)printf("\tData format:\tJPEG\n");
        printf("\tImage type:\t");
        switch(data->MPentry[i].individualImgAttr.data.MPTypeCode)
        {
            case MPType_LargeThumbnail_Class1:
                printf("Large Thumbnail (VGA)\n");
                break;
            case MPType_LargeThumbnail_Class2:
                printf("Large Thumbnail (Full-HD)\n");
                break;
            case MPType_MultiFrame_Panorama  :
                printf("Multi-Frame Panorama\n");
                break;
            case MPType_MultiFrame_Disparity :
                printf("Multi-Frame Disparity\n");
                break;
            case MPType_MultiFrame_MultiAngle:
                printf("Multi-Frame Multi-Angle\n");
                break;
            case MPType_Baseline             :
                printf("Baseline\n");
                break;
            default:
                printf("UNDEFINED! 0x%x(value=0x%x)\n",
                       data->MPentry[i].individualImgAttr.data.MPTypeCode,
                       (unsigned int)data->MPentry[i].individualImgAttr.value);
        }
        if(data->MPentry[i].individualImgAttr.data.dependentChild)printf("\tDependent child image\n");
        if(data->MPentry[i].individualImgAttr.data.dependentParent)printf("\tDependent parent image\n");
        if(data->MPentry[i].individualImgAttr.data.representativeImage)printf("\tRepresentative image\n");

#define print_attr(TagType,fieldname,name)\
        {if(ATTR_IS_SPECIFIED(data->attributes,MPTag_ ## fieldname))\
        {\
            printf(name ": ");\
            print_ ## TagType (data->attributes.fieldname);\
            printf("\n");\
        }}
        print_attr(MPFLong,     IndividualNum,      "MP Individual Number\t\t");
        print_attr(MPFLong,     PanOrientation,     "Panorama Scanning orientation\t");
        print_attr(MPFRational, PanOverlapH,        "Panorama Horizontal Overlap\t");
        print_attr(MPFRational, PanOverlapV,        "Panorama Vertical Overlap\t");
        print_attr(MPFLong,     BaseViewpointNum,   "Base Viewpoint Number\t\t");
        print_attr(MPFSRational,ConvergenceAngle,   "Convergence angle\t\t");
        print_attr(MPFRational, BaselineLength,     "Baseline Length\t\t\t");
        print_attr(MPFSRational,VerticalDivergence, "Vertical Divergence Angle\t");
        print_attr(MPFSRational,AxisDistanceX,      "Horizontal Axis (X) distance\t");
        print_attr(MPFSRational,AxisDistanceY,      "Vertical Axis (Y) distance\t");
        print_attr(MPFSRational,AxisDistanceZ,      "Collimation Axis (Z) distance\t");
        print_attr(MPFSRational,YawAngle,           "Yaw angle\t\t\t\t");
        print_attr(MPFSRational,PitchAngle,         "Pitch angle\t\t\t\t");
        print_attr(MPFSRational,RollAngle,          "Roll angle\t\t\t\t");
        printf("----------\n");
    }


    printf("-----------------End of MPF-----------------\n\n\n");
    return TRUE;
}


int MPExtReadValueIFD (MPFbuffer_ptr b,MPExt_Data *data, int swapEndian)
{
    int read_bytes=0;
    data->MPentry=(MPExt_MPEntry*)calloc(data->numberOfImages,sizeof(MPExt_MPEntry));
    for(data->currentEntry=0; data->currentEntry < data->numberOfImages; data->currentEntry++)
    {
        data->MPentry[data->currentEntry].individualImgAttr.value=mpf_getint32(b,swapEndian);
        read_bytes+=4;
        data->MPentry[data->currentEntry].size=mpf_getint32(b,swapEndian);
        read_bytes+=4;
        data->MPentry[data->currentEntry].offset=mpf_getint32(b,swapEndian);
        read_bytes+=4;
        data->MPentry[data->currentEntry].dependentImageEntry1=mpf_getint16(b,swapEndian);
        read_bytes+=2;
        data->MPentry[data->currentEntry].dependentImageEntry2=mpf_getint16(b,swapEndian);
        read_bytes+=2;
    }
    return read_bytes;
}



int MPExtReadTag (MPFbuffer_ptr b,MPExt_Data *data, int swapEndian)
{
    int i;
    uint16_t tag=0;
    tag=mpf_getint16(b,swapEndian);
    int read_bytes=2;


    switch(tag)
    {
    case MPTag_MPFVersion :
        /*Specification says that the version count = 4 but that the total length of TAG+DATA is 12 */
        /*Retrieve only the 4 lasts characters, should be equal to 0100                             */
        mpf_getint16(b,swapEndian);/*Type? 07->Undefined?*/
        mpf_getint32(b,swapEndian);/*String size(Count)*/
        for(i=0; i<4; ++i)data->version[i]=mpf_getbyte(b);
        read_bytes+=10;
        break;
    case MPTag_NumberOfImages :
        /*NumberOfImages block size is 12bytes*/
        mpf_getint16(b,swapEndian);
        read_bytes+=2;/*Type ? 04-> LONG?*/
        mpf_getint32(b,swapEndian);
        read_bytes+=4;/*Count = 1 ?*/
        /*Long value = last for 4 bytes ?*/
        data->numberOfImages=mpf_getint32(b,swapEndian);
        read_bytes+=4;
        break;
    case MPTag_MPEntry:
        data->EntryIndex.type=mpf_getint16(b,swapEndian);
        read_bytes+=2;/*Type? 07 = undefined?*/
        data->EntryIndex.EntriesTabLength=mpf_getint32(b,swapEndian);
        read_bytes+=4;
        data->EntryIndex.FirstEntryOffset=mpf_getint32(b,swapEndian);
        read_bytes+=4;
        break;

    case MPTag_IndividualNum:
        read_bytes+=mpf_getLONG(&data->attributes.IndividualNum,1,b,swapEndian);
        break;
    case MPTag_PanOrientation:
        read_bytes+=mpf_getLONG(&data->attributes.PanOrientation,1,b,swapEndian);
        break;
    case MPTag_PanOverlapH:
        read_bytes+=mpf_getRATIONAL(&data->attributes.PanOverlapH,1,b,swapEndian);
        break;
    case MPTag_PanOverlapV:
        read_bytes+=mpf_getRATIONAL(&data->attributes.PanOverlapV,1,b,swapEndian);
        break;
    case MPTag_BaseViewpointNum:
        read_bytes+=mpf_getLONG(&data->attributes.BaseViewpointNum,1,b,swapEndian);
        break;
    case MPTag_ConvergenceAngle:
        read_bytes+=mpf_getSRATIONAL(&data->attributes.ConvergenceAngle,1,b,swapEndian);
        break;
    case MPTag_BaselineLength:
        read_bytes+=mpf_getRATIONAL(&data->attributes.BaselineLength,1,b,swapEndian);
        break;
    case MPTag_VerticalDivergence:
        read_bytes+=mpf_getSRATIONAL(&data->attributes.VerticalDivergence,1,b,swapEndian);
        break;
    case MPTag_AxisDistanceX:
        read_bytes+=mpf_getSRATIONAL(&data->attributes.AxisDistanceX,1,b,swapEndian);
        break;
    case MPTag_AxisDistanceY:
        read_bytes+=mpf_getSRATIONAL(&data->attributes.AxisDistanceY,1,b,swapEndian);
        break;
    case MPTag_AxisDistanceZ:
        read_bytes+=mpf_getSRATIONAL(&data->attributes.AxisDistanceZ,1,b,swapEndian);
        break;
    case MPTag_YawAngle:
        read_bytes+=mpf_getSRATIONAL(&data->attributes.YawAngle,1,b,swapEndian);
        break;
    case MPTag_PitchAngle:
        read_bytes+=mpf_getSRATIONAL(&data->attributes.PitchAngle,1,b,swapEndian);
        break;
    case MPTag_RollAngle:
        read_bytes+=mpf_getSRATIONAL(&data->attributes.RollAngle,1,b,swapEndian);
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
    if(tag >=MPTag_IndividualNum && tag <= MPTag_RollAngle)
        ATTR_IS_SPECIFIED(data->attributes,tag)=1;
    return read_bytes;
}




int MPExtReadIndexIFD (MPFbuffer_ptr b,MPExt_Data *data, int swapEndian)
{
    int read_bytes=0;
    int i;
    data->count=mpf_getint16(b,swapEndian);/*Number of tags*/
    read_bytes+=2;
    for(i=0; i<data->count; ++i)
    {
        read_bytes+=MPExtReadTag(b,data,swapEndian);
    }
    data->nextIFDOffset=mpf_getint32(b,swapEndian);
    read_bytes+=4;

    read_bytes+=MPExtReadValueIFD(b,data,swapEndian);
    return read_bytes;
}

int isFirstImage=0;

boolean MPExtReadAPP02AsFirstImage(j_decompress_ptr b)
{
    isFirstImage=1;
    int res=MPExtReadAPP02(b);
    isFirstImage=0;
    return res;
}

boolean MPExtReadMPF (MPFbuffer_ptr b,MPExt_Data *data)
{
    int i;
    long length=b->_size;
    long OFFSET_START = length;

    data->byte_order= mpf_getint32(b,1);
    length-=4;

    int endiannessSwap=isLittleEndian() ^ (data->byte_order == LITTLE_ENDIAN);
    /*TODO : Take the endianess into account...*/
    printf("ENDIANNESSSWAP=%d\n",endiannessSwap);
    data->first_IFD_offset=mpf_getint32(b,endiannessSwap);
    length-=4;

    while(length > (int)( OFFSET_START - data->first_IFD_offset) ) //While we didn't reach the IFD...
    {
        mpf_getbyte(b);
        length--;
    }

    if(isFirstImage)
    {
        printf("%ld != %ld\n",OFFSET_START,length);

        length-=MPExtReadIndexIFD(b,data,endiannessSwap);
        printf("%ld != %ld\n",OFFSET_START,length);

    }

    /**ASSUMING MP ATTRIBUTES IFD TO BE RIGHT AFTER THE VALUE OF MP INDEX IFD**/
    //TODO : use offset (nextIFD of First IFD)
    assert( (isFirstImage && (int)(OFFSET_START-data->nextIFDOffset) == length ) ||
            (int)(OFFSET_START-data->first_IFD_offset) == length);
    {
        data->count_attr_IFD=mpf_getint16(b,endiannessSwap);
        length-=2;
        for(i=0; i<data->count_attr_IFD; ++i)
        {
            //TODO: add parsing from attribute tags
            length-=MPExtReadTag(b,data,endiannessSwap);
        }
    }

    printf("bytes remaining : %ld\n",length);
    while(length-- >0)
    {
        printf("0x%.2x ",mpf_getbyte(b));
    }
    printf("\n");
    print_APP02_MPF(data);




    return 1;
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

   int ret= MPExtReadMPF(&buf,&data);


    /*****************************************************************************************/
    /************************************DONT FORGET IT !*************************************/
    /************************Will probably be moved somewhere else****************************/
    destroyMPF_Data(&data);
    /*****************************************************************************************/
    /*****************************************************************************************/
    return ret;
}

