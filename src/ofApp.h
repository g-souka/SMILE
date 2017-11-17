#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include "ofxCv.h"
#include "FaceOsc.h"

class ofApp : public ofBaseApp, public FaceOsc {

    public:

		void loadSettings();	// this void is used to load the settings present in the settings.xml file
								// this was imported from the face tracking examples of the addon

        void setup();
        void update();
        void draw();

        void keyPressed(int key);
/*		void keyReleased(int key);
        void mouseMoved(int x, int y);
        void mouseDragged(int x, int y, int button);
*/      void mousePressed(int x, int y, int button);
/*	    void mouseReleased(int x, int y, int button);
        void mouseEntered(int x, int y);
        void mouseExited(int x, int y);
        void windowResized(int w, int h);
        void dragEvent(ofDragInfo dragInfo);
 */     void gotMessage(ofMessage msg);
  
    
		// webcam input and video frame variables
		ofVideoGrabber vidGrabber;			// the video grabber is used to grab the camera input
		ofPixels photoFrameNeutral;			// to store the information for the Neutral and Ideal picture,
		ofTexture photoFrameNeutralTexture;	// obtained from the camera, ofPixels is used - this information is stored in the cpu
		ofPixels photoFrameIdeal;			// in order to draw the ofPixels, ofTexture is used, this information is stored in the gpu
		ofTexture photoFrameIdealTexture;	// it is from these ofPixels variables that the external images are generated

		// video, face and window dimension variables
        int camWidth, camHeight;			// in order to simplify the placement of elements
		int windowCenterW, windowCenterH;	// these variables were used to have a responsive behaviour
		int sourceWidth, sourceHeight;		// to the window size and input sizes
		float mouthWidth, mouthHeight;

		// face tracking variables
		ofBaseVideoDraws *videoSource;
		ofxFaceTracker tracker;
		ofMatrix4x4 rotationMatrix;

		// gui for OSC communication variable
		ofxPanel gui;

		// video source void (also imported from the addon examples)
		void setVideoSource(bool useCamera);

		// boolean variables
		bool bUseCamera;		// use a camera
		bool bPaused;			// pause the camera input
		bool bDrawMesh;			// decide if the face mesh is drawn
		bool bGuiVisible;		// toggle the GUI visibility
		bool bHasPhotoNeutral;	// bool that determines if the Neutral photo has been taken
		bool bHasPhotoIdeal;	// bool that determines if the Ideal photo has been taken
		bool bShowText;			// used to toggle the text, used for debugging
        bool bAllowBackwards;	// used to prevent the user to go back in the slides

		// image variables
		ofImage text01, text02, text03, text04, text05, text06, text07, text08;		// these ofImage variables hold the different external images that are used
		ofImage logoSmile, logoSmileInverted;										// all the text and logos are stored in these variables and then shown when called
		ofImage textSmile, textNatural, textProjection, textBoth, textHappiness;

		// moment variables
		int momentNum;			// this variable is used to determine which moment is the app currently in
								// each moment has it's own visual characteristics and this is used to change what is shown
		
		// timer variables
		float startTime;
		float endTimeNeutralPhoto, endTimeIdealPhoto, endTimeFinalReset;	// each momment where a timer is used had a timer specific to that situation
		bool bTimerNeutralReached, bTimerIdealReached, bTimerEndReached;	// the time would be defined in the endTime variables
																			// the bools were used to determine that the timers had been reached

		// alpha oscillation variables
		float t;				// these were used to add a transparency oscillation to the face mesh
		float alphaOscillation;	// that is drawn on top of the users face

		// mouth percentage values variables
		float pctWidth, pctHeight, pctMouth; // pct means percentage
};
