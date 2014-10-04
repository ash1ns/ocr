#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_image_ext.h"

const t_img_desc T_IMG_DESC_DEFAULT = {
    .data = NULL,
    .x = 0,
    .y = 0,
    .comp = 0
};

t_img_desc* load_image(char* filename, int comp)
{
    t_img_desc *img = malloc(sizeof(t_img_desc));
    if (!img)
        exit(EXIT_FAILURE);

    *img = T_IMG_DESC_DEFAULT;
    img->data = stbi_load(filename, &(img->x), &(img->y), &(img->comp), comp);

    if (img->x == 0 || img->y == 0) {
        perror(stbi_failure_reason());
        exit(EXIT_FAILURE);
    }

    return img;
}

int write_image(char* filename, t_img_desc* img)
{
    return stbi_write_png(filename, img->x, img->y, img->comp, img->data,
            img->x * img->comp);
}

void free_image(t_img_desc* img)
{
    free(img->data);
    free(img);
    img = NULL;
}

// (x,y) axis to tab index
static inline
int xytoi(int i, int j, t_img_desc* img)
{
    return (img->comp) * (i + (img->x) * j);
}

void grey_scale(t_img_desc* img)
{
    if (img->comp != 3)
        return;

    for (int i = 0; i < img->x * img->y; i++) {
        img->data[i] = grey(img->data[(i * 3)], img->data[(i * 3) + 1],
                img->data[(i * 3) + 2]);
    }

    // make the array shorter
    uchar *tmp = realloc(img->data, sizeof(char) * img->x * img->y);
    if (!tmp) {
        free_image(img);
        exit(EXIT_FAILURE);
    }

    img->data = tmp;
    img->comp = 1;
}

uchar grey(uchar r, uchar g, uchar b)
{
    return (uchar)(0.21 * r + 0.72 * g + 0.07 * b);
}

uint* histogram(t_img_desc* img)
{
    uint *h = calloc(256, sizeof(int));
    if (!h) {
        free_image(img);
        exit(EXIT_FAILURE);
    }

    uchar *ptr = img->data;
    const uchar *end = &img->data[(img->x * img->y * img->comp) - 1];

    while(ptr < end)
        h[*ptr++]++;

    return h;
}

// Faster than histogram()
// Explanation: Counting bytes fast - http://goo.gl/5LZ7fE
uint* histogram_fast(t_img_desc* img)
{
    uint *h = malloc(sizeof(int) * 256);
    if (!h) {
        free_image(img);
        exit(EXIT_FAILURE);
    }

    uchar *ptr = img->data;
    const uchar *end = &img->data[(img->x * img->y * img->comp) - 1];

    uint count1[256] = { 0 };
    uint count2[256] = { 0 };
    uint count3[256] = { 0 };
    uint count4[256] = { 0 };

    while (ptr < end - 3) {
        count1[*ptr++]++;
        count2[*ptr++]++;
        count3[*ptr++]++;
        count4[*ptr++]++;
    }

    while (ptr < end)
        count1[*ptr++]++;

    for (int i = 0; i < 256; i++)
        h[i] = count1[i] + count2[i] + count3[i] + count4[i];

    return h;
}

void binarize(uchar* data, int size, int th)
{
    for (int i = 0; i < size; i++)
        data[i] = (data[i] >= th) ? 255 : 0;
}

void binarize_basic(t_img_desc* img)
{
    if (img->comp != 1)
        return;

    binarize(img->data, img->x * img->y, 127);
}

void binarize_otsu(t_img_desc* img)
{
    if (img->comp != 1)
        return;

    uint *h = histogram_fast(img);
    int th = thresold(h);
    free(h);

    binarize(img->data, img->x * img->y, th);
}

int thresold(uint* h)
{
    /* Prevent using the function, currently it doesn't work.
     *  - return big value;
     *  - division by zero with i (fixed with 1e-6, bullshit..)
     */
    printf("ERROR: Thresold is not implemented yet.\n");
    exit(EXIT_FAILURE);
    /* END */

    double *ans = malloc(sizeof(double) * 256);
    if (!ans) {
        free(h);
        exit(EXIT_FAILURE);
    }

    double v1 = 0, v2 = 0, p1 = 0, p2 = 0;

    //Loop that calculates the best thresold
    for (int i = 0; i < 256; i++) {
        //For the element under the current thresold
        for (int j = 0; j < i; j++) {
            v1 += (j-i)*(j-i);
            p1 += h[j];
        }
        v1 = (1/(i+1e-6))*v1;

        //For the element up the current thresold
        for (int k = i; k < 256; k++) {
            v2 += (k-i)*(k-i);
            p2 += h[k];
        }
        v2 = (1/(i+1e-6))*v2;

        ans[i] = p1*v1+p2*v2;
    }

    double min = ans[0];

    for (int l = 1; l < 256; l++) {
        if (ans[l]<min)
            min = ans[l];
    }

    free(ans);
    return (int)min;
}

void average_filter(t_img_desc* img)
{
    if (img->comp != 3)
        return;

    uchar *result = calloc(img->x * img->y * img->comp, sizeof(char));
    if (!result)
        exit(EXIT_FAILURE);

    for (int y = 1; y < img->y - 1; y++) {
        for (int x = 1; x < img->x - 1; x++) {
            int k = xytoi(x, y, img);

            //result[r]
            result[k] =
                (img->data[xytoi(x - 1, y - 1, img)] +
                img->data[xytoi(x, y - 1, img)] +
                img->data[xytoi(x + 1, y - 1, img)] +
                img->data[xytoi(x - 1, y, img)] +
                img->data[k] +
                img->data[xytoi(x + 1, y, img)] +
                img->data[xytoi(x - 1, y + 1, img)] +
                img->data[xytoi(x, y + 1, img)] +
                img->data[xytoi(x + 1, y + 1, img)]) / 9;

            //result[g]
            result[k + 1] =
                (img->data[xytoi(x - 1, y - 1, img) + 1] +
                img->data[xytoi(x, y - 1, img) + 1] +
                img->data[xytoi(x + 1, y - 1, img) + 1] +
                img->data[xytoi(x - 1, y, img) + 1] +
                img->data[k + 1] +
                img->data[xytoi(x + 1, y, img) + 1] +
                img->data[xytoi(x - 1, y + 1, img) + 1] +
                img->data[xytoi(x, y + 1, img) + 1] +
                img->data[xytoi(x + 1, y + 1, img) + 1]) / 9;

            //result[b]
            result[k + 2] =
                (img->data[xytoi(x - 1, y - 1, img) + 2] +
                img->data[xytoi(x, y - 1, img) + 2] +
                img->data[xytoi(x + 1, y - 1, img) + 2] +
                img->data[xytoi(x - 1, y, img) + 2] +
                img->data[k + 2] +
                img->data[xytoi(x + 1, y, img) + 2] +
                img->data[xytoi(x - 1, y + 1, img) + 2] +
                img->data[xytoi(x, y + 1, img) + 2] +
                img->data[xytoi(x + 1, y + 1, img) + 2]) / 9;
        }
    }

    free(img->data);
    img->data = result;
}

//Application of the equation of a gaussian function in one dimension
//link: en.wikipedia.org/wiki/Gaussian_blur
void gaussian_blur(t_img_desc* img, float sigma)
{
    if (img->comp != 1)
        return;

    for(int i = 0; i < img->x * img->y; i++) {
        img->data[i] = (1 / sqrt(2 * M_PI * sigma * sigma)) *
            exp(-(i * i) / (2 * sigma * sigma));
    }
}
