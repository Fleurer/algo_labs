/*
 * quick_sort.c
 * 2012 Dec 1
 * by fleuria <me.ssword@gmail.com>
 * 
 * */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static inline void swap(int *a, int *b) {
    int tmp = *a;
    *a = *b;
    *b = tmp;
}

unsigned int qkpartition(int arr[], size_t n, int p)
{
    int l=0, r=n-1;
    int pivot = arr[l];

    while (l < r) {
        // Find the first element which greater 
        // than the pivot in the left side, and 
        // the first element which less than 
        // the pivot in the right side, then
        // swap them and go on.
        while (l<r && arr[l] < pivot) l++;
        while (l<r && arr[r] > pivot) r--;
        if (l<r)
            swap(&arr[l], &arr[r]);
    }
    return l;
}

int qksort(int arr[], size_t n) 
{
    unsigned int p = 0;

    if (n == 0)
        return 0;

    p = qkpartition(arr, n, p);
    qksort(&arr[0], p);
    qksort(&arr[p+1], n-p-1);
    return 0;
}

int main(int argc, char *argv[])
{
    int *arr;
    int i;
    int n;

    n = argc - 1;
    arr = malloc(n * sizeof(int));
    for (i=0; i<n; i++) {
        arr[i] = atoi(argv[i+1]);
    }
    qksort(arr, n);
    for (i=0; i<n; i++) {
        printf("%d ", arr[i]);
    }
    printf("\n");
    return 0;
}
