#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include "bmp_utility.h"

#define HW_REGS_BASE         (0xff200000)
#define HW_REGS_SPAN         (0x00200000)
#define HW_REGS_MASK         (HW_REGS_SPAN - 1)
#define VIDEO_BASE           0x0000
#define PUSH_BASE            0x3010

#define IMAGE_WIDTH          320
#define IMAGE_HEIGHT         240
#define IMAGE_SPAN           (IMAGE_WIDTH * IMAGE_HEIGHT * 2)
#define IMAGE_MASK           (IMAGE_SPAN - 1)

#define FPGA_ONCHIP_BASE     (0xC8000000)

uint16_t pixels[IMAGE_HEIGHT][IMAGE_WIDTH];      // Color image
uint8_t  pixels_bw[IMAGE_HEIGHT][IMAGE_WIDTH];   // Grayscale image

int main(void) {
    volatile unsigned int *key_ptr = NULL;
    volatile unsigned short *video_mem = NULL;
    void *virtual_base;
    void *video_base;
    int fd;

    // Open /dev/mem
    if ((fd = open("/dev/mem", (O_RDWR | O_SYNC))) == -1) {
        printf("ERROR: could not open \"/dev/mem\"...\n");
        return 1;
    }

    // Map lightweight HPS-FPGA bridge for key and other peripherals
    virtual_base = mmap(NULL, HW_REGS_SPAN, (PROT_READ | PROT_WRITE), MAP_SHARED, fd, HW_REGS_BASE);
    if (virtual_base == MAP_FAILED) {
        printf("ERROR: mmap() failed...\n");
        close(fd);
        return 1;
    }

    // Map video memory (on-chip memory buffer)
    video_base = mmap(NULL, IMAGE_SPAN, (PROT_READ), MAP_SHARED, fd, FPGA_ONCHIP_BASE);
    if (video_base == MAP_FAILED) {
        printf("ERROR: mmap() video memory failed...\n");
        munmap(virtual_base, HW_REGS_SPAN);
        close(fd);
        return 1;
    }

    // Setup pointer to the pushbutton key base address
    key_ptr = (unsigned int *)(virtual_base + (PUSH_BASE & HW_REGS_MASK));
    video_mem = (unsigned short *)video_base;

    printf("Waiting for key 1, 2, or 3 to be pressed...\n");

    // Wait for any key (not key0) to be pressed
    while (1) {
        int key_val = *(key_ptr);
        if (key_val == 0x2 || key_val == 0x4 || key_val == 0x8) {
            printf("Key pressed! Capturing image...\n");
            break;
        }
        usleep(10000); // sleep 10ms
    }

    for (int y = 0; y < IMAGE_HEIGHT; ++y) {
        for (int x = 0; x < IMAGE_WIDTH; ++x) {
            uint16_t pixel = *(video_mem + (y * IMAGE_WIDTH + x));
            pixels[y][x] = pixel;
    
            // Convert RGB565 to RGB888
            uint8_t r = (pixel >> 11) & 0x1F;
            uint8_t g = (pixel >> 5) & 0x3F;
            uint8_t b = pixel & 0x1F;
    
            r <<= 3;
            g <<= 2;
            b <<= 3;
    
            // Grayscale to B/W conversion using threshold
            uint8_t grayscale = (uint8_t)((r * 0.3 + g * 0.59 + b * 0.11));
            pixels_bw[y][x] = (grayscale > 127) ? 255 : 0;
        }
    }
    

    // Save images
    saveImageShort("final_image_color.bmp", &pixels[0][0], IMAGE_WIDTH, IMAGE_HEIGHT);
    saveImageGrayscale("final_image_bw.bmp", &pixels_bw[0][0], IMAGE_WIDTH, IMAGE_HEIGHT);
    printf("Images saved as final_image_color.bmp and final_image_bw.bmp\n");

    // Clean up
    if (munmap(virtual_base, HW_REGS_SPAN) != 0 ||
        munmap(video_base, IMAGE_SPAN) != 0) {
        printf("ERROR: munmap() failed...\n");
        close(fd);
        return 1;
    }

    close(fd);
    return 0;
}
