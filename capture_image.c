#include "address_map_arm.h"
#include <stdio.h>
#include <time.h>

#define KEY_BASE              0xFF200050
#define VIDEO_IN_BASE         0xFF203060
#define FPGA_ONCHIP_BASE      0xC8000000

int picture_count = 0;
int current_effect = 0;

void add_timestamp(volatile short *Video_Mem_ptr, int count);
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
        if (*KEY_ptr == 0x1)  // First button pressed
        {
            *(Video_In_DMA_ptr + 3) = 0x0;  // Disable the video to capture one frame
            while (*KEY_ptr != 0);  // Wait for button release
            picture_count++;

            switch (current_effect) {
                case 0: add_timestamp(Video_Mem_ptr, picture_count); break;
                case 1: flip_image(Video_Mem_ptr); break;
                case 2: mirror_image(Video_Mem_ptr); break;
                case 3: convert_black_and_white(Video_Mem_ptr); break;
                case 4: invert_pixels(Video_Mem_ptr); break;
            }

            current_effect = (current_effect + 1) % 5;  // Cycle through effects
        }
        
        if (*KEY_ptr == 0x2)  // Second button pressed
        {
            *(Video_In_DMA_ptr + 3) = 0x4;  // Re-enable the video stream
            while (*KEY_ptr != 0);  // Wait for button release
        }
    }
}

void add_timestamp(volatile short *Video_Mem_ptr, int picture_count) {
    time_t now = time(NULL);
    struct tm *t = localtime(&now);
    char timestamp[20];
    snprintf(timestamp, sizeof(timestamp), "%02d:%02d:%02d", t->tm_hour, t->tm_min, t->tm_sec);
    int x, y;
    char count_str[10];
    snprintf(count_str, sizeof(count_str), "Img: %d", picture_count);
    for (y = 0; y < 8; y++) {
        for (x = 0; x < 8 * 20; x++) {
            if (x < 8 * 9 && y < 8) { // Display count string
                *(Video_Mem_ptr + (y << 9) + x) = 0xFFFF;
            }
            *(Video_Mem_ptr + (y << 9) + x) = 0xFFFF;
        }
    }
}
/*
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
*/

void mirror_image(volatile short *Video_Mem_ptr) {
    int x, y;
    for (y = 0; y < 240; y++) {  
        for (x = 0; x < 160; x++) {  
            short temp = *(Video_Mem_ptr + (y << 9) + x);
            *(Video_Mem_ptr + (y << 9) + x) = *(Video_Mem_ptr + (y << 9) + (319 - x));
            *(Video_Mem_ptr + (y << 9) + (319 - x)) = temp;
        }
    }
}

void flip_image(volatile short *Video_Mem_ptr) {
    int x, y;
    for (y = 0; y < 120; y++) {  
        for (x = 0; x < 320; x++) {  
            short temp = *(Video_Mem_ptr + (y << 9) + x);
            *(Video_Mem_ptr + (y << 9) + x) = *(Video_Mem_ptr + ((239 - y) << 9) + x);
            *(Video_Mem_ptr + ((239 - y) << 9) + x) = temp;
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
