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
//void ofApp::videoCenter() {
//
//	videoInputCenterW = videoInput.getWidth() * 0.5;
//	videoInputCenterH = videoInput.getHeight() * 0.5);
//
//}

//--------------------------------------------------------------
void ofApp::setup(){
	ofSetWindowTitle("SMILE :—)");
	ofSetVerticalSync(true);

	bHasPhotoNeutral = false;
	bHasPhotoIdeal = false;
	bTextVisible = true;
	
	//we can now get back a list of devices.
    vector<ofVideoDevice> devices = vidGrabber.listDevices();

    for(int i = 0; i < devices.size(); i++){
        if(devices[i].bAvailable){
            ofLogNotice() << devices[i].id << ": " << devices[i].deviceName;
        }else{
            ofLogNotice() << devices[i].id << ": " << devices[i].deviceName << " - unavailable ";
        }
    }

	camWidth = 640;  // try to grab at this size.
	camHeight = 480;

	vidGrabber.setDeviceID(1);
    vidGrabber.setDesiredFrameRate(60);
    vidGrabber.initGrabber(camWidth, camHeight);

	photoFrameNeutral.allocate(camWidth, camHeight, OF_PIXELS_RGB);
	photoFrameNeutralTexture.allocate(photoFrameNeutral);
	photoFrameIdeal.allocate(camWidth, camHeight, OF_PIXELS_RGB);
	photoFrameIdealTexture.allocate(photoFrameIdeal);

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
	sPressKey = "PLEASE PRESS ANY KEY";
	sBestSmile = "GIVE US YOUR BEST SMILE!";
	sDoBetter = "YOU CAN DO BETTER";
	sAlmost = "ALMOST THERE";
	sGreat = "THAT'S GREAT";

	loadSettings();
	
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
}

//--------------------------------------------------------------
void ofApp::draw(){

	ofBackground(ofColor::powderBlue);

	vidGrabber.draw(windowCenterW - (camWidth * 0.5), windowCenterH - (camHeight * 0.5));
	//photoFrameNeutralTexture.draw(windowCenterW - (camWidth * 0.5), camHeight * 0.5 + windowCenterH, camWidth * 0.5, camHeight * 0.5);
	//photoFrameIdealTexture.draw(windowCenterW, camHeight * 0.5 + windowCenterH, camWidth * 0.5, camHeight * 0.5);


	if (bTextVisible) {

		ofSetColor(255, 255, 255, 240);
		ofDrawRectangle(0, 0, ofGetWindowWidth(), ofGetWindowHeight());

		ofSetColor(ofColor::black);
		ofRectangle boundsHello = verdana40.getStringBoundingBox(sHello, 0, 0);
		verdana40.drawString(sHello, ofGetWindowWidth() * 0.5 - boundsHello.width * 0.5, ofGetWindowHeight() * 0.4);
		
		ofRectangle boundsPressKey = verdana40.getStringBoundingBox(sPressKey, 0, 0);
		verdana40.drawString(sPressKey, ofGetWindowWidth() * 0.5 - boundsPressKey.width * 0.5, ofGetWindowHeight() * 0.4 + 48.0f);
	}

	// when the user presses the key to remove the text and initiate the interaction
	// the first photo labelled Neutral is taken after 1000 ms

	else if (bHasPhotoNeutral == false) {
		Sleep(1000);

		ofPixels & pixels = vidGrabber.getPixels();
		for (int i = 0; i < pixels.size(); i++) {
			photoFrameNeutral[i] = pixels[i];
		}
		photoFrameNeutralTexture.loadData(photoFrameNeutral);
		ofSaveImage(photoFrameNeutral, ofToString(ofGetTimestampString() + " - photoFrameNeutral") + ".jpg");

		bHasPhotoNeutral = true;
	}

	if (bTextVisible == false) {
		ofSetColor(ofColor::mistyRose);
		ofRectangle boundsBestSmile = verdana40.getStringBoundingBox(sBestSmile, 0, 0);
		verdana40.drawString(sBestSmile, ofGetWindowWidth() * 0.5 - boundsBestSmile.width * 0.5, camHeight * 0.25);
	}

	if (tracker.getFound() && bTextVisible == false) {

		if (mouthWidth <= 12.0 || mouthHeight <= 2.0) {
			ofRectangle boundsDoBetter = verdana14.getStringBoundingBox(sDoBetter, 0, 0);
			verdana14.drawString(sDoBetter, ofGetWindowWidth() * 0.5 - boundsDoBetter.width * 0.5, ofGetWindowHeight() * 0.6);
		}

		else if (mouthWidth >= 16.0 && mouthHeight >= 4.0) {
			ofRectangle boundsGreat = verdana14.getStringBoundingBox(sGreat, 0, 0);
			verdana14.drawString(sGreat, ofGetWindowWidth() * 0.5 - boundsGreat.width * 0.5, ofGetWindowHeight() * 0.6);

			// implementar TIMER aqui

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

		ofSetColor(255);
		ofDrawBitmapString("sending OSC to " + host + ":" + ofToString(port), 20, ofGetHeight() - 100);
		ofDrawBitmapString("Width: " + ofToString(mouthWidth), 20, ofGetHeight() - 80);
		ofDrawBitmapString("Height: " + ofToString(mouthHeight), 20, ofGetHeight() - 60);

		if (bDrawMesh) {
			ofSetLineWidth(1);
			ofPushView();
			ofTranslate(ofGetWindowWidth() * 0.5 - (vidGrabber.getWidth() * 0.5), ofGetWindowHeight() * 0.5 - (vidGrabber.getHeight() * 0.5));
			tracker.getImageMesh().drawWireframe();
			//tracker.getImageMesh().drawFaces();
			ofPopView();

		}
	}

	else {
		ofSetColor(255);
		ofDrawBitmapString("Searching for face...", 20, ofGetHeight() - 100);
	}

	if (bPaused) {
		ofSetColor(255);
		ofDrawBitmapString("Paused", 20, ofGetHeight() - 140);
	}	

	if (bGuiVisible) {
		gui.draw();
	}

	ofDrawBitmapString("Framerate: " + ofToString((int)ofGetFrameRate()), 20, ofGetHeight() - 40);
	ofDrawBitmapString("Press: ' ' to pause, 'g' to toggle gui, 'm' to toggle mesh, 'r' to reset tracker.", 20, ofGetHeight() - 20);

}


//--------------------------------------------------------------
void ofApp::keyPressed(int key){

	if(key == 's' || key == 'S'){
        vidGrabber.videoSettings();
    }

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

		case 't':
			bTextVisible = !bTextVisible;
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