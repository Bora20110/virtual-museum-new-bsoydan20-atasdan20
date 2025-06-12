

#include "pngLoader.h"

#include <stdio.h>
#include <stdlib.h>

// Loads a PNG file and returns a populated glpngtexture struct
glpngtexture *genPNGTexture(char *filename)
{
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        fprintf(stderr, "Error: Could not open \"%s\"!\n", filename);
        exit(EXIT_FAILURE);
    }

    png_byte magic[8];
    fread(magic, 1, sizeof(magic), fp);
    if (!png_check_sig(magic, sizeof(magic))) {
        fprintf(stderr, "Error: \"%s\" is not a valid PNG file!\n", filename);
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
    if (!png_ptr) {
        fclose(fp);
        exit(EXIT_FAILURE);
    }

    png_infop info_ptr = png_create_info_struct(png_ptr);
    if (!info_ptr) {
        fclose(fp);
        png_destroy_read_struct(&png_ptr, NULL, NULL);
        exit(EXIT_FAILURE);
    }

    if (setjmp(png_jmpbuf(png_ptr))) {
        fclose(fp);
        png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
        exit(EXIT_FAILURE);
    }

    png_init_io(png_ptr, fp);
    png_set_sig_bytes(png_ptr, sizeof(magic));
    png_read_info(png_ptr, info_ptr);

    int bit_depth, color_type;
    png_uint_32 width, height;
    png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type, NULL, NULL, NULL);

    // Color conversions
    if (color_type == PNG_COLOR_TYPE_PALETTE)
        png_set_palette_to_rgb(png_ptr);
    if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
        png_set_expand_gray_1_2_4_to_8(png_ptr);
    if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
        png_set_tRNS_to_alpha(png_ptr);
    if (bit_depth == 16)
        png_set_strip_16(png_ptr);
    else if (bit_depth < 8)
        png_set_packing(png_ptr);

    png_read_update_info(png_ptr, info_ptr);

    glpngtexture *tex = malloc(sizeof(glpngtexture));
    GetPNGtextureInfo(color_type, tex);

    tex->texels = malloc(width * height * tex->internalFormat);
    png_bytep *row_pointers = malloc(sizeof(png_bytep) * height);
    for (int i = 0; i < height; ++i)
        row_pointers[i] = tex->texels + (height - i - 1) * width * tex->internalFormat;

    png_read_image(png_ptr, row_pointers);
    png_read_end(png_ptr, NULL);
    png_destroy_read_struct(&png_ptr, &info_ptr, NULL);

    free(row_pointers);
    fclose(fp);

    tex->width = (GLsizei)width;
    tex->height = (GLsizei)height;
    return tex;
}

// Determines OpenGL format and channel count based on PNG color type
void GetPNGtextureInfo(int color_type, glpngtexture *tex)
{
    switch (color_type) {
        case PNG_COLOR_TYPE_GRAY:
            tex->format = GL_LUMINANCE;
            tex->internalFormat = 1;
            break;
        case PNG_COLOR_TYPE_GRAY_ALPHA:
            tex->format = GL_LUMINANCE_ALPHA;
            tex->internalFormat = 2;
            break;
        case PNG_COLOR_TYPE_RGB:
            tex->format = GL_RGB;
            tex->internalFormat = 3;
            break;
        case PNG_COLOR_TYPE_RGB_ALPHA:
            tex->format = GL_RGBA;
            tex->internalFormat = 4;
            break;
        default:
            fprintf(stderr, "Error: Unsupported PNG color type %d\n", color_type);
            exit(EXIT_FAILURE);
    }
}
