//
//  Assignment3.cpp
//  OpenCV
//
//  Created by Ahmed Mohamed Fareed on 11/9/15.
//  Copyright © 2015 Ahmed Mohamed Fareed. All rights reserved.
//

#include <iostream>
#include <opencv2/opencv.hpp>
#include <string.h>
#include <vector>
#include <map>
#include <unordered_map>

using namespace cv;
using namespace std;

bool continueToNextLevel = false;
double thrshold = 15;
struct Segment;
struct Cell;
int counterlevels = 0;
int counterOfWrongPixels = 0;

Mat loadImage(String imageTitle, int imageType = 0) {
    
    Mat image = imread(imageTitle, imageType);
    if(! image.data )
    {
        cout <<  "Could Load image: " << imageTitle << std::endl ;
        exit(-1);
    }
    cout << "Image Loaded" << endl;
    return image;
}
#define TEST1 "/Users/coder93/Google Drive/Git Projects/Mobile/Xcode Projects/OpenCV/OpenCV/OpenCV/house.bmp"
#define TEST2 "/Users/coder93/Desktop/test2.jpg"
#define TEST3 "/Users/coder93/Desktop/test3.jpg"
#define TEST4 "/Users/coder93/Desktop/test4.png"
#define TEST5 "/Users/coder93/Desktop/test5.jpg"

Mat image = loadImage(TEST5);

Mat imageVarience = Mat::zeros(image.rows, image.cols, DataType<double>::type);
Mat lifeStatus = Mat(image.rows, image.cols, DataType<int>::type, -1);
Mat pixelSegment = Mat(image.rows, image.cols, DataType<int>::type, -1);
map<int, Segment> segmentsManager;

int segmentCounter = 0;

struct Segment {
private:
    double segMean = 0;
    double segVariance = 0;
    bool meanUpToData = false;
    bool varUpToData = false;
public:
    bool toBeDeleted = false;
    int segment;
    vector<pair<int, int>> joinedCells;
    vector<int> neighbourSegments;
    vector<int> tobeJoinedWith;
    int lifeStatus = -1;
    int oldSize = 0;
    Segment(pair<int, int> servivedCell) {
        segment = int(segmentCounter++);
        add(make_pair(servivedCell.first, servivedCell.second));
    }
    
    void add(pair<int, int> pixel) {
        if(pixelSegment.at<int>(pixel.first,pixel.second) != segment)
            joinedCells.push_back(make_pair(pixel.first, pixel.second));
        
        pixelSegment.at<int>(pixel.first,pixel.second) = segment;
        meanUpToData = false;
        varUpToData = false;
    }

    double mean() {
        if(!meanUpToData || oldSize != joinedCells.size()) {
            segMean = 0;
            for(int i = 0; i < joinedCells.size(); i++)
                segMean+= image.at<uchar>(joinedCells[i].first,joinedCells[i].second);
            
            segMean/= joinedCells.size();
            meanUpToData = true;
            oldSize = (int)joinedCells.size();
        }
        return segMean;
    }
    
    void setMean(double value) {
        segMean = value;
        meanUpToData = true;
    }
    
    void setVariance(double value) {
        segVariance = value;
        varUpToData = true;
    }
    
    double variance() {
        if(!meanUpToData || oldSize != joinedCells.size() ) {
            mean();
            varUpToData = false;
            oldSize = (int)joinedCells.size();
        }
        
        if(!varUpToData || oldSize != joinedCells.size()) {
            segVariance = 0;
            for(int i=0; i < joinedCells.size(); i ++)
                segVariance+= pow((image.at<uchar>(joinedCells[i].first,joinedCells[i].second) - segMean),2);
            
            segVariance /= joinedCells.size();
            varUpToData = true;
            oldSize = (int)joinedCells.size();
        }
        return segVariance;
    }
};
string concatenate(std::string const& name, int i)
{
    stringstream s;
    s << name << i;
    return s.str();
}

int temCooo = 0;
void makeImageAndShowIt() {
    Mat newImage = Mat(image.rows,image.cols,image.type());
    
    for (int i = 0; i < image.rows; i++) {
        for (int j = 0; j < image.cols; j++) {
            newImage.at<uchar>(i,j) =  image.at<uchar>(i,j);
        }
    }
    
    int px = 0;
    for(auto& x: segmentsManager) {
        for(auto& y: x.second.joinedCells) {
            px +=1;
            newImage.at<uchar>(y.first,y.second) = x.second.mean();
        }
    }
    string x = concatenate("Final Image", temCooo++);
    namedWindow( x, CV_WINDOW_AUTOSIZE );
    imshow( x, newImage );

    cout << "Total No of Pixels is: " <<px << endl;
    cout << "Done"<< endl;
}

/************************************************************
 *  @PART 1: Construction the base level for the rest of the 
 *           pyramide <Segments>
 *  @return: a list of segments for the next level
 ***********************************************************/

void calculateImageVarience(int mX, int mY){
    int number = 0;
    double mean = 0;
    double variance = 0;
    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            int x = mX + i;
            int y = mY + j;
            
            if ((x >=0 && x < image.rows) &&
                (y >= 0 && y < image.cols)) {
                number++;
                mean+= image.at<uchar>(x,y);
            }
        }
    }
    mean /= number;
    number = 0;
    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            int x = mX + i;
            int y = mY + j;
        
            if ((x >=0 && x < image.rows) &&
                (y >= 0 && y < image.cols)) {
                number++;
                variance+= pow((image.at<uchar>(x,y) - mean),2);
            }
        }
    }
    imageVarience.at<double>(mX,mY) = variance/number;
} // Checked


void printNoOfPixels() {
    int px = 0;
    for(auto& x: segmentsManager)
        for(auto& y: x.second.joinedCells)
            px++;
    
    cout << "Total No of Pixels is: " <<px << endl;
}


/**
 *  killAround checks 3x3 around a survived pixel and kills all
 *  @param: pixel -> location of the survived pixel
 *  @return: none.
 **/
void killAround(const pair<int,int>  pixel) {
    for (int i = -1;  i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            int x = pixel.first + i;
            int y = pixel.second + j;
            if (i==0 && j==0) continue;
            if ((x >=0 && x < image.rows) && (y >=0 && y < image.cols)) {
                if (lifeStatus.at<int>(x,y) == -1)
                    lifeStatus.at<int>(x,y) = 0;
            }
        }
    }
} // Checked

/**
 *  checkLifeStatus takes a pixel and checks wheather to kill or not or skip
 *                  if survived generats segment for that pixel
 *  @param: pixel -> location of the survived pixel
 *  @return: 0 -> the pixel has died
 *           1 -> the pixel has servived
 **/
int checkPixelLifeStatus(pair<int, int> pixel) {
    double imageV = imageVarience.at<double>(pixel.first,pixel.second);
    
    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            int x = pixel.first + i;
            int y = pixel.second + j;
            if (i==0 && j==0) continue;
            if ((x >=0 && x < image.rows) && (y >=0 && y < image.cols)) {
                if (lifeStatus.at<int>(x,y) == -1 && (imageVarience.at<double>(x,y) < imageV))
                    return 0;
            }
        }
    }
    
    //else the pixel survived
    lifeStatus.at<int>(pixel.first,pixel.second) = 1;
    int segName = segmentCounter;
    segmentsManager.insert(make_pair(segName, Segment(make_pair(pixel.first, pixel.second))));
    killAround(make_pair(pixel.first,pixel.second));
    return 1;
}

bool checkVectorContaintSegment(vector<int> & v,int seg) {
    for (int i= 0; i< v.size(); i++)
        if (v[i] == seg)
            return true;
    
    return false;
}

/**
 *  assigenToSegment takes a dead pixel and try to assigne to a survived pixel 
 *                  <segment>
 *  @param: pixel -> location of the dead pixel
 *  @return: none.
 **/
void assigenToSegment(const pair<int,int> pixel) {

    double minimumSoFar = 999999;
    pair<int, int> servivedPix;
    // get all survived cells around a dead cell
    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            int x = pixel.first  + i;
            int y = pixel.second + j;
            if (i==0 && j==0) continue;
            if ( (x >=0 && x < image.rows) && (y >=0 && y <  image.cols)) {
                int tmpV = lifeStatus.at<int>(x,y);
                double meanDiffrance = abs(image.at<uchar>(x,y) - image.at<uchar>(pixel.first,pixel.second));
                if(tmpV == 1 && meanDiffrance < minimumSoFar) {
                    minimumSoFar = meanDiffrance;
                    servivedPix = make_pair(x, y);
                }
            }
        }
    }
    segmentsManager.at(pixelSegment.at<int>(servivedPix.first,servivedPix.second)).add(make_pair(pixel.first, pixel.second));
}

void joinSegments(double masterIndex, double slaveIndex) {
    for (auto& x: segmentsManager.at(slaveIndex).joinedCells)
        segmentsManager.at(masterIndex).add(x);
}

/**
 *  checkSegConnc takes a pixel and check for any connection
    with other segment with the 3x3 <pixel is the center>
 *  @param: pixel -> pixel to be searsh around
 *  @return: vector of connected segments
 **/
vector<int> checkSegConnectionPixelLevel(pair<int, int> pixel) {
    vector<int> ans = vector<int>();
    for (int i = -1; i < 2; i++) {
        for (int j = -1; j < 2; j++) {
            int x = pixel.first + i;
            int y = pixel.second + j;
            if ((x >=0 && x < image.rows) && (y >=0 && y < image.cols)) {
                if(pixelSegment.at<int>(x,y) != pixelSegment.at<int>(pixel.first,pixel.second)) {
                    bool tmpSkipBool = false;
                    for (int k=0; k < ans.size() ; k++)
                        if (ans[k] == pixelSegment.at<int>(x,y)) {
                            tmpSkipBool = true;
                            break;
                        }
                    if (tmpSkipBool) continue;
                    ans.push_back(pixelSegment.at<int>(x,y));
                }
            }
        }
    }
    return ans;
}

/**
 *  CheckSegmentConnectivity is appilied on the Vector<Segment> segmentsManager
    iterate over the segments and iterate over joinedCells of each segment
    check on each pixel to get all connected Segment to this segment
    then loop over the connected Segments ......$$
 *  @param: join -> wheather to join segment or not
 *  @return: none
 **/
void checkSegmentConnectivity() {
    for (auto& x: segmentsManager) {

        vector<int> connectedSegment;
        for (int i = 0 ; i < x.second.joinedCells.size(); i++) {
            vector<int> tmpSegConnection = checkSegConnectionPixelLevel(x.second.joinedCells.at(i));
            for (int j = 0; j < tmpSegConnection.size(); j++) {
                if (abs(x.second.mean() - segmentsManager.at(tmpSegConnection.at(j)).mean()) < thrshold) {
                    if (tmpSegConnection[j] != x.first && !checkVectorContaintSegment(connectedSegment, tmpSegConnection[j])){
                        connectedSegment.push_back(tmpSegConnection[j]);
                        continueToNextLevel = true;
                    }
                }
            }
        }
        
        x.second.neighbourSegments = vector<int>();
        for (int i = 0; i < connectedSegment.size(); i++) {
            joinSegments(x.first, connectedSegment[i]);
            segmentsManager.erase(connectedSegment[i]);
        }
    }
}

/**
 *  constructBasePayramidBase is a func. responsible for calling all related funcs.
 *  @param: none.
 *  @return: none.
 **/
void constructBasePayramidBase() {
    // Calculate Variences of the imageVarience
    for (int i = 0; i < image.rows; i++)
        for (int j = 0; j < image.cols; j++)
            calculateImageVarience(i,j);

    bool doAg = true;
    while(doAg) {
        doAg = false;
        for (int i = 0; i < image.rows; i++){
            for (int j = 0; j <  image.cols; j++){
                if (lifeStatus.at<int>(i,j) == -1) {
                    if(checkPixelLifeStatus(make_pair(i, j)) == 0) {
                        doAg = true;
                    }
                }
            }
        }
    }

    cout << "Total # of survived cells: " << segmentsManager.size() << endl;
    
    for (int i = 0; i < image.rows; i++) {
        for (int j = 0; j< image.cols; j++) {
            if (lifeStatus.at<int>(i,j) == 0)
                assigenToSegment(make_pair(i, j));
        }
    }
    
    cout << "   All Pixels have been assigned to Segments." << endl;
    // checkSegment and join them
    checkSegmentConnectivity();

    cout << "Total # of survived segments after joining: " << segmentsManager.size() << endl;
    printNoOfPixels();
}

/***********************************************************
 *  @PART 2: Managing the segments and go into next levels
 *  @return: final Image <Top of the pyramide>
 ***********************************************************/

void setSegmentConnectedNeighbour(int segmentIndex) {
    segmentsManager.at(segmentIndex).neighbourSegments = vector<int>();
    for (int i = 0 ; i < segmentsManager.at(segmentIndex).joinedCells.size(); i++) {
        vector<int> tmpSegConnection = checkSegConnectionPixelLevel(segmentsManager.at(segmentIndex).joinedCells[i]);
        for (int j = 0; j < tmpSegConnection.size(); j++) {
            if (tmpSegConnection[j] != segmentIndex && !checkVectorContaintSegment(segmentsManager.at(segmentIndex).neighbourSegments, tmpSegConnection[j])){
                segmentsManager.at(segmentIndex).neighbourSegments.push_back(tmpSegConnection[j]);
            }
        }
    }
}

/**
 *  segmentsVectorLifeStatus checks wheather the segment will diy or servive.
 *                           if it servived then kills all segments around it.
 *  @param: segIndex -> the index for the segment to check on.
 *  @return: 0 -> the segment has died
 *           1 -> the segment has servived
 **/
int segmentsVectorLifeStatus(int segIndex) {
    setSegmentConnectedNeighbour(segIndex);
    for(auto& x: segmentsManager.at(segIndex).neighbourSegments) {
        if (segmentsManager.at(x).lifeStatus == -1 &&
            segmentsManager.at(x).variance() < segmentsManager.at(segIndex).variance()) {
            return 0;
        }
    }
    
    // killing Segments
    for(auto& x: segmentsManager.at(segIndex).neighbourSegments)
        if (segmentsManager.at(x).lifeStatus == -1) {
            segmentsManager.at(x).lifeStatus = 0;
        }

    segmentsManager.at(segIndex).lifeStatus = 1;
    return 1;
}

/**
 *  joinKilledSegments
 *  @param: killedSeg -> the index for the segment to check on.
 *  @return: 0 -> the segment has died
 *           1 -> the segment has servived
 **/
void joinKilledSegments(int killedSeg) {
    double minimumSoFar = 999999;
    int joinedSegIndex = -1;
    setSegmentConnectedNeighbour(killedSeg);
    for(int i = 0 ; i < segmentsManager.at(killedSeg).neighbourSegments.size(); i++)  {
        if (segmentsManager.at(segmentsManager.at(killedSeg).neighbourSegments[i]).lifeStatus == 1) {
            if(segmentsManager.at(segmentsManager.at(killedSeg).neighbourSegments[i]).variance() < minimumSoFar) {
                joinedSegIndex = i;
                minimumSoFar = segmentsManager.at(segmentsManager.at(killedSeg).neighbourSegments[i]).variance();
            }
        }
    }
//    joinSegments(segmentsManager.at(killedSeg).neighbourSegments.at(joinedSegIndex), killedSeg);

    segmentsManager.at(killedSeg).toBeDeleted = true;
    segmentsManager.at(segmentsManager.at(killedSeg).neighbourSegments.at(joinedSegIndex)).tobeJoinedWith.push_back(killedSeg);
}

/**
 *  continueTillFInish
 *  @param: none.
 *  @return: none.
 **/
void continueTillFInish () {
        makeImageAndShowIt();
    while (continueToNextLevel) {
        continueToNextLevel = false;
        counterlevels++;
        for(auto& x: segmentsManager)
            x.second.lifeStatus = -1;
        for(auto& x: segmentsManager)
            x.second.tobeJoinedWith= vector<int>();
        
        cout << "\n------------ Level "<< counterlevels <<" -------------" << endl;
        cout << "Total survived::  " << segmentsManager.size() << endl;
        int tmpSegCount = (int) segmentsManager.size();
        bool doAg = true;
        while(doAg) {
            doAg = false;
            for(auto& x: segmentsManager)
                if(x.second.lifeStatus == -1 )
                    if( segmentsVectorLifeStatus(x.first) != 1)
                        doAg = true;
        }
        
        for(auto& x: segmentsManager)
            if(x.second.lifeStatus == 0 && !x.second.toBeDeleted )
                joinKilledSegments(x.first);
        
        for(auto& x: segmentsManager)
            if(x.second.lifeStatus == 1 && !x.second.toBeDeleted )
                for (int i = 0;  i < x.second.tobeJoinedWith.size(); i++) {
                    joinSegments(x.first, x.second.tobeJoinedWith[i]);
                }

        
        map<int, Segment>::iterator itr = segmentsManager.begin();
        while (itr != segmentsManager.end()) {
            
            if (itr->second.toBeDeleted) {
                itr = segmentsManager.erase(itr);
            } else {
                ++itr;
            }
        }
        
        tmpSegCount=tmpSegCount - (int)segmentsManager.size();
        cout << "   Segments classification \n\t  -killed: " << tmpSegCount <<"\n\t  -survived: " << segmentsManager.size() << endl;
        checkSegmentConnectivity();
        makeImageAndShowIt();
    }
    cout << "\n\nTotal survived:  " << segmentsManager.size() << endl;
}



int main()
{
    clock_t t1,t2;
    t1=clock();
    constructBasePayramidBase();
    continueTillFInish();
    
    makeImageAndShowIt();
    t2=clock();
    float diff ((float)t2-(float)t1);
    cout<< "Run Time: " << diff/CLOCKS_PER_SEC<<endl;
    waitKey(0);
    return 0;
}
