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
	ofSetWindowTitle("SMILE :)");
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

	// string configuration
	sBestSmile = "SMILE!";

	// xml settings load
	loadSettings();

	// load images
	text01.load("images/text01.png");
	text02.load("images/text02.png");
	text03.load("images/text03.png");
	text04.load("images/text04.png");
	text05.load("images/text05.png");
	text06.load("images/text06.png");
	text07.load("images/text07.png");
	text08.load("images/text08.png");
	logoSmile.load("images/logoSmile.png");
	logoSmileInverted.load("images/logoSmileInverted.png");
	momentNum = 1;
	
	// timer configuration
	bTimerNeutralReached = false;
	bTimerIdealReached = false;
	bTimerEndReached = false;

	endTimeNeutralPhoto = 3000;
	endTimeIdealPhoto = 5000;
	endTimeFinalReset = 3000;

	startTime = ofGetElapsedTimeMillis();

	// mouth value scaling
	pctWidth = ofMap(mouthWidth, 0.0, 20.0, 0.0, 1.0, false);
	pctHeight = ofMap(mouthHeight, 0.0, 8.0, 0.0, 1.0, false);
	pctMouth = pctWidth * 0.7 + pctHeight * 0.3;

	// text bool configuration
	bShowText = true;
}


//--------------------------------------------------------------
void ofApp::update(){

	// in order to update in real time
	// the following variables must also be set in update()

	// update mouth values
	mouthWidth = tracker.getGesture(ofxFaceTracker::MOUTH_WIDTH);
	mouthHeight = tracker.getGesture(ofxFaceTracker::MOUTH_HEIGHT);

	// update window size variable
	windowCenterW = ofGetWindowWidth() * 0.5;
	windowCenterH = ofGetWindowHeight() * 0.5;

	if (bPaused)
		return;

	// update face tracker
	videoSource->update();
	if (videoSource->isFrameNew()) {
		tracker.update(toCv(*videoSource));
		sendFaceOsc(tracker);
	}

	// update the camera input
	vidGrabber.update();

	// update the timer and alpha oscillation
	t = ofGetElapsedTimef();
	alphaOscillation = (sin(t * 0.2 * TWO_PI) * 0.5 + 0.5);

	// update mouth scaling values
	pctWidth = ofMap(mouthWidth, 0.0, 20.0, 0.0, 1.0, false);
	pctHeight = ofMap(mouthHeight, 0.0, 8.0, 0.0, 1.0, false);
	pctMouth = pctWidth * 0.7 + pctHeight * 0.3;
}

//--------------------------------------------------------------
void ofApp::draw(){

	ofBackground(ofColor::white);

	if (momentNum == 6 && momentNum == 7) {
		logoSmileInverted.draw(ofGetWindowWidth() - logoSmileInverted.getWidth() - 10, 10);
	}
	else {
		logoSmile.draw(ofGetWindowWidth() - logoSmile.getWidth() - 10, 10);
	}

	// moment 01
	if (momentNum == 1) {
		text01.draw(windowCenterW - text01.getWidth() * 0.5, windowCenterH - text01.getHeight() * 0.5);
	}

	// moment 02
	if (momentNum == 2) {
		text02.draw(windowCenterW - text02.getWidth() * 0.5, windowCenterH - text02.getHeight() * 0.5);

		float timer = ofGetElapsedTimeMillis() - startTime;

		if (timer >= endTimeNeutralPhoto && !bTimerNeutralReached) {
			bTimerNeutralReached = true;
		}

		// get the percentage of the timer
		float pct = ofMap(timer, 0.0, endTimeNeutralPhoto, 0.0, 1.0, true);

		// some information about the timer
		string  info = "Start Time: " + ofToString(startTime, 1) + "\n";
				info += "End Time:   " + ofToString(endTimeNeutralPhoto / 1000.0, 1) + " seconds\n";
				info += "Timer:      " + ofToString(timer / 1000.0, 1) + " seconds\n";
				info += "Percentage: " + ofToString(pct * 100, 1) + "%\n";
		ofSetColor(0);
		ofDrawBitmapString(info, 20, 20);
				
		if (bHasPhotoNeutral == false && bTimerNeutralReached) {
			ofMessage msg("  Neutral Timer Reached\n  photoFrameNeutralTexture loaded");
			ofSendMessage(msg);

			ofPixels & pixels = vidGrabber.getPixels();
			for (int i = 0; i < pixels.size(); i++) {
				photoFrameNeutral[i] = pixels[i];
			}
			photoFrameNeutralTexture.loadData(photoFrameNeutral);
			ofSaveImage(photoFrameNeutral, ofToString(ofGetTimestampString() + " - photoFrameNeutral") + ".jpg");

			bHasPhotoNeutral = true;
		}
	}

	// moment 03
	if (momentNum == 3) { text03.draw(windowCenterW - text03.getWidth() * 0.5, windowCenterH - text03.getHeight() * 0.5); }

	// moment 04
	if (momentNum == 4) { text04.draw(windowCenterW - text04.getWidth() * 0.5, windowCenterH - text04.getHeight() * 0.5); }

	// moment 05
	if (momentNum == 5) { text05.draw(windowCenterW - text05.getWidth() * 0.5, windowCenterH - text04.getHeight() * 0.5); }

	// moment 06
	if (momentNum == 6) {
		ofBackground(ofColor::black);

		logoSmileInverted.draw(ofGetWindowWidth() - logoSmileInverted.getWidth() - 10, 10);

		// show the camera input
		vidGrabber.draw(windowCenterW - (camWidth * 0.5), windowCenterH - (camHeight * 0.5), camWidth, camHeight);

		// draw borders
		ofSetLineWidth(3);

		// ofPolyline to draw a video border
		vector<ofPoint> pts;

			pts.push_back(ofPoint (windowCenterW - (camWidth * 0.5) + (camWidth * 0.06), windowCenterH - (camHeight * 0.5) + (camHeight * 0.06)) );
			pts.push_back(ofPoint (windowCenterW + (camWidth * 0.5) - (camWidth * 0.06), windowCenterH - (camHeight * 0.5) + (camHeight * 0.06)) );
			pts.push_back(ofPoint (windowCenterW + (camWidth * 0.5) - (camWidth * 0.06), windowCenterH + (camHeight * 0.5) - (camHeight * 0.06)) );
			pts.push_back(ofPoint (windowCenterW - (camWidth * 0.5) + (camWidth * 0.06), windowCenterH + (camHeight * 0.5) - (camHeight * 0.06)) );
			pts.push_back(ofPoint (windowCenterW - (camWidth * 0.5) + (camWidth * 0.06), windowCenterH - (camHeight * 0.5) + (camHeight * 0.06)) );
			
		ofPolyline videoBorder(pts);
		videoBorder.draw();
	}

	// found face
	if (momentNum == 6 && tracker.getFound()) {

		if (bShowText) {
			ofDrawBitmapString("Start Time: " + ofToString(startTime, 1), 20, ofGetHeight() - 220);
			ofDrawBitmapString("End Time:   " + ofToString(endTimeIdealPhoto / 1000.0, 1) + " seconds", 20, ofGetHeight() - 200);

			ofDrawBitmapString("sending OSC to " + host + ":" + ofToString(port), 20, ofGetHeight() - 160);
			ofDrawBitmapString("Mouth Percentage: " + ofToString(pctMouth), 20, ofGetHeight() - 140);
			ofDrawBitmapString("Mouth Width: " + ofToString(mouthWidth), 20, ofGetHeight() - 120);
			ofDrawBitmapString("Mouth Height: " + ofToString(mouthHeight), 20, ofGetHeight() - 100);
		}

		// horizontal bar showing smile progress
		ofFill();
		ofSetColor(100);
		ofDrawRectangle(windowCenterW - (camWidth * 0.5), windowCenterH + (camHeight * 0.5) + 20, camWidth, 20);

		// percentage of bar width that indicates smile amount
		ofSetColor(ofColor::deepPink, 200);
		ofDrawRectangle(windowCenterW - (camWidth * 0.5), windowCenterH + (camHeight * 0.5) + 20, pctMouth * camWidth, 20);

		// threshold to cross with smile
		ofSetLineWidth(3);
		ofSetColor(255, 255, 255, 128);
		ofDrawLine(windowCenterW + (camWidth * 0.25), windowCenterH + (camHeight * 0.5) + 20, windowCenterW + (camWidth * 0.25), windowCenterH + (camHeight * 0.5) + 40);
		
		// GIVE US YOUR BEST SMILE text
		ofSetColor(ofColor::white);
		ofRectangle boundsBestSmile = verdana14.getStringBoundingBox(sBestSmile, 0, 0);
		verdana14.drawString(sBestSmile, windowCenterW + (camWidth * 0.25), windowCenterH + (camHeight * 0.5) + 60);

		// if face mesh is drawn
		if (bDrawMesh) {
			ofSetColor(255, 255, 255, 55 + 200 * alphaOscillation);
			ofSetLineWidth(1);

			// overlap the mesh with the camera display
			ofPushView();
			ofTranslate(ofGetWindowWidth() * 0.5 - (vidGrabber.getWidth() * 0.5), ofGetWindowHeight() * 0.5 - (vidGrabber.getHeight() * 0.5));
			tracker.getImageMesh().drawWireframe();
			//tracker.getImageMesh().drawFaces();
			ofPopView();
		}

		if (bHasPhotoIdeal == false) {
			if(pctMouth < 0.75) {
				startTime = ofGetElapsedTimeMillis();
			}
			else {
				float timer = ofGetElapsedTimeMillis() - startTime;
				if (timer >= endTimeIdealPhoto && !bTimerIdealReached) {
					bTimerIdealReached = true;
				}

				// get the percentage of the timer
				float pct = ofMap(timer, 0.0, endTimeIdealPhoto, 0.0, 1.0, true);

				// some information about the timer
				string  info = "Start Time: " + ofToString(startTime, 1) + "\n";
						info += "End Time:   " + ofToString(endTimeIdealPhoto / 1000.0, 1) + " seconds\n";
						info += "Timer:      " + ofToString(timer / 1000.0, 1) + " seconds\n";
						info += "Percentage: " + ofToString(pct * 100, 1) + "%\n";
				ofSetColor(ofColor::hotPink);
				ofDrawBitmapString(info, 20, 20);

				if (bTimerIdealReached) {
					ofMessage msg("  Ideal Timer Reached\n  photoFrameIdealTexture loaded");
					ofSendMessage(msg);

					ofPixels & pixels = vidGrabber.getPixels();
					for (int i = 0; i < pixels.size(); i++) {
						photoFrameIdeal[i] = pixels[i];
					}
					photoFrameIdealTexture.loadData(photoFrameIdeal);
					ofSaveImage(photoFrameIdeal, ofToString(ofGetTimestampString() + " - photoFrameIdeal") + ".jpg");

					bHasPhotoIdeal = true;
				}
			}
		}
	}

	if (momentNum == 6 && tracker.getFound() == false) {
		ofDrawBitmapString("Searching for face...", 20, ofGetHeight() - 140);
	}

	// moment 07
	if (momentNum == 7) {
		ofBackground(ofColor::black);
		ofSetColor(ofColor::white);

		logoSmileInverted.draw(ofGetWindowWidth() - logoSmileInverted.getWidth() - 10, 10);

		// draw photo textures
		photoFrameNeutralTexture.draw(windowCenterW - camWidth, windowCenterH - (camHeight * 0.5), camWidth, camHeight);
		photoFrameIdealTexture.draw(windowCenterW, windowCenterH - (camHeight * 0.5), camWidth, camHeight);

		// draw borders
		ofNoFill();
		ofSetLineWidth(20);
		ofDrawRectangle(windowCenterW - camWidth, windowCenterH - (camHeight * 0.5), camWidth, camHeight);
		ofDrawRectangle(windowCenterW, windowCenterH - (camHeight * 0.5), camWidth, camHeight);
	}

	// moment 08
	if (momentNum == 8) {
		text06.draw(windowCenterW - text06.getWidth() * 0.5, windowCenterH - text06.getHeight() * 0.5);
	}
	
	// moment 09
	if (momentNum == 9) {
		text07.draw(windowCenterW - text07.getWidth() * 0.5, windowCenterH - text07.getHeight() * 0.5);
	}

	// moment 10
	if (momentNum == 10) {
		text08.draw(windowCenterW - text08.getWidth() * 0.5, windowCenterH - text08.getHeight() * 0.5);

		float timer = ofGetElapsedTimeMillis() - startTime;
		if (timer >= endTimeFinalReset && !bTimerEndReached) {
			bTimerEndReached = true;

			// get the percentage of the timer
			float pct = ofMap(timer, 0.0, endTimeFinalReset, 0.0, 1.0, true);

			// some information about the timer
			string  info = "Start Time: " + ofToString(startTime, 1) + "\n";
			info += "End Time:   " + ofToString(endTimeFinalReset / 1000.0, 1) + " seconds\n";
			info += "Timer:      " + ofToString(timer / 1000.0, 1) + " seconds\n";
			info += "Percentage: " + ofToString(pct * 100, 1) + "%\n";
			ofSetColor(0);
			ofDrawBitmapString(info, 20, 20);
		
			ofMessage msg("  End Timer Reached");
			ofSendMessage(msg);

			bHasPhotoNeutral = false;
			bHasPhotoIdeal = false;
			momentNum = 1;
			bTimerEndReached = !bTimerEndReached;
		}
	}

	// camera is paused
	if (bPaused) {
		ofSetColor(0);
		ofDrawBitmapString("Paused", 20, ofGetHeight() - 160);
	}	

	// draw GUI
	if (bGuiVisible) {
		gui.draw();
	}

	ofSetColor(ofColor::hotPink);

	if (bShowText) {
		ofDrawBitmapString("alphaOscillation " + ofToString(alphaOscillation), 20, ofGetHeight() - 80);
		ofDrawBitmapString("momentNum " + ofToString(momentNum), 20, ofGetHeight() - 60);
		ofDrawBitmapString("Framerate: " + ofToString((int)ofGetFrameRate()), 20, ofGetHeight() - 40);
		ofDrawBitmapString("Press: 'p' to pause, 'g' to toggle gui, 'm' to toggle mesh, 'r' to reset tracker.", 20, ofGetHeight() - 20);
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

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

	if(key == 's' || key == 'S'){
        vidGrabber.videoSettings();
    }

	switch (key) {
	
		case 't':
			bShowText = !bShowText;
			break;

		case 'r':
			tracker.reset();
			break;

		case 'm':
			bDrawMesh = !bDrawMesh;
			break;

		case 'p':
			bPaused = !bPaused;
			break;

		case 'g':
			bGuiVisible = !bGuiVisible;
			break;
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
*/

//--------------------------------------------------------------
void ofApp::mousePressed(int x, int y, int button){

	if (button == 2 && momentNum > 1)		momentNum--;
	else if (button == 0 && momentNum < 10)	momentNum++;

	if (momentNum == 2) {
		startTime = ofGetElapsedTimeMillis();
	}

	if (momentNum == 6) {
		startTime = ofGetElapsedTimeMillis();
	}

	if (momentNum == 10) {
		startTime = ofGetElapsedTimeMillis();
	}
}

/*
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
*/

//--------------------------------------------------------------
void ofApp::gotMessage(ofMessage msg){
	cout << "Got the message: \n" + msg.message << endl;
}

/*
//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
}
*/
