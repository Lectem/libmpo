#ifndef MPO_H_INCLUDED
#define MPO_H_INCLUDED
#include <stdio.h>
#include <stdlib.h>
#include <jpeglib.h>


#include <stdint.h>

#ifdef NDEBUG
#define mpo_printf(args...)	((void)0)
#else
#define mpo_printf(args...) printf(args)
#endif

/**
 * \file
 * \brief
 * \author Lectem
 */

/*Those are Exif types*/
typedef enum
{
    MPF_BYTE         = 1,/*8-bit unsigned integer*/
    MPF_ASCII        = 2,/*8-bit byte containing one 7-bit ASCII code.The final byte is terminated with NULL. */
    MPF_SHORT        = 3,/*16-bits unsigned integer*/
    MPF_LONG         = 4,/*32-bits integer*/
    MPF_RATIONAL     = 5,/*Two LONGs. The first LONG is the numerator and the second LONG expresses the denominator. */
    MPF_UNDEFINED    = 7,
    MPF_SLONG        = 9,/*32-byte signed integer (2's complement notation).*/
    MPF_SRATIONAL    = 10 /*Two SLONGs. The first SLONG is the numerator and the second SLONG is the denominator. */
}
MPFVal_types;


typedef uint8_t	        MPFByte;          /* 1 byte  */
typedef int8_t	        MPFSByte;         /* 1 byte  */
typedef char *		    MPFAscii;
typedef uint16_t	    MPFShort;         /* 2 bytes */
typedef int16_t         MPFSShort;        /* 2 bytes */
typedef uint32_t	    MPFLong;          /* 4 bytes */
typedef int32_t		    MPFSLong;         /* 4 bytes */
typedef struct {MPFLong numerator; MPFLong denominator;} MPFRational;
typedef unsigned char	MPFUndefined;     /* 1 byte  */
typedef struct {MPFSLong numerator; MPFSLong denominator;} MPFSRational;

typedef struct
{
    MPFByte * buffer;
    long _cur;
    long _size;
}
MPFbuffer;

typedef MPFbuffer * MPFbuffer_ptr;

/* See the DC-007_E Specification. */
/* 5.2.2.1  Table 3, page 13 */
typedef enum{ LITTLE_ENDIAN = 0x49492A00,
    BIG_ENDIAN = 0x4D4D002A }MPExt_ByteOrder;

/* See the DC-007_E Specification. */
/* 5.2.2.3  Table 3, page 13 */
typedef enum
{
    /**MP Index IFD**/
    /*Mandatory*/
    MPTag_MPFVersion        = 0xB000,
    MPTag_NumberOfImages    = 0xB001,
    MPTag_MPEntry           = 0xB002,
    /*Optional*/
    //TODO : implement those tags
    MPTag_ImageUIDList      = 0xB003,
    MPTag_TotalFrames       = 0xB004,

    /**Individual image tags (attributes)**/
    MPTag_IndividualNum     = 0xb101,
    MPTag_PanOrientation    = 0xb201,
    MPTag_PanOverlapH       = 0xb202,
    MPTag_PanOverlapV       = 0xb203,
    MPTag_BaseViewpointNum  = 0xb204,
    MPTag_ConvergenceAngle  = 0xb205,
    MPTag_BaselineLength    = 0xb206,
    MPTag_VerticalDivergence= 0xb207,
    MPTag_AxisDistanceX     = 0xb208,
    MPTag_AxisDistanceY     = 0xb209,
    MPTag_AxisDistanceZ     = 0xb20a,
    MPTag_YawAngle          = 0xb20b,
    MPTag_PitchAngle        = 0xb20c,
    MPTag_RollAngle         = 0xb20d

}MPExt_MPTags;

typedef union
{
    INT32 value;
    struct {
        unsigned int MPTypeCode:24;
        unsigned int imgType:3;
        unsigned int reserved:2;
        unsigned int representativeImage:1;
        unsigned int dependentChild:1;
        unsigned int dependentParent:1;
    } data;
}
MPExt_IndividualImageAttr;

typedef enum
{
    MPType_LargeThumbnail_Mask  =0x10000,
    MPType_LargeThumbnail_Class1=0x10001, /*VGA Equivalent*/
    MPType_LargeThumbnail_Class2=0x10002, /*Full HD Equivalent*/
    MPType_MultiFrame_Mask      =0x20000,
    MPType_MultiFrame_Panorama  =0x20001,
    MPType_MultiFrame_Disparity =0x20002,
    MPType_MultiFrame_MultiAngle=0x20003,
    MPType_Baseline             =0x30000
}
MPExt_MPType;

typedef struct
{
    MPFShort type;
    MPFLong EntriesTabLength;
    MPFLong dataOffset;
}
MPExt_MPEntryIndexFields;

typedef struct
{
    MPExt_IndividualImageAttr individualImgAttr;
    MPFLong size;
    MPFLong offset;
    MPFShort dependentImageEntry1;
    MPFShort dependentImageEntry2;
}
MPExt_MPEntry;

typedef struct
{
    MPFLong IndividualNum;
    MPFLong PanOrientation;
    MPFRational PanOverlapH;
    MPFRational PanOverlapV;
    MPFLong BaseViewpointNum;
    MPFSRational ConvergenceAngle;
    MPFRational BaselineLength;
    MPFSRational VerticalDivergence;
    MPFSRational AxisDistanceX;
    MPFSRational AxisDistanceY;
    MPFSRational AxisDistanceZ;
    MPFSRational YawAngle;
    MPFSRational PitchAngle;
    MPFSRational RollAngle;
    boolean is_specified[MPTag_RollAngle-MPTag_IndividualNum+1];/*Set to true if the value has to be written/was present in the file*/
    /*Use is_specified[TAG-MPTag_IndividualNum] to get the value*/
}
MPExt_ImageAttr;

#define ATTR_IS_SPECIFIED(attr,tag) ((attr).is_specified[(tag)-MPTag_IndividualNum])


typedef struct
{
    MPFUndefined MPF_identifier[4];
    MPExt_ByteOrder byte_order;
    MPFLong first_IFD_offset;
    /*MP Index IFD*/
    INT16 count;
    char version[4];
    MPFLong numberOfImages;
    MPFLong currentEntry;
    MPExt_MPEntryIndexFields EntryIndex;
    MPFLong nextIFDOffset;

    MPFShort count_attr_IFD;

    MPExt_MPEntry* MPentry;

    MPExt_ImageAttr attributes;
}
MPExt_Data;

typedef struct
{
    MPExt_Data* APP02;
    struct jpeg_compress_struct *cinfo;
    JOCTET ** images_data;
}
mpo_compress_struct;


typedef struct
{
    MPExt_Data *APP02;
    struct jpeg_decompress_struct *cinfo;
    JOCTET ** images_data;
}
mpo_decompress_struct;


void decompress_mpo(char* filename);
void decompress_mpo_from_mem(unsigned char* src,long size,int isFirstImage);
boolean MPExtReadAPP02AsFirstImage(j_decompress_ptr cinfo);
boolean MPExtReadAPP02 (j_decompress_ptr cinfo);



inline char isLittleEndian();
boolean MPExtReadMPF (MPFbuffer_ptr b,MPExt_Data *data,int isFirstImage);
void destroyMPF_Data(MPExt_Data *data);



#endif // MPO_H_INCLUDED
