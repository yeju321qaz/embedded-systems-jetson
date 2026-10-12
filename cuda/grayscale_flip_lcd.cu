/*
 * =====================================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  Ch03 Samples
 *
 *        Version:  1.0
 *        Created:  07/14/2021 10:41:21 PM
 *       Revision:  none
 *       Compiler:  nvcc
 *
 *         Author:  Yoon, Myung Kuk, myungkuk.yoon@ewha.ac.kr
 *   Organization:  EWHA Womans Unversity
 *
 * =====================================================================================
    // Send the image to LCD
 */

#include<iostream>
#include "ppm.h"
#include "clockMeasure.h"

extern "C" {
#include "DEV_Config.h"
#include "LCD_1in3.h"
#include "GUI_Paint.h"
#include <signal.h>     //signal()
}

__global__
void gpuCode(unsigned char *outArray, const unsigned char *inArray, const int w, const int h, int BLUR_SIZE){
   int index;
   int r,g,b;
   int row = blockIdx.y * blockDim.y + threadIdx.y;
   int col = blockIdx.x * blockDim.x + threadIdx.x;

   if (row < h && col < w) {
        index = (row * w + col) * 3;
        r = inArray[index];
        g = inArray[index + 1];
        b = inArray[index + 2];

        outArray[index] = outArray[index + 1] = outArray[index + 2] = 
		0.299 * r + 0.587 * g + 0.114 * b;
   }
}

#define PRESSED 1
#define RELEASED 2

int main(void){
    signal(SIGINT, Handler_1IN3_LCD);

    if(DEV_ModuleInit() != 0){
        DEV_ModuleExit();
        exit(0);
    }

    LCD_1IN3_Init(HORIZONTAL);
    LCD_1IN3_Clear(WHITE);
    LCD_SetBacklight(1023);

    UWORD *BlackImage;
    UDOUBLE Imagesize = LCD_1IN3_HEIGHT*LCD_1IN3_WIDTH*2;
    if((BlackImage = (UWORD *)malloc(Imagesize)) == NULL) {
        printf("Failed to apply for black memory...\r\n");
        exit(0);
    }

    Paint_NewImage(BlackImage, LCD_1IN3_WIDTH, LCD_1IN3_HEIGHT, 0, WHITE, 16);
    Paint_Clear(WHITE);

    int w, h;
    unsigned char *imageArray;
    unsigned char *outImageArray;

    unsigned char *d_imageArray;
    unsigned char *d_outImageArray;

    ppmLoad("LCD_1inch3.ppm", &imageArray, &w, &h);

    size_t arraySize = sizeof(unsigned char) * h * w * 3;
    outImageArray = (unsigned char*)malloc(arraySize);

    // Send the original image to LCD
    int i,j,index;
    int r,g,b,rgb565;
    for (i=0;i<h;i++) {
        for (j=0;j<w;j++) {
            // convert a pixel color to RGB565
            index = (i * w + j) * 3;
            r=imageArray[index];
            g=imageArray[index+1];
            b=imageArray[index+2];
            rgb565=((r>>3) << 11) | ((g>>2) << 5) | b >> 3;
            Paint_SetPixel(j,i,rgb565);
        }
    }
    LCD_1IN3_Display(BlackImage);

    int key1_state=RELEASED;
    while (1) {
        if (key1_state==RELEASED && !(GET_KEY1)) {
            key1_state=PRESSED;
        } else if (key1_state==PRESSED && GET_KEY1) {
            key1_state=RELEASED;
            break;
        }
    }

    // Alloc space for device
     cudaMalloc((void **) &d_imageArray, arraySize);
     cudaMalloc((void **) &d_outImageArray, arraySize);
     // copy umage to device
     cudaMemcpy(d_imageArray, imageArray, arraySize, cudaMemcpyHostToDevice);

     const int tSize = 16;
     dim3 blockSize(tSize, tSize, 1);
     dim3 gridSize(ceil((float)w/tSize), ceil((float)h/tSize), 1);

     // blur size = 5
     gpuCode<<<gridSize, blockSize>>>(d_outImageArray, d_imageArray, w, h, 5);

     cudaDeviceSynchronize();
     cudaMemcpy(outImageArray, d_outImageArray, arraySize, cudaMemcpyDeviceToHost);

    for (i = 0; i < h; i++) {
    for (j = 0; j < w; j++) {
        // Reverse the column index for horizontal flip
        int flipped_j = w - 1 - j;

        // Convert a pixel color to RGB565
        index = (i * w + j) * 3;
        r = imageArray[index];
        g = imageArray[index + 1];
        b = imageArray[index + 2];
        rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
        Paint_SetPixel(flipped_j, i, rgb565); // Set pixel in flipped column
    }
}
LCD_1IN3_Display(BlackImage);

// Send the blurred image to LCD (with horizontal flip)
for (i = 0; i < h; i++) {
    for (j = 0; j < w; j++) {
        // Reverse the column index for horizontal flip
        int flipped_j = w - 1 - j;

        // Convert a pixel color to RGB565
        index = (i * w + j) * 3;
        r = outImageArray[index];
        g = outImageArray[index + 1];
        b = outImageArray[index + 2];
        rgb565 = ((r >> 3) << 11) | ((g >> 2) << 5) | (b >> 3);
        Paint_SetPixel(flipped_j, i, rgb565); // Set pixel in flipped column
    }
}
LCD_1IN3_Display(BlackImage);

    free(BlackImage);
    BlackImage = NULL;
    DEV_ModuleExit();	

    return 0;
}