#include <opencv2/opencv.hpp>

#include "process_directory.h"
#include "boost/lexical_cast.hpp"

cv::RNG rng(12345);

using namespace cv;
using namespace std;


void showImage(std::string name, cv::Mat image) {
    cv::namedWindow(name, CV_WINDOW_NORMAL);
    cv::resizeWindow(name, 600, 600);
    cv::imshow(name, image);
    cv::waitKey();
}
/*******************************************************************************************************************************************/
cv::Mat getLargestCountour(cv::Mat mask, std::vector<std::vector<cv::Point> >& contours, 
                           int& largestConourId, bool display) {
    std::vector<cv::Vec4i> hierarchy;
    
    cv::findContours( mask, contours, hierarchy, RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0) );

    int largest_area = 0;
    largestConourId = 0;

    cv::Mat coloredMask = cv::Mat::zeros( mask.size(), CV_8UC3 );

    for( int i = 0; i< contours.size(); i++ ) {
        double a = cv::contourArea(contours[i], false);
        if (a > largest_area) {
            largest_area = a;
            largestConourId = i;
        }
    }
    cv::Mat newMask = cv::Mat::zeros( mask.size(), CV_8UC1 );

    cv::drawContours( coloredMask, contours, largestConourId, cv::Scalar(255, 0, 0), CV_FILLED, 8, cv::Mat(), INT_MAX, cv::Point(-1,-1));
    cv::drawContours( newMask, contours, largestConourId, cv::Scalar(255), CV_FILLED, 8, cv::Mat(), INT_MAX, cv::Point(-1,-1));

    if (display) {
        showImage("3. Finding largest contour", coloredMask);
    }

    return newMask;
}
/****************************************************************************************************************************************/
cv::Mat removePalm(cv::Mat mask, std::vector<std::vector<cv::Point> > contours, int largestContourId, bool display) {
    cv::Mat maskOut;
    try {
        std::vector<int> hulls;
        if (contours.size() <= 0 || largestContourId > contours.size()) {
            throw("Problem with contours or largest Contour id");
        }
        cv::convexHull(cv::Mat(contours[largestContourId]), hulls, false);

        std::vector<cv::Vec4i> defects;
        if (hulls.size() > 3) {
            cv::convexityDefects(cv::Mat(contours[largestContourId]), hulls, defects);
        } else {
            throw ("Cannot estimate defects");
        }
        cv::Mat defectsDisplay;
        if (display) mask.copyTo(defectsDisplay);
        std::vector<cv::Point> farthestPoints;
        for(const cv::Vec4i& v : defects)
        {
            float farthestPoint = v[3] / 256;
            if (farthestPoint > 40) 
            { 
                cv::Point farthest(contours[largestContourId][v[2]]);
                farthestPoints.push_back(farthest);
                
                if (display) cv::circle(defectsDisplay, farthest, 4, cv::Scalar(0, 0, 255), 35);
            }
        }
        if (farthestPoints.size() < 1) {
            throw ("Cannot estimate difects (find fingers)");
        }
        if (display) showImage("4. Possible finger boundaries", defectsDisplay);

        std::vector<cv::RotatedRect> minRect( contours.size() );
        cv::RotatedRect minEllipse;
        minEllipse = cv::fitEllipse( Mat(farthestPoints) ); 

        cv::Mat maskDrawElipse = Mat::zeros( mask.size(), CV_8UC1 );
        cv::ellipse( maskDrawElipse, minEllipse, cv::Scalar(255), CV_FILLED, 8 );
        
        cv::Mat element = getStructuringElement( cv::MORPH_ELLIPSE, cv::Size( 16, 16 ), cv::Point( 15, 15 ) );
        cv::dilate(maskDrawElipse, maskDrawElipse, element);

        cv::bitwise_xor(maskDrawElipse, mask, maskOut);
        if (display) {
            showImage("5. A small dilation & remove palm by fitting elipse to farthers defect points", maskOut);
        }
        element = getStructuringElement( cv::MORPH_ELLIPSE, cv::Size( 11, 11 ), cv::Point( 10, 10 ) );
        cv::erode(maskOut, maskOut, element);

    } catch (char* er) {
       std::cout << "\033[1;31m" << er << " " << __func__ << "\033[0m\n";
    } catch (...) {
        std::cout << "\033[1;31mError occured in " << __func__ << "\033[0m\n";
    }

    return maskOut;
}
/****************************************************************************************************************************************/
cv::Mat findAndProcessFingers(cv::Mat rgbImage, cv::Mat mask, bool display) {
    cv::Mat_<uchar> maskOut = cv::Mat::zeros(mask.rows, mask.cols, CV_8UC1);
   
    std::vector<std::vector<cv::Point> > contoursFingers;
    std::vector<cv::Vec4i> hierarchyFingers;
    
    cv::findContours (mask, contoursFingers, hierarchyFingers, RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0) );
    
    for( int i = 0; i < contoursFingers.size(); i++ ) {
        if (cv::contourArea(contoursFingers[i]) > 20000) {

            cv::Mat_<uchar> finger = cv::Mat::zeros( mask.size(), CV_8UC1 );
            drawContours( finger, contoursFingers, i, cv::Scalar(255), CV_FILLED, 8, cv::Mat(), INT_MAX, cv::Point(-1,-1) );

            cv::Mat fingerRGB;

            cv::Rect boundingRect = cv::boundingRect(contoursFingers[i]);
            rgbImage.copyTo(fingerRGB, finger);

            fingerRGB = fingerRGB(boundingRect);

            cv::Mat_<uchar> foregnObjects;
            cv::cvtColor(fingerRGB, fingerRGB, CV_BGR2Lab);

            if (boundingRect.width > boundingRect.height) {
                int w = boundingRect.width / 3;
                
                for (size_t r = boundingRect.y; r < boundingRect.y + boundingRect.height; r++) {
                    for (size_t c = boundingRect.x; c < boundingRect.x + w; c++) {
                        maskOut(r, c) = finger(r, c);
                    }
                }
                
                fingerRGB = fingerRGB(cv::Rect(0 + w,  0, boundingRect.width - w, boundingRect.height));
                cv::inRange(fingerRGB, cv::Scalar(0, 5, 0), cv::Scalar(255, 255, 131), foregnObjects);
                cv::bitwise_not(foregnObjects, foregnObjects); 
                
                for (size_t r = boundingRect.y; r < boundingRect.y + boundingRect.height; r++) {
                    for (size_t c = boundingRect.x + w; c < boundingRect.x + boundingRect.width; c++) {
                        maskOut(r, c) = foregnObjects(r - boundingRect.y, c - boundingRect.x);
                    }
                }

            } else {
                int h = boundingRect.height / 3;

                for (size_t r = boundingRect.y; r < boundingRect.y + h; r++) {
                    for (size_t c = boundingRect.x; c < boundingRect.x + boundingRect.width; c++) {
                        maskOut(r, c) = finger(r, c);
                    }
                }

                fingerRGB = fingerRGB(cv::Rect(0,  0 + h, boundingRect.width, boundingRect.height - h));

                cv::inRange(fingerRGB, cv::Scalar(0, 5, 0), cv::Scalar(255, 255, 131), foregnObjects);
                cv::bitwise_not(foregnObjects, foregnObjects); 
                for (size_t r = boundingRect.y + h; r < boundingRect.y + boundingRect.height; r++) {
                    for (size_t c = boundingRect.x; c < boundingRect.x + boundingRect.width; c++) {
                        maskOut(r, c) = foregnObjects(r - boundingRect.y, c - boundingRect.x);
                    }
                }                
            }
           
        }
    }
    if (display) showImage("Processed segmented fingers", maskOut);
    return maskOut;

}
/********************************************************************************************************************************/
int main(int argc, char *argv[]) {

    ProcessDirectory pd;
    std::vector<std::string> images = pd.readImagesFromFolder(argv[1]);
    bool display = boost::lexical_cast<bool>(argv[2]);

    for (auto im: images) {
        std::cout << "\033[1;32m" << im << "\033[0m\n";
        cv::Mat image = cv::imread(im, CV_LOAD_IMAGE_COLOR);

        cv::Mat_<cv::Vec3b> hsv;
        cv::cvtColor(image, hsv, CV_BGR2HSV);
        
        cv::Mat mask;
        cv::inRange(hsv, cv::Scalar(0, 0, 0), cv::Scalar(255, 40, 255), mask);
        cv::bitwise_not (mask, mask);
        if (display) showImage("1. Binary image after HSV thresholding", mask);

        int kernelSize = 9;
        cv::Mat element = getStructuringElement( cv::MORPH_ELLIPSE, cv::Size( kernelSize + 1, kernelSize + 1 ), cv::Point( kernelSize, kernelSize ) );
        cv::dilate(mask, mask, element);
        if (display) showImage("2. Filling mask holes with kernel size of " + to_string(kernelSize), mask);

        std::vector<std::vector<cv::Point> > contours;
        int largestContourId;
        cv::Mat maskHand = getLargestCountour(mask, contours, largestContourId, display);
       // cv::Mat maskWithoutPalm = removePalm(maskHand, contours, largestContourId, display);
      //  cv::Mat maskFingersProcessed = findAndProcessFingers(image, maskWithoutPalm, display);

       // cv::Mat palm;
       // cv::bitwise_and(maskHand, 255 - maskWithoutPalm, palm);
       // cv::Mat maskFinal;
       // cv::bitwise_or(palm, maskFingersProcessed, maskFinal);
        showImage("Original", image);
        showImage("Mask", maskHand);

      /*  if (display) {
            cv::Mat out;
            image.copyTo(out, maskFinal);  
            showImage("Segmented image", out);
        } */
    }

    return 0;
}


