#include <iostream>
#include <fstream>

#include <opencv2/features2d/features2d.hpp>
#include <opencv2/imgproc/imgproc.hpp>

#include <boost/timer.hpp>

#include "ygz/Basic.h"
#include "ygz/Algorithm.h"

using namespace std; 
using namespace cv; 

/*********************************************
 * 测试 orb 提取和匹配是否正确
 * ******************************************/

int main ( int argc, char** argv )
{
    if ( argc< 2 ) {
        cout <<"usage: test_orb_match path_to_TUM_dataset [index1] [index2]" <<endl;
        return 1;
    }
    
    int index1 = 1;
    int index2 = 2;
    
    if ( argc == 3 )
        index1 = std::atoi(argv[2]);
    if ( argc == 4 ) {
        index1 = std::atoi(argv[2]);
        index2 = std::atoi(argv[3]);
    }
    
    ifstream fin( string(argv[1])+"/associate.txt" ); 
    vector<string> rgbFiles, depthFiles;
    vector<double> rgbTime, depthTime; 
    while( !fin.eof() ) {
        double timeRGB, timeDepth;
        string rgbfile, depthfile;
        fin>>timeRGB>>rgbfile>>timeDepth>>depthfile;
        rgbFiles.push_back( rgbfile );
        depthFiles.push_back( depthfile );
        rgbTime.push_back(timeRGB);
        depthTime.push_back(timeDepth);
        if ( fin.good() == false ) 
            break;
    }
    
    ygz::Config::SetParameterFile("./config/default.yaml");
    
    Mat color1 = imread( string(argv[1])+string("/")+rgbFiles[index1] );
    Mat color2 = imread( string(argv[1])+string("/")+rgbFiles[index2] );
    
    ygz::FeatureDetector detector;
    detector.LoadParams();
    
    ygz::Frame frame1, frame2;
    frame1._color = color1;
    frame2._color = color2;
    frame1.InitFrame();
    frame2.InitFrame();
    
    boost::timer timer;
    detector.Detect( &frame1 );
    detector.Detect( &frame2 );
    cout<<"detect cost time: "<<timer.elapsed()<<endl;
    cout<<"features: "<<frame1._features.size()<<", "<<frame2._features.size()<<endl;
    
    // compute the BoW vector
    ORBVocabulary vocab;
    vocab.loadFromBinaryFile("./vocab/ORBvoc.bin");
    ygz::Frame::SetORBVocabulary(&vocab);
    frame1.ComputeBoW();
    frame2.ComputeBoW();
    
    ygz::Matcher matcher;
    map<int, int> matches; 
    timer.restart();
    matcher.SearchByBoW( &frame1, &frame2, matches );
    LOG(INFO)<<"Match BoW cost time: "<<timer.elapsed()<<endl;
    LOG(INFO) << "Total matches: "<<matches.size()<<endl;
    
    // Draw the matches 
    Mat img_show(color1.rows, 2*color1.cols, CV_8UC3); 
    color1.copyTo( img_show(Rect(0,0,color1.cols,color1.rows)) );
    color2.copyTo( img_show(Rect(color1.cols,0,color1.cols,color1.rows)) );
    
    for ( ygz::Feature* fea : frame1._features )
    {
        circle( img_show, 
            Point2f(fea->_pixel[0], fea->_pixel[1]),
            2,Scalar(0,0,250),2 );
    }
    
    for ( ygz::Feature* fea : frame2._features )
    {
        circle( img_show, 
            Point2f(color2.cols+fea->_pixel[0], fea->_pixel[1]),
            2,Scalar(0,0,250),2 );
    }
    
    for ( auto& m:matches ) 
    {
        circle( img_show, 
            Point2f(frame1._features[m.first]->_pixel[0], frame1._features[m.first]->_pixel[1]),
            2,Scalar(0,250,0),2 );
        circle( img_show, 
            Point2f(color1.cols+frame2._features[m.second]->_pixel[0], frame2._features[m.second]->_pixel[1]), 
            2,Scalar(0,250,0),2 );
        line( img_show,
            Point2f(frame1._features[m.first]->_pixel[0], frame1._features[m.first]->_pixel[1]),
            Point2f(color2.cols+frame2._features[m.second]->_pixel[0], frame2._features[m.second]->_pixel[1]), 
            Scalar(0,250,0),1
        );
    }
    imshow("matches", img_show );
    waitKey(0);
    
    return 0;
}