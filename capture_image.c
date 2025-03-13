#include "address_map_arm.h"
#include <stdio.h>
#include <time.h>

#define KEY_BASE              0xFF200050
#define VIDEO_IN_BASE         0xFF203060
#define FPGA_ONCHIP_BASE      0xC8000000

int picture_count = 0;

void add_timestamp(volatile short *Video_Mem_ptr);
void flip_mirror(volatile short *Video_Mem_ptr);
void convert_black_and_white(volatile short *Video_Mem_ptr);
void invert_pixels(volatile short *Video_Mem_ptr);

int main(void)
{
    volatile int * KEY_ptr = (int *) KEY_BASE;
    volatile int * Video_In_DMA_ptr = (int *) VIDEO_IN_BASE;
    volatile short * Video_Mem_ptr = (short *) FPGA_ONCHIP_BASE;

    int x, y;

    *(Video_In_DMA_ptr + 3) = 0x4;  // Enable the video

    while (1)
    {
        if (*KEY_ptr != 0)  // check if any KEY was pressed
        {
            *(Video_In_DMA_ptr + 3) = 0x0;  // Disable the video to capture one frame
            while (*KEY_ptr != 0);  // wait for pushbutton KEY release
            picture_count++;
            break;
        }
    }

    while (1)
    {
        if (*KEY_ptr != 0)  // check if any KEY was pressed
        {
            break;
        }
    }

    add_timestamp(Video_Mem_ptr);
    flip_mirror(Video_Mem_ptr);
    convert_black_and_white(Video_Mem_ptr);
    invert_pixels(Video_Mem_ptr);

    for (y = 0; y < 240; y++) {
        for (x = 0; x < 320; x++) {
            short temp2 = *(Video_Mem_ptr + (y << 9) + x);
            *(Video_Mem_ptr + (y << 9) + x) = temp2;
        }
    }

}

void add_timestamp(volatile short *Video_Mem_ptr) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[20];
    snprintf(timestamp, sizeof(timestamp), "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
    int x, y;
    for (y = 0; y < 8; y++) {
        for (x = 0; x < 8 * 20; x++) {
            *(Video_Mem_ptr + (y << 9) + x) = 0xFFFF; // White pixels for timestamp
        }
    }
}

void flip_mirror(volatile short *Video_Mem_ptr) {
    int x, y;
    for (y = 0; y < 240; y++) {
        for (x = 0; x < 160; x++) {
            short temp = *(Video_Mem_ptr + (y << 9) + x);
            *(Video_Mem_ptr + (y << 9) + x) = *(Video_Mem_ptr + (y << 9) + (319 - x));
            *(Video_Mem_ptr + (y << 9) + (319 - x)) = temp;
        }
    }
}

void convert_black_and_white(volatile short *Video_Mem_ptr) {
    int x, y;
    for (y = 0; y < 240; y++) {
        for (x = 0; x < 320; x++) {
            short pixel = *(Video_Mem_ptr + (y << 9) + x);
            int r = (pixel >> 11) & 0x1F;
            int g = (pixel >> 5) & 0x3F;
            int b = pixel & 0x1F;
            int grayscale = (r * 299 + g * 587 + b * 114) / 1000;
            *(Video_Mem_ptr + (y << 9) + x) = (grayscale << 11) | (grayscale << 6) | grayscale;
        }
    }
}

void invert_pixels(volatile short *Video_Mem_ptr) {
    int x, y;
    for (y = 0; y < 240; y++) {
        for (x = 0; x < 320; x++) {
            *(Video_Mem_ptr + (y << 9) + x) = ~(*(Video_Mem_ptr + (y << 9) + x));
        }
    }
}
