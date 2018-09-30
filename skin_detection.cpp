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
        
        cv::Mat element = getStructuringElement( cv::MORPH_ELLIPSE, cv::Size( 16, 16 ), cv::Point( 5, 5 ) );
        cv::dilate(maskDrawElipse, maskDrawElipse, element);

        cv::bitwise_xor(maskDrawElipse, mask, maskOut);
        if (display) {
            showImage("5. A small dilation & remove palm by fitting elipse to farthers defect points", maskOut);
        }
        element = getStructuringElement( cv::MORPH_ELLIPSE, cv::Size( 6, 6 ), cv::Point( 5, 5 ) );
        cv::erode(maskOut, maskOut, element);

    } catch (std::string er) {
       std::cout << "\033[1;31m" << er << " " << __func__ << "\033[0m\n";
    } catch (std::string er) {
        std::cout << "\033[1;31mError occured in " << __func__ << "\033[0m\n";
    }

    return maskOut;
}
/****************************************************************************************************************************************/
cv::Mat findAndProcessFingers(cv::Mat rgbImage, cv::Mat mask, bool display) {
    cv::Mat maskOut = cv::Mat::zeros(mask.rows, mask.cols, CV_8UC1);
   
    std::vector<std::vector<cv::Point> > contoursFingers;
    std::vector<cv::Vec4i> hierarchyFingers;
    
    cv::findContours (mask, contoursFingers, hierarchyFingers, RETR_EXTERNAL, CV_CHAIN_APPROX_SIMPLE, cv::Point(0, 0) );
    
    for( int i = 0; i < contoursFingers.size(); i++ ) {
        if (cv::contourArea(contoursFingers[i]) > 20000) {

            cv::Mat finger = cv::Mat::zeros( mask.size(), CV_8UC1 );
            drawContours( finger, contoursFingers, i, cv::Scalar(255), CV_FILLED, 8, cv::Mat(), INT_MAX, cv::Point(-1,-1) );

            cv::Mat fingerRGB;

            cv::Rect boundingRect = cv::boundingRect(contoursFingers[i]);
            rgbImage.copyTo(fingerRGB, finger);

            fingerRGB = fingerRGB(boundingRect);

            cv::cvtColor(fingerRGB, fingerRGB, CV_BGR2Lab);
            cv::Mat foregnObjects;
            cv::inRange(fingerRGB, cv::Scalar(0, 10, 0), cv::Scalar(255, 255, 131), foregnObjects);
            int kernelSize = 55;
            cv::Mat element = getStructuringElement( cv::MORPH_ELLIPSE, cv::Size( kernelSize + 1, kernelSize + 1 ), cv::Point( kernelSize, kernelSize ) );
        
            //cv::dilate(foregnObjects, foregnObjects, element);
            cv::bitwise_not(foregnObjects, foregnObjects);
            foregnObjects.copyTo(maskOut(boundingRect));
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

        int kernelSize = 15;
        cv::Mat element = getStructuringElement( cv::MORPH_ELLIPSE, cv::Size( kernelSize + 1, kernelSize + 1 ), cv::Point( kernelSize, kernelSize ) );
        cv::dilate(mask, mask, element);
        if (display) showImage("2. Filling mask holes with kernel size of " + to_string(kernelSize), mask);

        std::vector<std::vector<cv::Point> > contours;
        int largestContourId;
        cv::Mat maskHand = getLargestCountour(mask, contours, largestContourId, display);
        cv::Mat maskWithoutPalm = removePalm(maskHand, contours, largestContourId, display);
        cv::Mat maskFingersProcessed = findAndProcessFingers(image, maskWithoutPalm, display);

        cv::Mat palm;
        cv::bitwise_and(maskHand, 255 - maskWithoutPalm, palm);
        showImage("palm", palm);

        cv::Mat maskFinal;
        cv::bitwise_or(palm, maskFingersProcessed, maskFinal);
        showImage("Mask", maskFinal);

        if (display) {
        cv::Mat out;
        image.copyTo(out, maskFinal);  
        showImage("Segmented image", out);
        }
    }

    return 0;
}


