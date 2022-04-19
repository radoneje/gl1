//
// Created by Denis Shevchenko on 19.04.2022.
//

#ifndef TEXTURE_H
#define TEXTURE_H

struct Image
{
    unsigned char* pixels;
    int width;
    int height;
    int numChannels;
};

class Texture
{
public:
    Texture ();
    void Prepare (int texN);

    void ReadPPMImage (char *fn);

    GLuint texName;
    Image image;
};

#endif
