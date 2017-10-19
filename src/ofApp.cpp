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
	bGuiVisible = true;
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
	ofSetWindowTitle("Face Tracking With Smile Photo Capture");	
	ofSetVerticalSync(true);

	bHasPhoto = false;
	
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

	photoFrame.allocate(camWidth, camHeight, OF_PIXELS_RGB);
	photoFrameTexture.allocate(photoFrame);

	loadSettings();

	mouthWidth = tracker.getGesture(ofxFaceTracker::MOUTH_WIDTH);
	mouthHeight = tracker.getGesture(ofxFaceTracker::MOUTH_HEIGHT);
	
}


//--------------------------------------------------------------
void ofApp::update(){

	// esta variavel tem que ser definida no update
	// provavelmente por estar a ser enviada por OSC em tempo real
	// no setup não funciona
	mouthWidth = tracker.getGesture(ofxFaceTracker::MOUTH_WIDTH);
	mouthHeight = tracker.getGesture(ofxFaceTracker::MOUTH_HEIGHT);

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

	ofBackground(ofColor::teal);

    vidGrabber.draw(0, 0);
	photoFrameTexture.draw(camWidth, 0, camWidth, camHeight);

	if (tracker.getFound()) {

		if (mouthWidth >= 16.0 && mouthHeight >= 6.0) {
			ofSetColor(ofColor::gold);
			ofDrawEllipse(camWidth * 0.5, 40 + camHeight, mouthWidth * 10, mouthHeight * 10);

			if (bHasPhoto == false) {
				ofPixels & pixels = vidGrabber.getPixels();
				for (int i = 0; i < pixels.size(); i++) {
					photoFrame[i] = pixels[i];
				}
				photoFrameTexture.loadData(photoFrame);
				ofSaveImage(photoFrame, ofToString("photoFrame-" + ofGetTimestampString()) + ".jpg");

				bHasPhoto = true;
				cout << bHasPhoto;
			}
		}
		else {
			ofSetColor(ofColor::darkOrchid);
			ofDrawEllipse(camWidth * 0.5, 40 + camHeight, mouthWidth * 10, mouthHeight * 10);
		}

		ofSetColor(255);
		ofDrawBitmapString("sending OSC to " + host + ":" + ofToString(port), 20, ofGetHeight() - 100);
		ofDrawBitmapString("Width: " + ofToString(mouthWidth), 20, ofGetHeight() - 80);
		ofDrawBitmapString("Height: " + ofToString(mouthHeight), 20, ofGetHeight() - 60);

		//	Valores de threshold Guilherme no comboio
		//	W: maior que 18
		//	H: maior que 6

		if (bDrawMesh) {
			ofSetLineWidth(1);
			tracker.getImageMesh().drawWireframe();

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
	ofDrawBitmapString("Press: space to pause, 'g' to toggle gui, 'm' to toggle mesh, 'r' to reset tracker.", 20, ofGetHeight() - 20);

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