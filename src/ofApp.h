#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include "ofxCv.h"
#include "FaceOsc.h"

class ofApp : public ofBaseApp, public FaceOsc {

    public:

		void loadSettings();

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
		ofVideoGrabber vidGrabber;
		ofPixels photoFrameNeutral;
		ofTexture photoFrameNeutralTexture;
		ofPixels photoFrameIdeal;
		ofTexture photoFrameIdealTexture;

		// video, face and window dimension variables
        int camWidth, camHeight;
		int windowCenterW, windowCenterH;
		int sourceWidth, sourceHeight;
		float mouthHeight, mouthWidth;

		ofBaseVideoDraws *videoSource;
		ofxFaceTracker tracker;
		ofMatrix4x4 rotationMatrix;
		ofxPanel gui;

		void setVideoSource(bool useCamera);

		// boolean variables
		bool bUseCamera;
		bool bPaused;
		bool bDrawMesh;
		bool bGuiVisible;
		bool bHasPhotoNeutral;
		bool bHasPhotoIdeal;
		bool bShowText;
        bool bAllowBackwards; // to prevent the user to go back in the slides

		// image variables
		ofImage text01, text02, text03, text04, text05, text06, text07, text08;
		ofImage logoSmile, logoSmileInverted;
		ofImage textSmile, textNatural, textProjection, textBoth, textHappiness;
		int momentNum;
		
		// timer variables
		float startTime;
		float endTimeNeutralPhoto, endTimeIdealPhoto, endTimeFinalReset;
		bool bTimerNeutralReached, bTimerIdealReached, bTimerEndReached;

		// alpha oscillation variables
		float t;
		float alphaOscillation;

		// mouth percentage values variables
		float pctWidth, pctHeight, pctMouth; // pct means percentage
};
