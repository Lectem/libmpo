#include "include/mpo.h"
#include "cmpo.h"
#include "icon.h"

#include <assert.h>

typedef struct
{
    struct jpeg_destination_mgr pub; /* public fields */

    FILE * outfile;		/* target stream */
    JOCTET * buffer;		/* start of buffer */
} my_destination_mgr;

typedef my_destination_mgr * my_dest_ptr;
#define OUTPUT_BUF_SIZE  4096	/* choose an efficiently fwrite'able size */


void mpo_init_write(MPExt_Data * data)
{
    /*Everything should be set to when entering here*/
    data->MPF_identifier[0]='M';
    data->MPF_identifier[1]='P';
    data->MPF_identifier[2]='F';
    data->MPF_identifier[3]=0;
    if(isLittleEndian())
        data->byte_order=LITTLE_ENDIAN;
    else    data->byte_order=BIG_ENDIAN;

    data->first_IFD_offset=8;

    data->version[0]='0';
    data->version[1]='1';
    data->version[2]='0';
    data->version[3]='0';


    data->count=3;//Number of tags
    data->EntryIndex.type=07;
    data->EntryIndex.EntriesTabLength=data->numberOfImages*16;
    data->EntryIndex.FirstEntryOffset=50/*End of MPentry*/
                                      +0*12/*Individual unique ID List ?*/
                                      +0*12/*Total number of captured frames*/;
    data->nextIFDOffset=data->EntryIndex.FirstEntryOffset
                        +data->EntryIndex.EntriesTabLength/*Starts right after Value Index IFD*/;
}


void mpo_init_compress(mpo_compress_struct* mpoinfo,int numberOfImages)
{
    int i;
    //could use memset, but would need to include string.h
    for(i=0;i<(long)sizeof(*mpoinfo);++i)
    {
        *((unsigned char*)mpoinfo+i)=0;
    }
    mpoinfo->images_data=calloc(numberOfImages,sizeof(JOCTET*));
    mpoinfo->APP02.numberOfImages=numberOfImages;
    mpoinfo->APP02.MPentry=calloc(numberOfImages,sizeof(MPExt_MPEntry));
    mpoinfo->cinfo=calloc(numberOfImages,sizeof(struct jpeg_compress_struct));
    if(mpoinfo->cinfo)
        for(i=0; i<numberOfImages; ++i)
        {
            jpeg_create_compress(&mpoinfo->cinfo[i]);
        }
    mpo_init_write(&mpoinfo->APP02);
    mpo_type_forall(mpoinfo,MPType_Baseline);
    if(numberOfImages>=1)mpoinfo->APP02.MPentry[0].individualImgAttr.data.representativeImage=1;

}


void mpo_destroy_compress(mpo_compress_struct* mpoinfo)
{
    unsigned int i;
    if(mpoinfo->cinfo)
        for(i=0; i<mpoinfo->APP02.numberOfImages; ++i)
        {
            jpeg_destroy_compress(&mpoinfo->cinfo[i]);
        }
    if(mpoinfo->cinfo)free(mpoinfo->cinfo);
    mpoinfo->cinfo=NULL;

    if(mpoinfo->APP02.MPentry)free(mpoinfo->APP02.MPentry);
    mpoinfo->APP02.MPentry=NULL;
    if(mpoinfo->images_data)free(mpoinfo->images_data);
    mpoinfo->images_data=NULL;
}

/** \brief Tell mpoinfo struct what source to use
 *
 * \param imageNumber   The index of the image we are working on
 * \param data          An array of data, shall be freed by the user
 * \param width         Image's width in pixels
 * \param height        Image's height in pixels
 *
 */

void mpo_image_mem_src(mpo_compress_struct* mpoinfo,int imageNumber,unsigned char src[])
{
    mpoinfo->images_data[imageNumber]=src;
}

/** \brief Convenience function to set width and height of all images at once
*/
void mpo_dimensions_forall(mpo_compress_struct* mpoinfo,int width,int height)
{
    unsigned int i;
    for(i=0; i<mpoinfo->APP02.numberOfImages; ++i)
    {
        mpoinfo->cinfo[i].image_width=width;
        mpoinfo->cinfo[i].image_height=height;
    }
}

/** \brief Convenience function to set colorspace info of all images at once
*/
void mpo_colorspace_forall(mpo_compress_struct* mpoinfo,J_COLOR_SPACE jcs,int input_components)
{
    unsigned int i;
    for(i=0; i<mpoinfo->APP02.numberOfImages; ++i)
    {
        mpoinfo->cinfo[i].input_components = input_components;		/* # of color components per pixel */
        mpoinfo->cinfo[i].in_color_space = jcs; 	/* colorspace of input image */
    }
}


/** \brief Convenience function to set quality of all images at once
*/
void mpo_quality_forall(mpo_compress_struct* mpoinfo,int quality)
{
    unsigned int i;
    for(i=0; i<mpoinfo->APP02.numberOfImages; ++i)
    {
        jpeg_set_quality(&mpoinfo->cinfo[i], quality, TRUE );
    }
}

/** \brief Convenience function to set type of all images at once
*/
void mpo_type_forall(mpo_compress_struct* mpoinfo,MPExt_MPType type)
{

    unsigned int i;
    for(i=0; i<mpoinfo->APP02.numberOfImages; ++i)
    {
        mpoinfo->APP02.MPentry[i].individualImgAttr.data.MPTypeCode=type;
    }
}


void jpeg_write_m_int16(j_compress_ptr cinfo,uint16_t value)
{
    jpeg_write_m_byte(cinfo,value&0x00FF);
    jpeg_write_m_byte(cinfo,value>>8 &0x00FF);
}

void jpeg_write_m_int32(j_compress_ptr cinfo,uint32_t value)
{
    jpeg_write_m_byte(cinfo,value&0x000000FF);
    jpeg_write_m_byte(cinfo,(value>>8) &0x000000FF);
    jpeg_write_m_byte(cinfo,(value>>16) &0x000000FF);
    jpeg_write_m_byte(cinfo,(value>>24) &0x000000FF);
}

void jpeg_write_m_bytes(j_compress_ptr cinfo,MPFByte *value,unsigned int length)
{
    unsigned int i;
    for(i=0; i<length; ++i)
        jpeg_write_m_byte(cinfo,value[i]);
}


int jpeg_write_m_UNDEFINED(j_compress_ptr cinfo,MPFByte *value, int count)
{
    int i;
    jpeg_write_m_int16(cinfo,MP_UNDEFINED);
    jpeg_write_m_int32(cinfo,count);
    for(i=0; i<count; ++i)
        jpeg_write_m_byte(cinfo,value[i]);
    return 2+4+count;

}



int jpeg_write_m_LONG(j_compress_ptr cinfo,MPFLong *value, int count)
{
    int i;
    jpeg_write_m_int16(cinfo,MP_LONG);
    jpeg_write_m_int32(cinfo,count);
    for(i=0; i<count; ++i)
        jpeg_write_m_int32(cinfo,value[i]);
    return 2+4+4*count;
}



int jpeg_write_m_RATIONAL(j_compress_ptr cinfo,MPFRational *value, int count)
{
    int i;
    jpeg_write_m_int16(cinfo,MP_RATIONAL);
    jpeg_write_m_int32(cinfo,count);
    for(i=0; i<count; ++i)
    {
        jpeg_write_m_int32(cinfo,value[i].numerator);
        jpeg_write_m_int32(cinfo,value[i].denominator);
    }
    return 0;
}


int jpeg_write_m_SRATIONAL(j_compress_ptr cinfo,MPFSRational *value, int count)
{
    int i;
    jpeg_write_m_int16(cinfo,MP_SRATIONAL);
    jpeg_write_m_int32(cinfo,count);
    for(i=0; i<count; ++i)
    {
        jpeg_write_m_int32(cinfo,value[i].numerator);
        jpeg_write_m_int32(cinfo,value[i].denominator);
    }
    return 0;
}


int mpo_write_MPExtTag (j_compress_ptr cinfo,MPExt_Data *data,MPExt_MPTags tag)
{
    jpeg_write_m_int16(cinfo,tag);
    int bytes_written=2;

    switch(tag)
    {
    case MPTag_MPFVersion :
        bytes_written+=jpeg_write_m_UNDEFINED(cinfo,(unsigned char *)data->version,4);
        break;
    case MPTag_NumberOfImages :
        bytes_written+=jpeg_write_m_LONG(cinfo,&data->numberOfImages,1);
        break;
    case MPTag_MPEntry:
        jpeg_write_m_int16(cinfo,data->EntryIndex.type);/*Type 07 = undefined*/
        jpeg_write_m_int32(cinfo,data->EntryIndex.EntriesTabLength);
        jpeg_write_m_int32(cinfo,data->EntryIndex.FirstEntryOffset);
        bytes_written+=2+4+4;
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
    return bytes_written;
}


int mpo_write_MPExt_ValueIFD (j_compress_ptr cinfo,MPExt_Data *data)
{
    for(data->currentEntry=0; data->currentEntry < data->numberOfImages; data->currentEntry++)
    {
        jpeg_write_m_int32(cinfo,
                           data->MPentry[data->currentEntry].individualImgAttr.value);
        jpeg_write_m_int32(cinfo,
                           data->MPentry[data->currentEntry].size);
        jpeg_write_m_int32(cinfo,
                           data->MPentry[data->currentEntry].offset);
        jpeg_write_m_int16(cinfo,
                           data->MPentry[data->currentEntry].dependentImageEntry1);
        jpeg_write_m_int16(cinfo,
                           data->MPentry[data->currentEntry].dependentImageEntry2);
    }
    return (4+4+4+2+2)*data->numberOfImages;
}



int mpo_write_MPExt_IndexIFD (mpo_compress_struct * mpoinfo)
{
    int bytes_written=0;
    j_compress_ptr cinfo=&mpoinfo->cinfo[0];
    jpeg_write_m_int16(cinfo,mpoinfo->APP02.count);
        printf("mpoinfo->APP02.count=%d\n",mpoinfo->APP02.count);

    bytes_written+=2;
    bytes_written+=mpo_write_MPExtTag(cinfo,&mpoinfo->APP02,MPTag_MPFVersion);
    bytes_written+=mpo_write_MPExtTag(cinfo,&mpoinfo->APP02,MPTag_NumberOfImages);
    bytes_written+=mpo_write_MPExtTag(cinfo,&mpoinfo->APP02,MPTag_MPEntry);
    jpeg_write_m_int32(cinfo,mpoinfo->APP02.nextIFDOffset);
    bytes_written+=4;
    bytes_written+=mpo_write_MPExt_ValueIFD(cinfo,&mpoinfo->APP02);
    return bytes_written;
}

int mpo_write_MPExt_AttrIFD(mpo_compress_struct * mpoinfo, int i)
{
    int bytes_written=0;
    j_compress_ptr cinfo=&mpoinfo->cinfo[i];
    jpeg_write_m_int16(cinfo,mpoinfo->APP02.count_attr_IFD);
    bytes_written+=2;

    /* ! EMPTY !*/
    /* TODO : everything related to attributes */
    return bytes_written;

}

long mpo_compute_MPExt_Data_size(mpo_compress_struct * mpoinfo, int image)
{
    long size=0;
    size+=4;/*MPF_identifier*/
    size+=4;/*byte_order*/
    size+=4;/*first_IFD_offset*/

    if(image==0)/*Write the Index IFD only for the first image*/
    {
        size+=2;//APP02.count
        size+=2/*tag*/ + 2 /*type*/ +4/*count*/ + 4 /*data*/; //MPTag_MPFVersion
        size+=2/*tag*/ + 2 /*type*/ +4/*count*/ + 4 /*data*/; //MPTag_NumberOfImages
        size+=2/*tag*/ + 2 /*type*/ +4/*EntriesTabLength*/ + 4 /*FirstEntryOffset*/; //MPTag_MPEntry
        size+=4;//nextIFDOffset
        size+=16*mpoinfo->APP02.numberOfImages;
    }
    size+=2;//mpo_write_MPExt_AttrIFD
        //Hotfix, simulates empty attr ifd....
        size+=32*3+4;//TEST


    return size;
}



void mpo_write_MPO_Marker(mpo_compress_struct * mpoinfo,int image)
{
    int length=0;
    int i;
    j_compress_ptr cinfo=&mpoinfo->cinfo[image];
    jpeg_write_m_header(cinfo,JPEG_APP0+2,mpo_compute_MPExt_Data_size(mpoinfo,image));


    for (i=0; i<4 ; ++i )
    {
        jpeg_write_m_byte(cinfo,mpoinfo->APP02.MPF_identifier[i]);
        length++;
    }
    jpeg_write_m_byte(cinfo,mpoinfo->APP02.byte_order>>24);
    length++;
    jpeg_write_m_byte(cinfo,mpoinfo->APP02.byte_order>>16);
    length++;
    jpeg_write_m_byte(cinfo,mpoinfo->APP02.byte_order>>8);
    length++;
    jpeg_write_m_byte(cinfo,mpoinfo->APP02.byte_order);
    length++;

    jpeg_write_m_int32(cinfo,mpoinfo->APP02.first_IFD_offset);
    length+=4;

    if(image==0)/*Write the Index IFD only for the first image*/
    {
        length+=mpo_write_MPExt_IndexIFD(mpoinfo);
    }

    length+=mpo_write_MPExt_AttrIFD(mpoinfo,image);

    //hotfix, simulate attr ifd
    for(i=0;i<32*3+4;++i)
    {
        jpeg_write_m_byte(cinfo,0);
        length++;
    }



    printf("bytes wrote = %d | Computed size = %ld \n",length,mpo_compute_MPExt_Data_size(mpoinfo,image));
    assert(length==mpo_compute_MPExt_Data_size(mpoinfo,image));

}


inline long mpotell(mpo_compress_struct* mpoinfo,int image)
{
    return ftell(((my_dest_ptr)mpoinfo->cinfo[image].dest)->outfile)+/*Current file offset*/
                OUTPUT_BUF_SIZE-mpoinfo->cinfo[image].dest->free_in_buffer;/*Current buffer offset /!\ Wrong if buffer is flushed...*/
}


GLOBAL(void)
mpo_write_file (mpo_compress_struct* mpoinfo,char * filename)
{
    unsigned int i;

    struct jpeg_error_mgr jerr;
    /* More stuff */
    FILE * outfile;		/* target file */
    JSAMPROW row_pointer[1];	/* pointer to JSAMPLE row[s] */
    int row_stride;		/* physical row width in image buffer */

    long first_image_MPF_offset_begin=0;

    if ((outfile = fopen(filename, "wb")) == NULL)
    {
        fprintf(stderr, "can't open %s\n", filename);
        exit(1);
    }
    for(i=0; i<mpoinfo->APP02.numberOfImages; ++i)
    {
        mpoinfo->cinfo[i].err = jpeg_std_error(&jerr);

        jpeg_stdio_dest(&mpoinfo->cinfo[i], outfile);

        if(i>0)
        {
            //Current offset is the absolute offset of the image to write
            mpoinfo->APP02.MPentry[i].offset=ftell(((my_dest_ptr)mpoinfo->cinfo[i].dest)->outfile)-first_image_MPF_offset_begin;
            printf("Image index in 1st image Index IFD:%d\n",mpoinfo->APP02.MPentry[i].offset);
        }
        jpeg_set_defaults(&mpoinfo->cinfo[i]);
        mpoinfo->cinfo[i].write_JFIF_header=0;
        mpoinfo->cinfo[i].write_Adobe_marker=0;

        jpeg_start_compress(&mpoinfo->cinfo[i], TRUE);
        if(i==0)
        {
            first_image_MPF_offset_begin=mpotell(mpoinfo,i)+2+2+4;/*Begining will be 8 bytes later (starting at endianess)*/
            printf("1st image MPF offset begin at=%ld\n",first_image_MPF_offset_begin);
        }

        mpo_write_MPO_Marker(mpoinfo,i);
        printf("--------ftell=%ld\n",ftell(((my_dest_ptr)mpoinfo->cinfo[i].dest)->outfile));
        printf("--------=%d\n",OUTPUT_BUF_SIZE-mpoinfo->cinfo[i].dest->free_in_buffer);


        row_stride = mpoinfo->cinfo[i].image_width * 3;	/* JSAMPLEs per row in image_buffer */

        while (mpoinfo->cinfo[i].next_scanline < mpoinfo->cinfo[i].image_height)
        {
            row_pointer[0] = & mpoinfo->images_data[i][mpoinfo->cinfo[i].next_scanline * row_stride];
            (void) jpeg_write_scanlines(&mpoinfo->cinfo[i], row_pointer, 1);
        }
        printf("--------ftell=%ld\n",ftell(((my_dest_ptr)mpoinfo->cinfo[i].dest)->outfile));
        printf("--------=%d\n",mpoinfo->cinfo[i].dest->free_in_buffer);

        /* Step 6: Finish compression */

        jpeg_finish_compress(&mpoinfo->cinfo[i]);
        printf("---ftell=%ld\n",ftell(((my_dest_ptr)mpoinfo->cinfo[i].dest)->outfile));
        printf("--------=%d\n",mpoinfo->cinfo[i].dest->free_in_buffer);
        mpoinfo->APP02.MPentry[i].size= ftell(((my_dest_ptr)mpoinfo->cinfo[i].dest)->outfile)
                                        - (mpoinfo->APP02.MPentry[i].offset);
        if(i>0)mpoinfo->APP02.MPentry[i].size-=first_image_MPF_offset_begin;

        while(ftell(outfile)%16)//Align Image offset as a multiple of 16
            fputc(0,outfile);
    }

    /***************************************
    ******** Update the Index table*********
    ****************************************/
    fseek(outfile,first_image_MPF_offset_begin,SEEK_SET);
    fseek(outfile,mpoinfo->APP02.EntryIndex.FirstEntryOffset+4,SEEK_CUR);
    for(i=0;i<mpoinfo->APP02.numberOfImages;++i)
    {
        fwrite(&mpoinfo->APP02.MPentry[i].size,sizeof(INT32),1,outfile);
        fwrite(&mpoinfo->APP02.MPentry[i].offset,sizeof(INT32),1,outfile);
        fseek(outfile,8,SEEK_CUR);
    }

    fclose(outfile);

}