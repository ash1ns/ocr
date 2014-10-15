#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <float.h>
#include "stb_image.h"
#include "stb_image_write.h"
#include "stb_image_ext.h"
//#include <err.h>

const t_img_desc T_IMG_DESC_DEFAULT = {
    .data = NULL,
    .x = 0,
    .y = 0,
    .comp = 0
};

// (x,y) axis to tab index
static inline
int xytoi(int i, int j, t_img_desc* img)
{
    return (img->comp) * (i + (img->x) * j);
}

static inline
double pw2(double x)
{
    return x * x;
}

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
        data[i] = (data[i] > th) ? 255 : 0;
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

int thresold(uint *h)
{
    size_t size = 0;
    for (size_t i = 0; i < 256; ++i)
        size += h[i];

    double *prob = malloc(sizeof(double) * 256);
    if (!prob) {
        free(h);
        exit(EXIT_FAILURE);
    }

    double *var = malloc(sizeof(double) * 256);
    if (!var) {
        free(h);
        free(prob);
        exit(EXIT_FAILURE);
    }

    for (size_t i = 0; i < 256; ++i) {
        prob[i] = (double)h[i] / (double)size;
        var[i] = DBL_MAX;
    }

    for (size_t j = 1; j < 255; ++j) {
        double prob_under = 0;
        for (size_t k = 0; k < j; ++k)
            prob_under += prob[k];

        if (prob_under == 0)
            continue;

        double prob_up = 0;
        for (size_t k = j; k < 256; ++k)
            prob_up += prob[k];

        if (prob_up == 0)
            continue;

        double m_under = 0;
        for (size_t k = 0; k < j; ++k)
            m_under += k * prob[k];
        m_under /= prob_under;

        double m_up = 0;
        for (size_t k = j; k < 256; ++k)
            m_up += k * prob[k];
        m_up /= prob_up;

        double var_under = 0;
        for (size_t k = 0; k < j; ++k)
            var_under += pw2(k - m_under) * (1. / (double)j);

        double var_up = 0;
        for (size_t k = j; k < 256; ++k)
            var_up += pw2(k - m_up) * (1. / (double)(256 - j));

        var[j] = var_under + var_up;

        //warnx("prob_under: %lf \tprob_up: %lf", prob_under, prob_up);
        //warnx("m_under: %lf \tm_up: %lf", m_under, m_up);
        //warnx("var_under: %lf \tvar_up: %lf", var_under, var_up);
        //warnx("(%zu) var: %lf \n", j, var[j]);
    }

    size_t min = 255;
    for (size_t i = 1; i < 255; ++i) {
        if (var[min] > var[i])
            min = i;
    }

    //warnx("th: %zu", min);
    return min;
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
