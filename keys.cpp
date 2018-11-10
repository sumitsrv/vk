
#define CV_NO_BACKWARD_COMPATIBILITY

#include <opencv2/highgui.hpp>
#include <opencv/cv.hpp>
#include <iostream>
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include "colorEdge.hpp"
#include "edge.hpp"

#define FG 255
#define BG 0
#define CREATE_DATA_SET 0
#define ON_WORK 1
#define THRESH_1 150
#define THRESH_2 200

using namespace std;
using namespace cv;

class Keys {

    int count1 = 0, count2 = 0, count3=0;
    CvMemStorage* storage=0;

    double angle( CvPoint* pt1, CvPoint* pt2, CvPoint* pt0 )
    {
        double dx1 = pt1->x - pt0->x;
        double dy1 = pt1->y - pt0->y;
        double dx2 = pt2->x - pt0->x;
        double dy2 = pt2->y - pt0->y;
        return (dx1*dx2 + dy1*dy2)/sqrt((dx1*dx1 + dy1*dy1)*(dx2*dx2 + dy2*dy2) + 1e-10);
    }

    CvSeq* findSquares4( Mat img, Mat storage )
    {
        CvSeq* contours;
        int i, c, l, N = 20;
        Size sz = Size( img.cols & -2, img.rows & -2 );
        Mat timg = img.clone();
        Mat extimg = img.clone();
        //IplImage* extra = cvCreateImage(sz, CV_16S, 3);;

        cvtColor(extimg, timg, CV_BGR2HSV);
        Mat gray = Mat( sz, 8, 1 );
        // IplImage* pyr = cvCreateImage( cvSize(sz.width/2, sz.height/2), 8, 3 );
        Mat tgray;
        CvSeq* result;
        double s, t, ang[4];
        // create empty sequence that will contain points -
        // 4 points per square (the square's vertices)
        CvSeq* squares = cvCreateSeq( 0, sizeof(CvSeq), sizeof(CvPoint), storage );

        // select the maximum ROI in the image
        // with the width and height divisible by 2
        Rect roi( 0, 0, sz.width, sz.height );
        timg = Mat(roi);

        tgray = Mat( sz, 8, 1 );

        // find squares in every color plane of the image
        for( c = 2; c < 4; c++ )
        {
            // extract the c-th color plane
            if(c<3)
            {
//                cvSetImageCOI( timg, c+1 );
                copy( timg, tgray, 0 );
            }
            else
            {
//                cvSetImageCOI(timg, 0);
                cvtColor(extimg, tgray, CV_BGR2GRAY);
                //colorEdge(timg, tgray);
                //cvEqualizeHist( tgray, tgray );
            }
            // try several threshold levels
            for( l = 0; l < N; l++ )
            {
                // hack: use Canny instead of zero threshold level.
                // Canny helps to catch squares with gradient shading

                if( l == 0 )
                {
                    // apply Canny. Take the upper threshold from slider
                    // and set the lower to 0 (which forces edges merging)
                    Canny( tgray, gray, THRESH_1, THRESH_2, 3 );
                    remove_loose_ends(gray);
                }
                else
                {
                    // apply threshold if l!=0:
                    //     tgray(x,y) = gray(x,y) < (l+1)*255/N ? 255 : 0
                    //cvErode(gray, gray, 0, 1);
                    threshold( tgray, gray, (l+1)*255/N, 255, CV_THRESH_BINARY );
                    remove_loose_ends(gray);
                    //markLines(gray, gray, 255);
                }
                //cvShowImage("juzz",gray);
                // find contours and store them all as a list

                findContours( gray, storage,
                                CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE, Point(0,0) );

                // test each contour
                while( storage )
                {
                    count3++;
                    // approximate contour with accuracy proportional
                    // to the contour perimeter
                    result = approxPolyDP( storage, storage,  8, true );

                    // square contours should have 4 vertices after approximation
                    // relatively large area (to filter out noisy contours)
                    // and be convex.
                    // Note: absolute value of an area is used because
                    // area may be positive or negative - in accordance with the
                    // contour orientation
                    if( result->total == 4 &&
                            cvContourArea(result,CV_WHOLE_SEQ,0) > 400 && cvContourArea(result,CV_WHOLE_SEQ,0) < 8000 &&
                            cvCheckContourConvexity(result) )
                    {
                        count1++;
                        s=0;
                        ang[0] = 0, ang[1] = 0, ang[2] = 0, ang[3] = 0 ;
                        for( i = 0; i < 5; i++ )
                        {
                            // find minimum angle between joint
                            // edges (maximum of cosine)
                            if( i >= 2 )
                            {
                                t = fabs(angle(
                                             (CvPoint*)cvGetSeqElem( result, i ),
                                             (CvPoint*)cvGetSeqElem( result, i-2 ),
                                             (CvPoint*)cvGetSeqElem( result, i-1 )));
                                s = s > t ? s : t;
                                ang[i-2] = t;
                            }
                        }

                        // vertices to resultant sequence
                        if( s < 0.6 )
                        {
                            count2++;
                            for( i = 0; i < 4; i++ )
                                cvSeqPush( squares, (CvPoint*)cvGetSeqElem( result, i ));

                        }
                    }

                    // take the next contour
                    contours = contours->h_next;
                }
            }
        }

        //printf("count3 = %d\t count1 = %d\t count2 = %d\t",count3, count1, count2);
        // release all the temporary images
//        cvReleaseImage( &gray );
//        //cvReleaseImage( &pyr );
//        cvReleaseImage( &tgray );
//        cvReleaseImage( &timg );
//        cvReleaseImage( &extimg);
        return squares;
    }


    // the function draws all the squares in the image
    void drawSquares( IplImage* img, CvSeq* squares, IplImage* original, int mode )
    {
        char* filepath= (char*)calloc(20,1) , *filename=(char*)calloc(50,1);
        int filecount=0 ;
        FILE *keyinfo;
        keyinfo = fopen("./ocr_temp_data/keyinfo.dat","a+");

        if(mode == CREATE_DATA_SET)
        {
            FILE *keycount;;
            keycount = fopen("./ocr_temp_data/keycount.dat","r");
            fscanf(keycount, "%d", &filecount);
            //printf("%d\t",CREATE_DATA_SET);
            fclose(keycount);
            filepath = strcat(filepath, "./ocr_temp_data/keysimages/key");
        }

        CvSeqReader reader;
        IplImage* cpy = cvCloneImage( img );
        //IplImage* cpy_gray = cvCreateImage(cvSize(img->width, img->height), 8, 1);
        //cvCvtColor(cpy, cpy_gray, CV_BGR2GRAY);
        int i, j, k, temp;

        // initialize reader of the sequence
        cvStartReadSeq( squares, &reader, 0 );

        // read 4 sequence elements at a time (all vertices of a square)
        for( i = 0; i < squares->total; i += 4 )
        {
            CvPoint pt[4], *rect = pt;
            int count = 4;
            int minx=1000, miny=1000, maxx=0, maxy=0, x[4], y[4];

            // read 4 vertices
            CV_READ_SEQ_ELEM( pt[0], reader );
            CV_READ_SEQ_ELEM( pt[1], reader );
            CV_READ_SEQ_ELEM( pt[2], reader );
            CV_READ_SEQ_ELEM( pt[3], reader );

            // draw the square as a closed polyline
            cvPolyLine( cpy, &rect, &count, 1, 1, CV_RGB(0,255,0), 1, CV_AA, 0 );
            for(j=0; j<4; j++)
            {
                x[j] = (&pt[j])->x;
                y[j] = (&pt[j])->y;
            }

            for(j=0; j<4; j++)
            {
                for(k=0; k<4; k++)
                {
                    if(x[j]>x[k])
                    {
                        temp=x[j];
                        x[j]=x[k];
                        x[k]=temp;
                    }
                }
            }
            for(j=0; j<4; j++)
            {
                for(k=0; k<4; k++)
                {
                    if(y[j]>y[k])
                    {
                        temp=y[j];
                        y[j]=y[k];
                        y[k]=temp;
                    }
                }
            }
            minx = x[2];
            maxx = x[1];
            miny = y[2];
            maxy = y[1];

            if(mode == CREATE_DATA_SET)
            {
                IplImage* keysimg = cvCreateImage(cvSize(maxx-minx, maxy-miny), 8, 1);
                cvSetImageROI(original, cvRect(minx, miny, maxx-minx, maxy-miny));
                cvCopy(original, keysimg, 0);
                sprintf(filename, "%s%d.jpg", filepath, filecount);
                fprintf(keyinfo, "%d\t%s\t%d %d %d %d %d %d %d %d\n", filecount++, filename, (&pt[0])->x, (&pt[0])->y, (&pt[1])->x, (&pt[1])->y, (&pt[2])->x, (&pt[2])->y, (&pt[3])->x, (&pt[3])->y);
                cvSaveImage(filename, keysimg);
                cvReleaseImage(&keysimg);
            }
        }

        // show the resultant image
        if(mode == CREATE_DATA_SET)
        {
            FILE *keycount;
            keycount = fopen("./ocr_temp_data/keycount.dat","w");
            fprintf(keycount, "%d", filecount);
            fclose(keycount);
        }

        fclose(keyinfo);

        cvShowImage( "Keys Detection", cpy );
        cvReleaseImage( &cpy );
    }


    int locate(IplImage* img0, IplImage* original)
    {
        int c;
        IplImage* img = 0;
        // create memory storage that will contain all the dynamic data
        storage = cvCreateMemStorage(0);

        // load i-th image
        //img0 = img1;
        if( !img0 )
        {
            printf("Couldn't load\n");
        }
        img = cvCloneImage( img0 );

        // create window and a trackbar (slider) with parent "image" and set callback
        // (the slider regulates upper threshold, passed to Canny edge detector)
        cvNamedWindow( "Keys Detection", 1 );
        cvMoveWindow("Keys Detection", img->width, 0);
        // find and draw the squares
        drawSquares( img, findSquares4( img, storage ), original, CREATE_DATA_SET );

        // wait for key.
        // Also the function cvWaitKey takes care of event processing
        c = cvWaitKey(0);
        // release both images
        cvReleaseImage( &img );
        // clear memory storage - reset free space position
        cvClearMemStorage( storage );

        cvDestroyWindow( "Keys Detection" );

        return 0;
    }


    void remove_loose_ends(Mat canny)
    {
        IplImage* clone = cvCloneImage(canny);

        uchar* data= (uchar*)canny->imageData;
        uchar* cldata= (uchar*)clone->imageData;

        int height = canny->height;
        int width = canny->width;
        int step= canny->widthStep;

        int i, j, k, count = 0;

        for(i=0; i<height; i++)
        {
            for(j=0; j<width; j++)
            {
                count = 0;
                if(data[i*step + j] == BG)
                {
                    for(k=-1; k<2; k++)
                    {
                        if(cldata[(i+k)*step + (j+k)] == BG)
                        {
                            count++;
                        }
                        if(cldata[i*step +j+k] == BG)
                        {
                            count++;
                        }
                        if(cldata[(i+k)*step +j] == BG)
                        {
                            count++;
                        }
                    }
                    //printf("%d\t",count);
                    if(count <4)
                        data[i*step + j] = FG;
                }

            }
        }
        cvReleaseImage(&clone);
    }
};