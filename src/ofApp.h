#pragma once

#include "ofMain.h"
#include "ofxGui.h"
#include "ofxCv.h"
#include "FaceOsc.h"

class ofApp : public ofBaseApp, public FaceOsc {

    public:

		void loadSettings();
		//void videoCenter();

        void setup();
        void update();
        void draw();

        void keyPressed(int key);
/*		void keyReleased(int key);
        void mouseMoved(int x, int y);
        void mouseDragged(int x, int y, int button);
        void mousePressed(int x, int y, int button);
        void mouseReleased(int x, int y, int button);
        void mouseEntered(int x, int y);
        void mouseExited(int x, int y);
        void windowResized(int w, int h);
        void dragEvent(ofDragInfo dragInfo);
        void gotMessage(ofMessage msg);
*/  
        ofVideoGrabber vidGrabber;
		ofPixels photoFrameNeutral;
		ofTexture photoFrameNeutralTexture;
		ofPixels photoFrameIdeal;
		ofTexture photoFrameIdealTexture;

        int camWidth, camHeight;
		int windowCenterW, windowCenterH;
		int sourceWidth, sourceHeight;
		float mouthHeight, mouthWidth;

		ofBaseVideoDraws *videoSource;
		ofxFaceTracker tracker;
		ofMatrix4x4 rotationMatrix;
		ofxPanel gui;

		void setVideoSource(bool useCamera);

		bool bUseCamera;
		bool bPaused;
		bool bDrawMesh;
		bool bGuiVisible;
		bool bHasPhotoNeutral;
		bool bHasPhotoIdeal;
		bool bTextVisible;

		ofTrueTypeFont verdana14;
		ofTrueTypeFont verdana40;

		string sHello, sPressKey;
		string sBestSmile, sDoBetter, sAlmost, sGreat;
		
};
