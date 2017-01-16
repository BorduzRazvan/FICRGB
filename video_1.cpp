#include <sstream>
#include <string>
#include <iostream>
//#include <opencv2\highgui.h>
#include "opencv2/highgui/highgui.hpp"
//#include <opencv2\cv.h>
#include "opencv2/opencv.hpp"


#include <cstdio>
#include <cstring>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <math.h>


#define HOST_IP "193.226.12.217" // that should do for now
#define PORTNUM 20231          // daytime
#define BUFSIZE 64
#define PIXEL_ERROR_MARGIN 5

using namespace std;
using namespace cv;
//initial min and max HSV filter values.
//these will be changed using trackbars
int H_MIN = 0;
int H_MAX = 256;
int S_MIN = 0;
int S_MAX = 256;
int V_MIN = 0;
int V_MAX = 256;
//default capture width and height
const int FRAME_WIDTH = 640;
const int FRAME_HEIGHT = 480;
//max number of objects to be detected in frame
const int MAX_NUM_OBJECTS = 50;
//minimum and maximum object area
const int MIN_OBJECT_AREA = 20 * 20;
const int MAX_OBJECT_AREA = FRAME_HEIGHT*FRAME_WIDTH / 1.5;
//names that will appear at the top of each window
const std::string windowName = "Original Image";
const std::string windowName1 = "HSV Image";
const std::string windowName2 = "Thresholded Image";
const std::string windowName22 = "Thresholded2 Image";
const std::string windowName3 = "After Morphological Operations";
const std::string trackbarWindowName = "Trackbars";


typedef struct moves { 
  char forward[2];
  char back[2]; 
  char left[2];
  char right[2];
} moves; 


void on_mouse(int e, int x, int y, int d, void *ptr)
{
	if (e == EVENT_LBUTTONDOWN)
	{
		cout << "Left button of the mouse is clicked - position (" << x << ", " << y << ")" << endl;
	}
}

void on_trackbar(int, void*)
{//This function gets called whenever a
 // trackbar position is changed
}

string intToString(int number) {


	std::stringstream ss;
	ss << number;
	return ss.str();
}

void createTrackbars() {
	//create window for trackbars


	namedWindow(trackbarWindowName, 0);
	//create memory to store trackbar name on window
	char TrackbarName[50];
	sprintf(TrackbarName, "H_MIN", H_MIN);
	sprintf(TrackbarName, "H_MAX", H_MAX);
	sprintf(TrackbarName, "S_MIN", S_MIN);
	sprintf(TrackbarName, "S_MAX", S_MAX);
	sprintf(TrackbarName, "V_MIN", V_MIN);
	sprintf(TrackbarName, "V_MAX", V_MAX);
	//create trackbars and insert them into window
	//3 parameters are: the address of the variable that is changing when the trackbar is moved(eg.H_LOW),
	//the max value the trackbar can move (eg. H_HIGH),
	//and the function that is called whenever the trackbar is moved(eg. on_trackbar)
	//                                  ---->    ---->     ---->
	createTrackbar("H_MIN", trackbarWindowName, &H_MIN, H_MAX, on_trackbar);
	createTrackbar("H_MAX", trackbarWindowName, &H_MAX, H_MAX, on_trackbar);
	createTrackbar("S_MIN", trackbarWindowName, &S_MIN, S_MAX, on_trackbar);
	createTrackbar("S_MAX", trackbarWindowName, &S_MAX, S_MAX, on_trackbar);
	createTrackbar("V_MIN", trackbarWindowName, &V_MIN, V_MAX, on_trackbar);
	createTrackbar("V_MAX", trackbarWindowName, &V_MAX, V_MAX, on_trackbar);


}
void drawObject(int x, int y, Mat &frame) {

	//use some of the openCV drawing functions to draw crosshairs
	//on your tracked image!

	//UPDATE:JUNE 18TH, 2013
	//added 'if' and 'else' statements to prevent
	//memory errors from writing off the screen (ie. (-25,-25) is not within the window!)

	circle(frame, Point(x, y), 20, Scalar(0, 255, 0), 2);
	if (y - 25 > 0)
		line(frame, Point(x, y), Point(x, y - 25), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(x, 0), Scalar(0, 255, 0), 2);
	if (y + 25 < FRAME_HEIGHT)
		line(frame, Point(x, y), Point(x, y + 25), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(x, FRAME_HEIGHT), Scalar(0, 255, 0), 2);
	if (x - 25 > 0)
		line(frame, Point(x, y), Point(x - 25, y), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(0, y), Scalar(0, 255, 0), 2);
	if (x + 25 < FRAME_WIDTH)
		line(frame, Point(x, y), Point(x + 25, y), Scalar(0, 255, 0), 2);
	else line(frame, Point(x, y), Point(FRAME_WIDTH, y), Scalar(0, 255, 0), 2);

	putText(frame, intToString(x) + "," + intToString(y), Point(x, y + 30), 1, 1, Scalar(0, 255, 0), 2);
	//cout << "x,y: " << x << ", " << y;

}
void morphOps(Mat &thresh) {

	//create structuring element that will be used to "dilate" and "erode" image.
	//the element chosen here is a 3px by 3px rectangle

	Mat erodeElement = getStructuringElement(MORPH_RECT, Size(3, 3));
	//dilate with larger element so make sure object is nicely visible
	Mat dilateElement = getStructuringElement(MORPH_RECT, Size(8, 8));

	erode(thresh, thresh, erodeElement);
	erode(thresh, thresh, erodeElement);


	dilate(thresh, thresh, dilateElement);
	dilate(thresh, thresh, dilateElement);



}
void trackFilteredObject(int &x, int &y, Mat threshold, Mat &cameraFeed) {

	Mat temp;
	threshold.copyTo(temp);
	//these two vectors needed for output of findContours
	vector< vector<Point> > contours;
	vector<Vec4i> hierarchy;
	//find contours of filtered image using openCV findContours function
	findContours(temp, contours, hierarchy, CV_RETR_CCOMP, CV_CHAIN_APPROX_SIMPLE);
	//use moments method to find our filtered object
	double refArea = 0;
	bool objectFound = false;
	if (hierarchy.size() > 0) {
		int numObjects = hierarchy.size();
		//if number of objects greater than MAX_NUM_OBJECTS we have a noisy filter
		if (numObjects < MAX_NUM_OBJECTS) {
			for (int index = 0; index >= 0; index = hierarchy[index][0]) {

				Moments moment = moments((cv::Mat)contours[index]);
				double area = moment.m00;

				//if the area is less than 20 px by 20px then it is probably just noise
				//if the area is the same as the 3/2 of the image size, probably just a bad filter
				//we only want the object with the largest area so we safe a reference area each
				//iteration and compare it to the area in the next iteration.
				if (area > MIN_OBJECT_AREA && area<MAX_OBJECT_AREA && area>refArea) {
					x = moment.m10 / area;
					y = moment.m01 / area;
					objectFound = true;
					refArea = area;
				}
				else objectFound = false;


			}
			//let user know you found an object
			if (objectFound == true) {
				putText(cameraFeed, "Tracking Object", Point(0, 50), 2, 1, Scalar(0, 255, 0), 2);
				//draw object location on screen
				//cout << x << "," << y;
				drawObject(x, y, cameraFeed);

			}


		}
		else putText(cameraFeed, "TOO MUCH NOISE! ADJUST FILTER", Point(0, 50), 1, 2, Scalar(0, 0, 255), 2);
	}
}

void send_command(char *command, int to_sleep, int socket)
{
    /* Clear out needed memory */
    char buffer[BUFSIZE];
    memset(buffer, 0, BUFSIZE);
    /* Build the command for robot */
    strcpy(buffer,command);
    strcat(buffer,"\n");
     int n;
   /* Send the command to robot */
  if(n=write(socket,buffer,BUFSIZE) < 0)
   {
   perror("ERROR");
   }
   /* Wait until the next command */
  sleep(to_sleep);
}

double get_panta(double x1, double y1, double x2, double y2) 
{
	return (y2-y1)/(x2-x1);
}

//x_c, y_c coordinates of my head
//x_o, y_o coordinates of my center (origin)
//x_i, y_i coordinates of the enemies center
double get_relative_angle(double x_c, double y_c, double x_o, double y_o, double x_e, double y_e)
{
	double m1, m2;
	m1 = get_panta(x_c, y_c, x_o, y_o);
	m2 = get_panta(x_o, y_o, x_e, y_e);
	
	return atan(fabs((m1-m2)/(1+m1*m2)));
}

double get_time_from_angle(double angle)
{
	return (2*angle)/M_PI;
}

void rotate(double x_c, double y_c, double x_o, double y_o, double x_i, double y_i, int socket)
{
	double y1, y2, x1, x2;
	double t;
	x1 = fabs(x_o - x_c);
	x2 = fabs(x_o - x_i);
	y1 = fabs(y_o - y_c);
	y2 = fabs(y_o - y_i);
	
	if(x1 <= PIXEL_ERROR_MARGIN && x2 <= PIXEL_ERROR_MARGIN)
	{
		if((y_c < y_o && y_o < y_i) || (y_i < y_o < y_c))
		{
			//turn 180 degrees - robots are facing opposite sides on the OY axis
			t = get_time_from_angle(M_PI);
			send_command("r",t,socket);
			return;
		}
		else
		{
			//robots are face to face on the OY axis
			return;
		}
	}
	else
	{
		if(y1 <= PIXEL_ERROR_MARGIN && y2 <= PIXEL_ERROR_MARGIN)
		{
			if((x_c < x_o && x_o < x_i) || (x_i < x_o < x_c))
			{
				//turn 180 degrees - robots are facing opposite sides on the OX axis
				t = get_time_from_angle(M_PI);
				send_command("r",t,socket);
				return;
			}
			else
			{
				//robots are face to face on the OY axis
				return;
			}
		}
	}
	
	if(y_o > y_c)
	{
		if(x_o < x_c && x_c < x_i)
		{
			t = get_time_from_angle(get_relative_angle(x_c, y_c, x_o, y_o, x_i, y_i));
			send_command("r",t,socket);
			return;
		}
		else
		{
			if(x_c < x_o && x_o < x_i)
			{
				t = get_time_from_angle(M_PI - get_relative_angle(x_c, y_c, x_o, y_o, x_i, y_i));
				send_command("r",t,socket);
				return;
			}
		}
	}
	
	if(y_o < y_c)
	{
		if(x_c < x_o && x_o < x_i)
		{
			t = get_time_from_angle(M_PI - get_relative_angle(x_c, y_c, x_o, y_o, x_i, y_i));
			send_command("l",t,socket);
			return;
		}
		else
		{
			if(x_o < x_c && x_c < x_i)
			{
				t = get_time_from_angle(get_relative_angle(x_c, y_c, x_o, y_o, x_i, y_i));
				send_command("r",t,socket);
				return;
			}
		}
	}
}

int main(int argc, char* argv[])
{

  if((argc != 3) || (strcmp(argv[1],argv[2])==0))
  { 
    return -1; 
  }
   
  int player1_min[3]={0,0,0}; 
  int player1_max[3]={0,0,0};
  int player2_min[3]={0,0,0};
  int player2_max[3]={0,0,0}; 
  
  switch(atoi(argv[1]))
  { 
    case 0: // blue 
        player1_min[0]=92;
        player1_min[1]=0;
        player1_min[2]=130; 
        
        player1_max[0]=224;
        player1_max[1]=256;
        player1_max[2]=256;
        break;
    case 1: // red 
    
        player1_min[0]=0;
        player1_min[1]=136;
        player1_min[2]=204; 
        
        player1_max[0]=92;
        player1_max[1]=239;
        player1_max[2]=256;
        break; 
    case 2: 
    
        player1_min[0]=59;
        player1_min[1]=27;
        player1_min[2]=130; 
        
        player1_max[0]=69;
        player1_max[1]=256;
        player1_max[2]=256;
        break; 
    default : 
      return -1;
  }
  
  switch(atoi(argv[2]))
  { 
    case 0: // blue 
    
        player2_min[0]=92;
        player2_min[1]=0;
        player2_min[2]=130; 
        
        player2_max[0]=224;
        player2_max[1]=256;
        player2_max[2]=256;
        break;
    case 1: // red 
        player2_min[0]=0;
        player2_min[1]=136;
        player2_min[2]=204; 
        
        player2_max[0]=92;
        player2_max[1]=239;
        player2_max[2]=256;
        break; 
    case 2: 
        player2_min[0]=59;
        player2_min[1]=27;
        player2_min[2]=130; 
        
        player2_max[0]=69;
        player2_max[1]=256;
        player2_max[2]=256;
        break; 
    default : 
        return -1;
  }
  
 // Connect to the robot 
     int sockfd, n;
    struct sockaddr_in remote;
    /* Clear out needed memory */

    memset(&remote, 0, sizeof(remote));

    /* Fill in required details in the socket structure */
    remote.sin_family = AF_INET;
    remote.sin_port = htons(PORTNUM);
    remote.sin_addr.s_addr = inet_addr(HOST_IP);
    /* Create a socket */
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0) {
        perror("socket");
   //     return -1;
    }
    /* Connect to remote host */
    if(connect(sockfd, (struct sockaddr *) &remote, sizeof(remote)) < 0) {
        perror("connect");
   //     return -1;
    }
    
   send_command("f",1,sockfd);
   send_command("s",1,sockfd);
   send_command("b",1,sockfd);
   send_command("s",1,sockfd);
      
	//some boolean variables for different functionality within this
	//program
	bool trackObjects = true;
	bool useMorphOps = true;

	Point p;
	//Matrix to store each frame of the webcam feed
	Mat cameraFeed;
	//matrix storage for HSV image
	Mat HSV;
	//matrix storage for binary threshold image
	Mat threshold;
  Mat threshold2;
  Mat threshold3; 
	//x and y values for the location of the object
	int x1 = 0, y1 = 0;
	int x2 = 0, y2 = 0;
	//create slider bars for HSV filtering
	createTrackbars();
	//video capture object to acquire webcam feed
	VideoCapture capture;
	//open capture object at location zero (default location for webcam)
	capture.open("rtmp://172.16.254.63/live/live");
	//set height and width of capture frame
	capture.set(CV_CAP_PROP_FRAME_WIDTH, FRAME_WIDTH);
	capture.set(CV_CAP_PROP_FRAME_HEIGHT, FRAME_HEIGHT);
	//start an infinite loop where webcam feed is copied to cameraFeed matrix
	//all of our operations will be performed within this loop



	
	while (1) {


		//store image to matrix
		capture.read(cameraFeed);
   if(cameraFeed.empty())
   { 
   return 1; 
   } 
  
		//convert frame from BGR to HSV colorspace
		cvtColor(cameraFeed, HSV, COLOR_BGR2HSV);
		//filter HSV image between values and store filtered image to
		//threshold matrix - color 1 blue (224, 256, 256) 
		inRange(HSV, Scalar(player1_min[0],player1_min[1], player1_min[2]), Scalar(player1_max[0], player1_max[1],player1_max[2]), threshold);
		//threshold matrix_2 - color 2 (MInS, MAXs ) green
	 	inRange(HSV, Scalar(player2_min[0],player2_min[1], player2_min[2]), Scalar(player2_max[0], player2_max[1],player2_max[2]), threshold2);
		//perform morphological operations on thresholded image to eliminate noise
		//and emphasize the filtered object(s)
		if (useMorphOps)
   {
			morphOps(threshold);
      morphOps(threshold2);
      
   }
    //pass in thresholded frame to our object tracking function
		//this function will return the x and y coordinates of the
		//filtered object
		if (trackObjects)
   {
			trackFilteredObject(x1, y1, threshold,  cameraFeed);
			trackFilteredObject(x2, y2, threshold2, cameraFeed);
   }
		//show frames
		imshow(windowName2, threshold);
		imshow(windowName, cameraFeed);
		//imshow(windowName1, HSV);
		setMouseCallback("Original Image", on_mouse, &p);
		//delay 30ms so that screen can refresh.
		//image will not appear without this waitKey() command
		waitKey(30);
	}

	return 0;
}