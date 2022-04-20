//
// Created by Denis Shevchenko on 19.04.2022.
//
#include <fstream>
#include <GL/glut.h>
#pragma warning (disable : 4996)
#include "Texture.h"

Texture::Texture ()
{
}

void Texture::Prepare (int texN)
{
    texName = texN;
    glPixelStorei (GL_UNPACK_ALIGNMENT, 1);

    glBindTexture (GL_TEXTURE_2D, texName);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER,GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,GL_NEAREST);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, image.width,
                 image.height, 0, GL_RGB, GL_UNSIGNED_BYTE,
                 image.pixels);
    // gluBuild2DMipmaps(GL_TEXTURE_2D, 0, GL_RGB, image.width,
    //                 image.height, 0, GL_RGB, GL_UNSIGNED_BYTE,
    //                 image.pixels)
}

void Texture::ReadPPMImage (char* fn)
{
    int tmpint;
    char str[100];
    FILE* inFile = fopen (fn,"rb");

    if (inFile == NULL)
    {
        printf ("Can't open input file %s. Exiting.\n",fn);
        exit (1);
    }

    fscanf (inFile,"P%d\n", &tmpint);

    if (tmpint != 6)
    {
        printf ("Input file is not ppm. Exiting. %d \n", tmpint);
        exit (1);
    }

    // skip comments embedded in header

    fgets (str,100,inFile);
    while (str[0]=='#')
        fgets(str,100,inFile);

    // read image dimensions

    sscanf (str,"%d %d",&image.width, &image.height);
    fgets (str,100,inFile);
    sscanf (str,"%d",&tmpint);

    if (tmpint != 255)
        printf("Warning: maxvalue is not 255 in ppm file\n");

    image.numChannels = 3;
    image.pixels = (unsigned char*) malloc (image.numChannels * image.width *  image.height * sizeof (unsigned char));

    if (image.pixels == NULL)
    {
        printf ("Can't allocate image of size %dx%d. Exiting\n", image.width, image.height);
        exit (1);
    }
    else
        printf("Reading image %s of size %dx%d\n", fn, image.width, image.height);


    fread (image.pixels, sizeof (unsigned char), image.numChannels * image.width * image.height, inFile);

    fclose (inFile);
}