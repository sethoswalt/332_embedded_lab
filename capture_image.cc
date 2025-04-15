#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <stdint.h>
#include "bmp_utility.h"

#define HW_REGS_BASE (0xff200000)
#define HW_REGS_SPAN (0x00200000)
#define HW_REGS_MASK (HW_REGS_SPAN - 1)

#define VIDEO_BASE 0x0000
#define FPGA_ONCHIP_BASE (0xC8000000)

#define IMAGE_WIDTH 320
#define IMAGE_HEIGHT 240
#define IMAGE_SPAN (IMAGE_WIDTH * IMAGE_HEIGHT * 2)  // RGB565 uses 2 bytes
#define IMAGE_MASK (IMAGE_SPAN - 1)
#define BW_THRESHOLD 127

unsigned short pixels[IMAGE_HEIGHT][IMAGE_WIDTH];
unsigned char pixels_bw[IMAGE_HEIGHT][IMAGE_WIDTH];

int main(void) {
    volatile unsigned int *video_in_dma = NULL;
    volatile unsigned short *video_mem = NULL;
    void *virtual_base;
    void *video_base;
    int fd;

    // Open /dev/mem
    if ((fd = open("/dev/mem", (O_RDWR | O_SYNC))) == -1) {
        printf("ERROR: could not open \"/dev/mem\"...\n");
        return 1;
    }

    // Map physical memory (video registers)
    virtual_base = mmap(NULL, HW_REGS_SPAN, (PROT_READ | PROT_WRITE),
                        MAP_SHARED, fd, HW_REGS_BASE);
    if (virtual_base == MAP_FAILED) {
        printf("ERROR: mmap() failed (regs)...\n");
        close(fd);
        return 1;
    }

    // Get pointer to video DMA registers
    video_in_dma = (unsigned int *)(virtual_base + ((VIDEO_BASE) & HW_REGS_MASK));

    // Enable video
    *(video_in_dma + 3) = 0x4;
    printf("Enabled video: 0x%x\n", *(video_in_dma + 3));

    // Map video frame buffer memory
    video_base = mmap(NULL, IMAGE_SPAN, (PROT_READ | PROT_WRITE),
                      MAP_SHARED, fd, FPGA_ONCHIP_BASE);
    if (video_base == MAP_FAILED) {
        printf("ERROR: mmap() failed (frame buffer)...\n");
        munmap(virtual_base, HW_REGS_SPAN);
        close(fd);
        return 1;
    }

    video_mem = (unsigned short *)video_base;

    // Process each pixel
    for (int y = 0; y < IMAGE_HEIGHT; y++) {
        for (int x = 0; x < IMAGE_WIDTH; x++) {
            unsigned short pixel = video_mem[y * IMAGE_WIDTH + x];
            pixels[y][x] = pixel;

            // Convert RGB565 to grayscale
            int r = (pixel >> 11) & 0x1F;
            int g = (pixel >> 5) & 0x3F;
            int b = pixel & 0x1F;

            r = (r * 255) / 31;
            g = (g * 255) / 63;
            b = (b * 255) / 31;

            int gray = (int)(0.299 * r + 0.587 * g + 0.114 * b);

            // Pure B/W thresholding
            pixels_bw[y][x] = (gray > BW_THRESHOLD) ? 255 : 0;
        }
    }

    // Save images
    const char *filename_color = "final_image_color.bmp";
    saveImageShort(filename_color, &pixels[0][0], IMAGE_WIDTH, IMAGE_HEIGHT);

    const char *filename_bw = "final_image_bw.bmp";
    saveImageGrayscale(filename_bw, &pixels_bw[0][0], IMAGE_WIDTH, IMAGE_HEIGHT);

    // Clean up
    munmap(video_base, IMAGE_SPAN);
    munmap(virtual_base, HW_REGS_SPAN);
    close(fd);

    printf("Images saved. Program complete.\n");
    return 0;
}
