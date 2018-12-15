#ifndef HANDDETECT_HPP
#define HANDDETECT_HPP

#include <iostream>
#include <opencv/cv.hpp>
#include <opencv2/highgui.hpp>
#include "webcam.hpp"

#define FRAMERUN 50

using namespace cv;
using namespace std;

class HandDetect {
public:
  size_t count_bgupdate = 0;
  int tips_position[10][2], oldtips[FRAMERUN][11][4],
      oldtipflag = 0; // oldtips[FRAMERUN][i][0] -> x-cordinate of tip, [i][1]
                      // -> y-cordinate of tip, [i][2] -> direction of movement,
                      // [i][3] -> base frame
  int posmax = 0;
  float speed[10][7]; // speed[][0]->speed; [][1]->forward movement duration;
                      // [][2]->backward movement duration; [][3]->point of
                      // change (x); [][4]->point of change (y); [][5]->speed
                      // flag; [][6]->direction at point of deflection

  HandDetect(WebCam& webcam);
  int getTipsCount(Mat img, int width, int height);
  void correlatedTips(int);
  void getKeyPress(Mat frame) const;
};

#endif