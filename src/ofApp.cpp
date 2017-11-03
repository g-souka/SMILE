#include "ofApp.h"
#include "FaceOsc.h"

using namespace ofxCv;
using namespace cv;


//--------------------------------------------------------------
void ofApp::loadSettings() {
	// if you want to package the app by itself without an outer
	// folder, you can place the "data" folder inside the app in
	// the Resources folder (right click on the app, "show contents")
	//ofSetDataPathRoot("../Resources/data/");

	// setup gui
	bGuiVisible = false;
	gui.setup();
	gui.setName("FaceOSC");
	gui.setPosition(0, 0);
	gui.add(bIncludePose.set("pose", true));
	gui.add(bIncludeGestures.set("gesture", true));
	gui.add(bIncludeAllVertices.set("raw", false));
	gui.add(bNormalizeRaw.set("normalize raw", false));

	// load settings file
	ofXml xml;
	if (!xml.load(ofToDataPath("settings.xml"))) {
		return;
	}

	// expects following tags to be wrapped by a main "faceosc" tag

	bool bUseCamera = true;

	xml.setTo("source");
	if (xml.exists("useCamera")) {
		bUseCamera = xml.getValue("useCamera", false);
	}
	xml.setToParent();

	xml.setTo("camera");
	if (xml.exists("device")) {
		vidGrabber.setDeviceID(xml.getValue("device", 0));
	}
	if (xml.exists("framerate")) {
		vidGrabber.setDesiredFrameRate(xml.getValue("framerate", 30));
	}
	camWidth = xml.getValue("width", 640);
	camHeight = xml.getValue("height", 480);
	vidGrabber.initGrabber(camWidth, camHeight);
	xml.setToParent();

	if (bUseCamera) {
		setVideoSource(true);
	}

	xml.setTo("face");
	if (xml.exists("rescale")) {
		tracker.setRescale(xml.getValue("rescale", 1.0));
	}
	if (xml.exists("iterations")) {
		tracker.setIterations(xml.getValue("iterations", 5));
	}
	if (xml.exists("clamp")) {
		tracker.setClamp(xml.getValue("clamp", 3.0));
	}
	if (xml.exists("tolerance")) {
		tracker.setTolerance(xml.getValue("tolerance", 0.01));
	}
	if (xml.exists("attempts")) {
		tracker.setAttempts(xml.getValue("attempts", 1));
	}
	bDrawMesh = true;
	if (xml.exists("drawMesh")) {
		bDrawMesh = (bool)xml.getValue("drawMesh", 1);
	}
	tracker.setup();
	xml.setToParent();

	xml.setTo("osc");
	host = xml.getValue("host", "localhost");
	port = xml.getValue("port", 8338);
	osc.setup(host, port);
	xml.setToParent();

	xml.clear();
}



//--------------------------------------------------------------
void ofApp::setup(){
	ofSetWindowTitle("SMILE :—)");
	ofSetVerticalSync(true);

	bHasPhotoNeutral = false;
	bHasPhotoIdeal = false;
	
	// print a list of avaiable input devices
    vector<ofVideoDevice> devices = vidGrabber.listDevices();

    for(int i = 0; i < devices.size(); i++){
        if(devices[i].bAvailable){
            ofLogNotice() << devices[i].id << ": " << devices[i].deviceName;
        }else{
            ofLogNotice() << devices[i].id << ": " << devices[i].deviceName << " - unavailable ";
        }
    }

	// camera input size, try to grab at this size
	camWidth = 640;
	camHeight = 480;

	// choose device
	vidGrabber.setDeviceID(1);
    vidGrabber.setDesiredFrameRate(60);
    vidGrabber.initGrabber(camWidth, camHeight);

	// allocate memory for photos
	photoFrameNeutral.allocate(camWidth, camHeight, OF_PIXELS_RGB);
	photoFrameNeutralTexture.allocate(photoFrameNeutral);
	photoFrameIdeal.allocate(camWidth, camHeight, OF_PIXELS_RGB);
	photoFrameIdealTexture.allocate(photoFrameIdeal);

	// face tracker variable configuration
	mouthWidth = tracker.getGesture(ofxFaceTracker::MOUTH_WIDTH);
	mouthHeight = tracker.getGesture(ofxFaceTracker::MOUTH_HEIGHT);

	// font configuration
	ofTrueTypeFont::setGlobalDpi(72);

	verdana14.load("verdana.ttf", 14, true, true);
	verdana14.setLineHeight(18.0f);
	verdana14.setLetterSpacing(1.037);

	verdana40.load("verdana.ttf", 40, true, true);
	verdana40.setLineHeight(48.0f);
	verdana40.setLetterSpacing(1.100);

	// string configuration
	sHello = "HELLO";
	sPressKey = "USE THE ARROWS TO NAVIGATE";
	sBestSmile = "GIVE US YOUR BEST SMILE!";
	sDoBetter = "YOU CAN DO BETTER";
	sAlmost = "ALMOST THERE";
	sGreat = "THAT'S GREAT";

	// xml settings load
	loadSettings();

	// load images
	text01.load("images/text01.png");
	text02.load("images/text02.png");
	text03.load("images/text03.png");
	text04.load("images/text04.png");
	//text05.load("images/text05.png");
	//text06.load("images/text06.png");
	momentNum = 1;
	
	// timer configuration
	bTimerReached = false;
	startTime = ofGetElapsedTimeMillis();
	endTime = 2000;
}


//--------------------------------------------------------------
void ofApp::update(){

	// in order to update in real time
	// the following variables must also be set in update()

	mouthWidth = tracker.getGesture(ofxFaceTracker::MOUTH_WIDTH);
	mouthHeight = tracker.getGesture(ofxFaceTracker::MOUTH_HEIGHT);

	windowCenterW = ofGetWindowWidth() * 0.5;
	windowCenterH = ofGetWindowHeight() * 0.5;

	if (bPaused)
		return;

	videoSource->update();
	if (videoSource->isFrameNew()) {
		tracker.update(toCv(*videoSource));
		sendFaceOsc(tracker);
	}

	vidGrabber.update();

	t = ofGetElapsedTimef();
	alphaOscillation = (sin(t * 0.2 * TWO_PI) * 0.5 + 0.5);
}

//--------------------------------------------------------------
void ofApp::draw(){

	ofBackground(ofColor::fireBrick);

	if (momentNum == 1) {
		text01.draw(windowCenterW - text01.getWidth() * 0.5, windowCenterH - text01.getHeight() * 0.5);
	}

	// when the user presses the key to move to the second moment
	// the first photo labelled Neutral is taken after 2000 ms

	if (momentNum == 2) {
		text02.draw(windowCenterW - text02.getWidth() * 0.5, windowCenterH - text02.getHeight() * 0.5);
		
		if (bHasPhotoNeutral == false) {
			
			float timer = ofGetElapsedTimeMillis() - startTime;
			if (timer >= endTime && !bTimerReached) {
				bTimerReached = true;
			}

			if (bTimerReached) {
				ofPixels & pixels = vidGrabber.getPixels();
				for (int i = 0; i < pixels.size(); i++) {
					photoFrameNeutral[i] = pixels[i];
				}
				photoFrameNeutralTexture.loadData(photoFrameNeutral);
				ofSaveImage(photoFrameNeutral, ofToString(ofGetTimestampString() + " - photoFrameNeutral") + ".jpg");

				bHasPhotoNeutral = true;
			}
		}
	}


	if (momentNum == 3) { text03.draw(windowCenterW - text03.getWidth() * 0.5, windowCenterH - text03.getHeight() * 0.5); }
	if (momentNum == 4) { text04.draw(windowCenterW - text04.getWidth() * 0.5, windowCenterH - text04.getHeight() * 0.5); }

	if (momentNum == 5) {
		ofSetColor(ofColor::mistyRose);
		ofRectangle boundsBestSmile = verdana40.getStringBoundingBox(sBestSmile, 0, 0);
		verdana40.drawString(sBestSmile, ofGetWindowWidth() * 0.5 - boundsBestSmile.width * 0.5, camHeight * 0.25);

		vidGrabber.draw(windowCenterW - (camWidth * 0.5), windowCenterH - (camHeight * 0.5));
	}

	if (tracker.getFound() && momentNum == 5) {

		if (mouthWidth <= 12.0 || mouthHeight <= 2.0) {
			ofRectangle boundsDoBetter = verdana14.getStringBoundingBox(sDoBetter, 0, 0);
			verdana14.drawString(sDoBetter, ofGetWindowWidth() * 0.5 - boundsDoBetter.width * 0.5, ofGetWindowHeight() * 0.6);
		}

		else if (mouthWidth >= 16.0 && mouthHeight >= 4.0) {
			ofRectangle boundsGreat = verdana14.getStringBoundingBox(sGreat, 0, 0);
			verdana14.drawString(sGreat, ofGetWindowWidth() * 0.5 - boundsGreat.width * 0.5, ofGetWindowHeight() * 0.6);

			// implementar TIMER aqui
			// dar feedback sobre a foto ser tirada
			// saltar para o proximo automaticamente

			if (bHasPhotoIdeal == false) {
				ofPixels & pixels = vidGrabber.getPixels();
				for (int i = 0; i < pixels.size(); i++) {
					photoFrameIdeal[i] = pixels[i];
				}
				photoFrameIdealTexture.loadData(photoFrameIdeal);
				ofSaveImage(photoFrameIdeal, ofToString(ofGetTimestampString() + " - photoFrameIdeal") + ".jpg");

				bHasPhotoIdeal = true;
			}
		}

		else {
			ofRectangle boundsAlmost = verdana14.getStringBoundingBox(sAlmost, 0, 0);
			verdana14.drawString(sAlmost, ofGetWindowWidth() * 0.5 - boundsAlmost.width * 0.5, ofGetWindowHeight() * 0.6);
		}

		ofSetColor(0);
		ofDrawBitmapString("sending OSC to " + host + ":" + ofToString(port), 20, ofGetHeight() - 140);
		ofDrawBitmapString("Width: " + ofToString(mouthWidth), 20, ofGetHeight() - 120);
		ofDrawBitmapString("Height: " + ofToString(mouthHeight), 20, ofGetHeight() - 100);

		if (bDrawMesh) {
			ofSetColor(255, 255, 255, 255 * alphaOscillation);
			ofSetLineWidth(1);
			ofPushView();
			ofTranslate(ofGetWindowWidth() * 0.5 - (vidGrabber.getWidth() * 0.5), ofGetWindowHeight() * 0.5 - (vidGrabber.getHeight() * 0.5));
			tracker.getImageMesh().drawWireframe();
			//tracker.getImageMesh().drawFaces();
			ofPopView();
		}
	}

	if (tracker.getFound() == false && momentNum == 5) {
		ofSetColor(0);
		ofDrawBitmapString("Searching for face...", 20, ofGetHeight() - 140);
	}

	if (momentNum == 6) {
		photoFrameNeutralTexture.draw(windowCenterW - camWidth, windowCenterH - (camHeight * 0.5), camWidth, camHeight);
		photoFrameIdealTexture.draw(windowCenterW, windowCenterH - (camHeight * 0.5), camWidth, camHeight);
	}

	if (momentNum == 7) {
		text05.draw(windowCenterW - text05.getWidth() * 0.5, windowCenterH - text05.getHeight() * 0.5);
	}
	
	if (momentNum == 8) {
		text06.draw(windowCenterW - text06.getWidth() * 0.5, windowCenterH - text06.getHeight() * 0.5);

		bHasPhotoNeutral = false;
		bHasPhotoIdeal = false;
	}

	if (bPaused) {
		ofSetColor(0);
		ofDrawBitmapString("Paused", 20, ofGetHeight() - 160);
	}	

	if (bGuiVisible) {
		gui.draw();
	}

	ofSetColor(0);
	ofDrawBitmapString("alphaOscillation " + ofToString(alphaOscillation), 20, ofGetHeight() - 80);
	ofDrawBitmapString("momentNum " + ofToString(momentNum), 20, ofGetHeight() - 60);
	ofDrawBitmapString("Framerate: " + ofToString((int)ofGetFrameRate()), 20, ofGetHeight() - 40);
	ofDrawBitmapString("Press: ' ' to pause, 'g' to toggle gui, 'm' to toggle mesh, 'r' to reset tracker.", 20, ofGetHeight() - 20);
	
}


//--------------------------------------------------------------
void ofApp::keyPressed(int key){

	if(key == 's' || key == 'S'){
        vidGrabber.videoSettings();
    }

	if (key == OF_KEY_LEFT && momentNum > 1)		momentNum--;
	else if (key == OF_KEY_RIGHT && momentNum < 8)	momentNum++;

	switch (key) {
	
		case 'r':
			tracker.reset();
			break;

		case 'm':
			bDrawMesh = !bDrawMesh;
			break;

		case ' ':
			bPaused = !bPaused;
			break;

		case 'g':
			bGuiVisible = !bGuiVisible;
			break;
	}

}


//--------------------------------------------------------------
void ofApp::setVideoSource(bool useCamera) {

	bUseCamera = useCamera;

	if (bUseCamera) {
		videoSource = &vidGrabber;
		sourceWidth = camWidth;
		sourceHeight = camHeight;
	}
}

/*
//--------------------------------------------------------------
void ofApp::keyReleased(int key){
}

//--------------------------------------------------------------
void ofApp::mouseMoved(int x, int y){
}

//--------------------------------------------------------------
void ofApp::mouseDragged(int x, int y, int button){
}


//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

}


//--------------------------------------------------------------
void ofApp::mouseReleased(int x, int y, int button){
}

//--------------------------------------------------------------
void ofApp::mouseEntered(int x, int y){
}

//--------------------------------------------------------------
void ofApp::mouseExited(int x, int y){
}

//--------------------------------------------------------------
void ofApp::windowResized(int w, int h){
}

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
}

//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
}
*/