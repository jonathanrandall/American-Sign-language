#ifndef _DOWN_SAMPLE_H
#define _DOWN_SAMPLE_H

#define DST_WIDTH 96          // Width of the destination image
#define DST_HEIGHT 96         // Height of the destination image
// #include "Arduino.h"

// uint16_t tmp;

uint16_t *dstImage; //[DST_WIDTH * DST_HEIGHT];   // Destination gray level image data
void downsampleImage(uint16_t *srcImage, uint16_t SRC_WIDTH, uint16_t SRC_HEIGHT);
void averageResampleImage(uint16_t *srcImage, uint16_t SRC_WIDTH, uint16_t SRC_HEIGHT);

void downsampleImage(uint16_t *srcImage, uint16_t SRC_WIDTH, uint16_t SRC_HEIGHT) {
  // float scaleX = (float)SRC_WIDTH / DST_WIDTH;    // Calculate the X scaling factor
  float scaleY = (float)SRC_HEIGHT / DST_HEIGHT;  // Calculate the Y scaling factor
  float scaleX = scaleY;
  for (int y = 0; y < DST_HEIGHT; y++) {
    for (int x = 0; x < DST_WIDTH; x++) {
      // Calculate the corresponding position in the source image
      //we want to move it over 40 pixes, because width is 320, and we want 40 to 280 of the image 
      int srcX = x * scaleX+40;
      int srcY = y * scaleY;

      // Get the pixel value from the source image
      uint16_t pixel = srcImage[srcY * SRC_WIDTH + srcX];

      // Set the downscaled pixel value in the destination image
      dstImage[y * DST_WIDTH + x] = pixel;
    }
  }
}

void averageResampleImage(uint16_t *srcImage, uint16_t SRC_WIDTH, uint16_t SRC_HEIGHT) {
  float scaleX = (float)SRC_WIDTH / DST_WIDTH;    // Calculate the X scaling factor
  float scaleY = (float)SRC_HEIGHT / DST_HEIGHT;  // Calculate the Y scaling factor

  for (int y = 0; y < DST_HEIGHT; y++) {
    for (int x = 0; x < DST_WIDTH; x++) {
      // Calculate the corresponding position in the source image
      int srcXStart = scaleX * x;
      int srcYStart = scaleY * y;

      // Calculate the sum of pixel values in the source area
      int pixelSum = 0;
      for (int sy = 0; sy < scaleY; sy++) {
        for (int sx = 0; sx < scaleX; sx++) {
          pixelSum += srcImage[(srcYStart + sy) * SRC_WIDTH + (srcXStart + sx)];
        }
      }

      // Calculate the average pixel value and set it in the destination image
      uint16_t averagePixelValue = pixelSum / (scaleX * scaleY);
      dstImage[y * DST_WIDTH + x] = averagePixelValue;
    }
  }
}

#endif
