#include "detection.h"

static
void trans_RLSA(uchar *tab, int size, int c)
{
    int count;
    for (int i = 0; i < size; ++i) {
        if (tab[i] == 0) {
            count = 1;

            while(i+count < size && tab[i+count] == 0)
                ++count;

            if (count > c) {
                while (count >= 0) {
                    tab[i+count] = 0;
                    --count;
                }
            }
            else {
                while (count >= 0) {
                    tab[i+count] = 255;
                    --count;
                }
            }
        }
    }
}

//Function that discriminates text and blocks of img
void RLSA(t_img_desc *img, int i, int j)
{
    uchar *tab = malloc(sizeof(char) * img->x);
    assert(tab);

    uchar *tmp = malloc(sizeof(char) * img->x * img->y);
    assert(tmp);

    for (int row = 0; row < img->y; ++row) {
        for (int x = 0; x < img->x; ++x)
            tab[x] = img->data[row * img->x + x];

        trans_RLSA(tab, img->x, i);

        for (int x = 0; x < img->x; ++x)
            tmp[row * img->x + x] = tab[x];
    }

    uchar *ptr = realloc(tab, sizeof(char) * img->y);
    assert(ptr);
    tab = ptr;

    for (int col = 0; col < img->x; ++col) {
        for (int y = 0; y < img->y; ++y)
            tab[y] = img->data[col + img->x * y];

        trans_RLSA(tab, img->y, j);

        for (int y = 0; y < img->y; ++y)
            tmp[col + img->x * y] = (!tmp[col + img->x *y]
                    && !tab[y]) ? 0 : 255;
    }

    free(tab);
    free(img->data);
    img->data = tmp;
}

size_t charX(t_img_desc *img, struct coorList *ld)
{
    size_t *array = calloc(ld->height, sizeof(size_t));
    size_t k;
    for (size_t i = 0; i < ld->height; ++i) {
        k = ld->X;
        while (k < ld->X + ld->length && img->data[k + k*i] == 255)
            ++k;
        array[i - ld->Y] = k;
    }
    k = array[0];
    for (size_t i = 0; i <ld->height; ++i) {
        if (array[i] < k)
            k = array[i];
    }
    return k;
}

size_t charLength(t_img_desc *img, struct coorList *ld, size_t begin)
{
    size_t w = 0, b = begin - 1;
    while (begin < ld->length + ld->X && w != ld->height) {
        ++b;
        w = 0;
        for (size_t i = 0; i < ld->height; ++i) {
            if (img->data[b + b * i] == 255)
                ++w;
        }
    }
    return b - begin;
}

struct coorList* getLines(t_img_desc *img)
{
    size_t i = 0, j = 0;
    //We go to the first black pixel
    while (img->data[i + i*j] == 255 && i + i*j < (size_t)(img->x * img->y)) {
        if (i == (size_t)(img->x - 1)) {
            i = 0;
            ++j;
        }
        else {
            ++i;
        }
    }
    //We check if no black pixel were found
    if (i + i*j == (size_t)(img->x * img->y))
        return NULL;
    //If we have found we create a coorList and initialize it
    struct coorList *aux = malloc(sizeof(struct coorList));
    struct coorList *ans = aux;
    ans->X = 0;
    ans->Y = 0;
    ans->length = 0;
    ans->height = 0;
    //Loop used to add all lines to the list
    while (i + i*j < (size_t)(img->x * img->y)) {
        aux->next = malloc(sizeof(struct coorList));
        aux = aux->next;
        aux->X = i;
        aux->Y = j;
        dimLine(aux, img, i, j);
        aux->next = NULL;
        //Goes to the next detected line
        i += aux->length;
        j += aux->height;
        while (img->data[i + i*j] == 255 && i + i*j < (size_t)(img->x * img->y)) {
            if (i == (size_t)(img->x - 1)) {
                i = 0;
                ++j;
            }
            else {
                ++i;
            }
        }
    }
    return ans;
}

void dimLine(struct coorList *l, t_img_desc *img, size_t i, size_t j)
{
    size_t aux = j;
    //Find height
    while (img->data[i + i*aux] == 0)
        ++aux;
    l->height = aux - 1 - j;
    //Find length
    j = aux;
    aux = i;
    while (img->data[aux + aux*j] == 0)
        ++aux;
    l->length = aux - 1 - i;
    return;
}
