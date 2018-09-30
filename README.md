Requires: opencv, boost

Build: mkdir build && cd build && cmake .. && make -jX && 

Run: ./skin_detection
     Inputs: 1) folder path which contains images
             2) bool to diplay steps (1 or 0)


This program is made with assumption that
1) only 1 hand visible in the image
2) white (or similar) background
3) hand is "open"
4) hand color is similar to provided images


The way it works:
1) convert to HSV
2) threshold image 
3) finds largest contour which should resprent hand
This is enought to estimate skin in provided images

One image contains ring, the following steps are used to try to segment ring as much as possibe.
The main idea is that thresholding is easier if only smaller part of skin is visible.

4) estimate convexity defects
5) fit elipse to the palm in order to segment fingers
6) evaluate each finger, assuming that ring is only on the lower part of the finger
7) threshold each finger in Lab color space
8) combine all masks
              		
