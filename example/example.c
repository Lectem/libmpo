#include <stdio.h>
#include <stdlib.h>
#include "include/mpo.h"
#include "icon.h"
#include "cmpo.h"

int main()
{
//    decompress_mpo("3DS-big-endian.mpo");
//    decompress_mpo("test-little-endian.mpo");


    int image_width=320;
    int image_height=240;
    mpo_compress_struct mpoinfo;
    mpo_init_compress(&mpoinfo,2);
    mpo_quality_forall(&mpoinfo,75);
    mpo_dimensions_forall(&mpoinfo,image_width,image_height);
    mpo_colorspace_forall(&mpoinfo,JCS_RGB,3);

    mpo_image_mem_src(&mpoinfo,0,iconBitmap);
    mpo_image_mem_src(&mpoinfo,1,iconBitmap);
    mpo_type_forall(&mpoinfo,MPType_MultiFrame_Disparity);//This is the type for 3D images

    mpo_write_file(&mpoinfo,"test.mpo");
    //Writes the mpo file WITHOUT exif
    mpo_destroy_compress(&mpoinfo);

    //We will be able to test it once the decompression will be done correctly
    //Actually not working because of the tricky SOI+EXIF markers lookup
    //decompress_mpo("test.mpo");

    return 0;
}
