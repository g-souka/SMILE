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

	// name the program window and set the VSync to true
	ofSetWindowTitle("SMILE :)");
	ofSetVerticalSync(true);

	// configure the first state of the captured photos booleans
	bHasPhotoNeutral = false;
	bHasPhotoIdeal = false;
	
	// print a list of avaiable input devices to the console
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

	// choose the device you'd like to use, the desired framerate and size to grab
	vidGrabber.setDeviceID(1);
    vidGrabber.setDesiredFrameRate(60);
    vidGrabber.initGrabber(camWidth, camHeight);

	// allocate memory for photos
	photoFrameNeutral.allocate(camWidth, camHeight, OF_PIXELS_RGB);	// relative to the ofPixels variable
	photoFrameNeutralTexture.allocate(photoFrameNeutral);			// relative to the ofTexture variable
	photoFrameIdeal.allocate(camWidth, camHeight, OF_PIXELS_RGB);	// the dimensions and pixel type must be specified
	photoFrameIdealTexture.allocate(photoFrameIdeal);				// the textures are relative to the pixels variables

	// face tracker variable configuration, the mouth values are extracted from these
	mouthWidth = tracker.getGesture(ofxFaceTracker::MOUTH_WIDTH);
	mouthHeight = tracker.getGesture(ofxFaceTracker::MOUTH_HEIGHT);

	// xml settings load
	loadSettings();

	// load images
	text01.load("images/text01.png");		// all of this images are external to the program
	text02.load("images/text02.png");		// they are stored in a root/data/images folder structure
	text03.load("images/text03.png");		// the root folder must be where the executable
	text04.load("images/text04.png");		// is also present so that the images are properly fetched
	text05.load("images/text05.png");
	text06.load("images/text06.png");
	text07.load("images/text07.png");
	text08.load("images/text08.png");

	textSmile.load("images/textSmile.png");
	textNatural.load("images/textNatural.png");
	textProjection.load("images/textProjection.png");
	textBoth.load("images/textBothAreYou.png");
	textHappiness.load("images/textHappiness.png");

	logoSmile.load("images/logoSmile.png");
	logoSmileInverted.load("images/logoSmileInverted.png");

	// moment start
	momentNum = 1;
	
	// timer configuration
	bTimerNeutralReached = false;
	bTimerIdealReached = false;
	bTimerEndReached = false;

	endTimeNeutralPhoto = 2000;		// these variables define, in miliseconds,
	endTimeIdealPhoto = 2000;		// how much time the program is supposed to wait
	endTimeFinalReset = 10000;		// until it triggers the programmed actions

	startTime = ofGetElapsedTimeMillis();

	// mouth value scaling
	pctWidth = ofMap(mouthWidth, 0.0, 20.0, 0.0, 1.0, false);	// the values of Mouth are scaled using a map function
	pctHeight = ofMap(mouthHeight, 0.0, 8.0, 0.0, 1.0, false);	// the mapping is done to 0. to 1. range
	pctMouth = pctWidth * 0.7 + pctHeight * 0.3;				// this allows for an easier manipulation of the representation of these values

	// text bool configuration
	bShowText = false;
    
    // backward bool configuration
    bAllowBackwards = false;
}


//--------------------------------------------------------------
void ofApp::update(){

	// in order to update in real time
	// some of the following variables must also be set in update()

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

	ofBackground(ofColor::white);	// configure the initial layout of the application
	ofSetColor(ofColor::white);		// here the background and text colors are defined
	ofSetLineWidth(5);				// set the line width to 5

	// some moments feature black backgrounds instead of white
	// as such, it is mandatory to change what LOGO image is displayed in the top right corner
	// since moment 6 and 7 have black backgrounds, the image that is draw must be white to be seen
	if (momentNum == 6 || momentNum == 7) {
		logoSmileInverted.draw(ofGetWindowWidth() - logoSmileInverted.getWidth() - 10, 10);
	}
	else {
		logoSmile.draw(ofGetWindowWidth() - logoSmile.getWidth() - 10, 10);
	}

	// moment 01
	// in this first moment the first text element is drawn
	// it is centered in the program window, this is done in a responsive way
	// if the window is scaled, the text element stays centered
	if (momentNum == 1) {
		text01.draw(windowCenterW - text01.getWidth() * 0.5, windowCenterH - text01.getHeight() * 0.5);
	}

	// moment 02
	// text element draw at center
	// timer is reset and starts as soon as moment 02 is entered
	// when the timer reaches the predetermined time a photo is secretly taken to the user
	// the photo is stored in memory and exported to an external file named "TimeStamp - photoFrameNeutral.jpg"
	// the bool for the photo is changed to true and the bool for the timer changes state
	if (momentNum == 2) {
		text02.draw(windowCenterW - text02.getWidth() * 0.5, windowCenterH - text02.getHeight() * 0.5);

		float timer = ofGetElapsedTimeMillis() - startTime;

		if (timer >= endTimeNeutralPhoto && !bTimerNeutralReached) {
			bTimerNeutralReached = true;
		}

		// get the percentage of the timer
		float pct = ofMap(timer, 0.0, endTimeNeutralPhoto, 0.0, 1.0, true);

		if (bShowText) {
			// some information about the timer
			string  info = "Start Time: " + ofToString(startTime, 1) + "\n";
			info += "End Time:   " + ofToString(endTimeNeutralPhoto / 1000.0, 1) + " seconds\n";
			info += "Timer:      " + ofToString(timer / 1000.0, 1) + " seconds\n";
			info += "Percentage: " + ofToString(pct * 100, 1) + "%\n";
			ofSetColor(0);
			ofDrawBitmapString(info, 20, 20);
		}

		if (bHasPhotoNeutral == false && bTimerNeutralReached) {
			// output a message to the console if the conditions are proven
			ofMessage msg("  Neutral Timer Reached\n  photoFrameNeutralTexture loaded\n  !bTimerNeutralReached");
			ofSendMessage(msg);

			// save the image to an external file
			ofPixels & pixels = vidGrabber.getPixels();
			for (int i = 0; i < pixels.size(); i++) {
				photoFrameNeutral[i] = pixels[i];
			}
			photoFrameNeutralTexture.loadData(photoFrameNeutral);
			ofSaveImage(photoFrameNeutral, ofToString(ofGetTimestampString() + " - photoFrameNeutral") + ".jpg");

			// timer and photo bool
			bHasPhotoNeutral = true;
			bTimerNeutralReached = !bTimerNeutralReached;
		}
	}

	// moment 03
	// display the text element at the center of the window
	if (momentNum == 3) { text03.draw(windowCenterW - text03.getWidth() * 0.5, windowCenterH - text03.getHeight() * 0.5); }

	// moment 04
	// display the text element at the center of the window
	if (momentNum == 4) { text04.draw(windowCenterW - text04.getWidth() * 0.5, windowCenterH - text04.getHeight() * 0.5); }

	// moment 05
	// display the text element at the center of the window
	if (momentNum == 5) { text05.draw(windowCenterW - text05.getWidth() * 0.5, windowCenterH - text04.getHeight() * 0.5); }

	// moment 06
	// this moment differentiates itself by having a black background, the logo is white also
	// text elements are no longer present and now the user must interact with the app through his smile
	// the camera input is shown at the center and below it there is a meter that represents the smile amount
	if (momentNum == 6) {
		ofBackground(ofColor::black);
		logoSmileInverted.draw(ofGetWindowWidth() - logoSmileInverted.getWidth() - 10, 10);

		// show the camera input
		vidGrabber.draw(windowCenterW - (camWidth * 0.5), windowCenterH - (camHeight * 0.5), camWidth, camHeight);

		// draw borders
        ofNoFill();
		ofSetLineWidth(5);
        // video borders
        ofDrawRectangle(windowCenterW - (camWidth * 0.5), windowCenterH - (camHeight * 0.5), camWidth, camHeight);
    }

	// if the face is found
	if (momentNum == 6 && tracker.getFound()) {
        
		// if the text is set to show, the screen will have information relating to different variables used for debugging
		if (bShowText) {
			ofDrawBitmapString("Start Time: " + ofToString(startTime, 1), 20, ofGetHeight() - 220);
			ofDrawBitmapString("End Time:   " + ofToString(endTimeIdealPhoto / 1000.0, 1) + " seconds", 20, ofGetHeight() - 200);

			ofDrawBitmapString("sending OSC to " + host + ":" + ofToString(port), 20, ofGetHeight() - 160);
			ofDrawBitmapString("Mouth Percentage: " + ofToString(pctMouth), 20, ofGetHeight() - 140);
			ofDrawBitmapString("Mouth Width: " + ofToString(mouthWidth), 20, ofGetHeight() - 120);
			ofDrawBitmapString("Mouth Height: " + ofToString(mouthHeight), 20, ofGetHeight() - 100);
		}

		// horizontal meter bar showing smile progress
		ofFill();
		ofSetColor(20, 20, 20);
		ofDrawRectangle(windowCenterW - (camWidth * 0.5), windowCenterH + (camHeight * 0.5) + 20, camWidth, 20);

		// percentage of bar width that indicates smile amount
		ofSetColor(ofColor::white, 200);
		ofDrawRectangle(windowCenterW - (camWidth * 0.5), windowCenterH + (camHeight * 0.5) + 20, pctMouth * camWidth, 20);

		// threshold to cross with smile
		// the line indicates the position of the threshold
		ofSetLineWidth(2);
		ofSetColor(ofColor::white);
		ofDrawLine(windowCenterW + (camWidth * 0.25), windowCenterH + (camHeight * 0.5) + 20, windowCenterW + (camWidth * 0.25), windowCenterH + (camHeight * 0.5) + 40);
		
		// SMILE text
		// the text below the bar must be correctly placed in order to understand where the threshold is
		ofSetColor(ofColor::white);
		textSmile.draw(windowCenterW + (camWidth * 0.25), windowCenterH + (camHeight * 0.5) + 50);

		// if face mesh is drawn
		if (bDrawMesh) {
			ofSetColor(255, 255, 255, 55 + 220 * alphaOscillation);
			ofSetLineWidth(1);

			// overlap the mesh with the camera display by translation
			// this must be done in order to have the mesh on top of the "real" face
			ofPushView();
			ofTranslate(ofGetWindowWidth() * 0.5 - (vidGrabber.getWidth() * 0.5), ofGetWindowHeight() * 0.5 - (vidGrabber.getHeight() * 0.5));
			tracker.getImageMesh().drawWireframe();
			//tracker.getImageMesh().drawFaces();
			ofPopView();
		}

		ofSetColor(ofColor::white);

		if (bHasPhotoIdeal == false) {
			if(pctMouth < 0.75) {	// this value is the threshold and indicates how much the people has to smile, range is [0, 1]
				startTime = ofGetElapsedTimeMillis();	// if the smile is below the threshold level, the timer keeps resetting (stays at zero)
			}
			else { // the moment the threshold is passed, start to count
				float timer = ofGetElapsedTimeMillis() - startTime;
				if (timer >= endTimeIdealPhoto && !bTimerIdealReached) {
					bTimerIdealReached = true; // if the condition is held for sufficient time, set the timer bool to true
				}

				// get the percentage of the timer, used for debug only
				float pct = ofMap(timer, 0.0, endTimeIdealPhoto, 0.0, 1.0, true);

				if (bShowText) {
					// some information about the timer
					string  info = "Start Time: " + ofToString(startTime, 1) + "\n";
					info += "End Time:   " + ofToString(endTimeIdealPhoto / 1000.0, 1) + " seconds\n";
					info += "Timer:      " + ofToString(timer / 1000.0, 1) + " seconds\n";
					info += "Percentage: " + ofToString(pct * 100, 1) + "%\n";
					ofSetColor(ofColor::deepPink);
					ofDrawBitmapString(info, 20, 20);
				}

				if (bTimerIdealReached) {	// when the timer is reached, take the Idealized photo and proceed to the next moment
					// output to the console that the timer, photo and the bool have been taken/changed
					ofMessage msg("  Ideal Timer Reached\n  photoFrameIdealTexture loaded\n  !bTimerIdealReached");
					ofSendMessage(msg);

					// generate the photo and save it to an external file
					ofPixels & pixels = vidGrabber.getPixels();
					for (int i = 0; i < pixels.size(); i++) {
						photoFrameIdeal[i] = pixels[i];
					}
					photoFrameIdealTexture.loadData(photoFrameIdeal);
					ofSaveImage(photoFrameIdeal, ofToString(ofGetTimestampString() + " - photoFrameIdeal") + ".jpg");

					// set the photo boolean to true
					bHasPhotoIdeal = true;
					// change the timer bool state
					bTimerIdealReached = !bTimerIdealReached;

					// jump automatically to moment 07 when the picture is taken
					momentNum = 7;
				}
			}
		}
	}

	// if no face is found, output text that a face is being searched for, used for debugging purposes
	if (momentNum == 6 && tracker.getFound() == false && bShowText) {
		ofDrawBitmapString("Searching for face...", 20, ofGetHeight() - 140);
	}

	// moment 07
	// show the Neutral photo at the center of the screen
	// below the photo place the text that asks the user what is happiness
	// contrary to the expectations the photo shown here is the neutral
	// and not the ideal one that was taken the moment before
	if (momentNum == 7) {
		ofBackground(ofColor::black);
		ofSetColor(ofColor::white);

		logoSmileInverted.draw(ofGetWindowWidth() - logoSmileInverted.getWidth() - 10, 10);

		// draw photo textures
		photoFrameNeutralTexture.draw(windowCenterW - (camWidth * 0.5), windowCenterH - (camHeight * 0.5), camWidth, camHeight);

		// WHAT IS HAPPINESS text
		textHappiness.draw(windowCenterW - (textHappiness.getWidth() * 0.5), windowCenterH + (camHeight * 0.5) + 20);
		
		// draw borders
		ofNoFill();
		// video borders
		ofDrawRectangle(windowCenterW - (camWidth * 0.5), windowCenterH - (camHeight * 0.5), camWidth, camHeight);
		// text borders
		ofDrawRectangle(windowCenterW - (camWidth * 0.5), windowCenterH + (camHeight * 0.5), camWidth, textHappiness.getHeight() + 40);
	}

	// moment 08
	// display both the photos side-by-side in order to compare them
	// label the left one as the neutral/natural
	// the right one as the idealized/projected
	if (momentNum == 8) {
		ofBackground(ofColor::black);
		ofSetColor(ofColor::white);

		logoSmileInverted.draw(ofGetWindowWidth() - logoSmileInverted.getWidth() - 10, 10);

		// draw photo textures
		photoFrameNeutralTexture.draw(windowCenterW - camWidth, windowCenterH - (camHeight * 0.5), camWidth, camHeight);
		photoFrameIdealTexture.draw(windowCenterW, windowCenterH - (camHeight * 0.5), camWidth, camHeight);

		// Natural | Projected text
		textNatural.draw(windowCenterW - (camWidth * 0.5) - (textNatural.getWidth() * 0.5), windowCenterH + (camHeight * 0.5) + 20);
		textProjection.draw(windowCenterW + (camWidth * 0.5) - (textProjection.getWidth() * 0.5), windowCenterH + (camHeight * 0.5) + 20);

		// draw borders
		ofNoFill();
		// video borders
		ofDrawRectangle(windowCenterW - camWidth, windowCenterH - (camHeight * 0.5), camWidth * 2, camHeight);
		// text borders
		ofDrawRectangle(windowCenterW - camWidth, windowCenterH + (camHeight * 0.5), camWidth * 2, textProjection.getHeight() + 40);
		// vertical line
		ofDrawLine(windowCenterW, windowCenterH - (camHeight * 0.5), windowCenterW, windowCenterH + (camHeight * 0.5) + textProjection.getHeight() + 40);
	}

	// moment 09
	// keep showing the two photos
	// change the text to a message about both versions of the subject being a part of who you are
	if (momentNum == 9) {
		ofBackground(ofColor::black);
		ofSetColor(ofColor::white);

		logoSmileInverted.draw(ofGetWindowWidth() - logoSmileInverted.getWidth() - 10, 10);

		// draw photo textures
		photoFrameNeutralTexture.draw(windowCenterW - camWidth, windowCenterH - (camHeight * 0.5), camWidth, camHeight);
		photoFrameIdealTexture.draw(windowCenterW, windowCenterH - (camHeight * 0.5), camWidth, camHeight);

		// But Both Are You text
		textBoth.draw(windowCenterW - (textBoth.getWidth() * 0.5), windowCenterH + (camHeight * 0.5) + 20);

		// draw borders
		ofNoFill();
		//video borders
		ofDrawRectangle(windowCenterW - camWidth, windowCenterH - (camHeight * 0.5), camWidth, camHeight);
		ofDrawRectangle(windowCenterW, windowCenterH - (camHeight * 0.5), camWidth, camHeight);
		// text borders
		ofDrawRectangle(windowCenterW - camWidth, windowCenterH + (camHeight * 0.5), camWidth * 2, textBoth.getHeight() + 40);
	}

	// moment 10
	// display the text element at the center of the window
	if (momentNum == 10) {
		text06.draw(windowCenterW - text06.getWidth() * 0.5, windowCenterH - text06.getHeight() * 0.5);
	}
	
	// moment 11
	// display the text element at the center of the window
	if (momentNum == 11) {
		text07.draw(windowCenterW - text07.getWidth() * 0.5, windowCenterH - text07.getHeight() * 0.5);
	}

	// moment 12
	// display the text element at the center of the window
	// start timer when moment is entered that automatically resets the app
	// this reset allows the app to be autonomous and restart from the beggining
	// for every new user that interacts with the installation
	if (momentNum == 12) {
		text08.draw(windowCenterW - text08.getWidth() * 0.5, windowCenterH - text08.getHeight() * 0.5);

		// initiate timer
		float timer = ofGetElapsedTimeMillis() - startTime;
		if (timer >= endTimeFinalReset && bTimerEndReached == false) {
			bTimerEndReached = true;

			// get the percentage of the timer, used for debbuging
			float pct = ofMap(timer, 0.0, endTimeFinalReset, 0.0, 1.0, true);

			if (bShowText) {
				// some information about the timer
				string  info = "Start Time: " + ofToString(startTime, 1) + "\n";
				info += "End Time:   " + ofToString(endTimeFinalReset / 1000.0, 1) + " seconds\n";
				info += "Timer:      " + ofToString(timer / 1000.0, 1) + " seconds\n";
				info += "Percentage: " + ofToString(pct * 100, 1) + "%\n";
				ofSetColor(ofColor::hotPink);
				ofDrawBitmapString(info, 20, 20);
			}

			// output to the console messages about the state changes and timer
			ofMessage msg("  End Timer Reached\n  bHasPhotoNeutral = false\n  bHasPhotoIdeal = false");
			ofSendMessage(msg);

			// set the photo bools to false so that the next user can take new pictures
			bHasPhotoNeutral = false;
			bHasPhotoIdeal = false;
			// set the moment to the first one, so that it starts ate the beggining
			momentNum = 1;
			// change the state of the timer bool so that it works every time a new user is interacting
			bTimerEndReached = !bTimerEndReached;
		}
	}

	// camera is paused, used for debugging
	if (bPaused && bShowText) {
		ofSetColor(0);
		ofDrawBitmapString("Paused", 20, ofGetHeight() - 160);
	}	

	// draw GUI
	if (bGuiVisible) {
		gui.draw();
	}

	// show the text
	if (bShowText) {
		ofSetColor(ofColor::hotPink);
		ofDrawBitmapString("alphaOscillation " + ofToString(alphaOscillation), 20, ofGetHeight() - 80);
		ofDrawBitmapString("momentNum " + ofToString(momentNum), 20, ofGetHeight() - 60);
		ofDrawBitmapString("Framerate: " + ofToString((int)ofGetFrameRate()), 20, ofGetHeight() - 40);
		ofDrawBitmapString("Press: 'p' to pause, 'g' to toggle gui, 'm' to toggle mesh, 'r' to reset tracker, 'b' to allow backward movement.", 20, ofGetHeight() - 20);
	}
}

//--------------------------------------------------------------
void ofApp::setVideoSource(bool useCamera) {

	// this void element was imported from the ofxFaceTracking addon
	// it checks if a camera input is to be used
	// and attributes several values to the variables used by the tracker

	bUseCamera = useCamera;

	if (bUseCamera) {
		videoSource = &vidGrabber;
		sourceWidth = camWidth;
		sourceHeight = camHeight;
	}
}

//--------------------------------------------------------------
void ofApp::keyPressed(int key){

	if(key == 's' || key == 'S'){	// opens a separate window that allows the camera to be controlled
        vidGrabber.videoSettings();	// this allows to manipulate some aspects like brightness, gain, white balance, etc.
    }								// every camera allows different types of control

	switch (key) {
	
		case 't':	// toggle the display of the text
			bShowText = !bShowText;
			break;

		case 'r':	// resets the face tracker 
			tracker.reset();
			break;

		case 'm':	// toggles the face mesh drawing
			bDrawMesh = !bDrawMesh;
			break;

		case 'p':	// pauses/unpauses the video input
			bPaused = !bPaused;
			break;

		case 'g':	// toggles the GUI
			bGuiVisible = !bGuiVisible;
			break;
            
        case 'b':	// toggles the capability to move back in the moments with the RMB
            bAllowBackwards = !bAllowBackwards;
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

    if(bAllowBackwards) { // if backward movement is not allowed the RMB does nothing
        if (button == 2 && momentNum > 1) momentNum--;
        else if (button == 0 && momentNum < 12)	momentNum++;
    }
    else { // in moment 6, the only way to move is by using your smile, LMB does not work
        if (button == 0 && momentNum < 12 && momentNum != 6) momentNum++;
    }

	// in the following moment variables the timers are reset
	// so that the counting is made from 0 to the defined time
	if (momentNum == 2) {
		startTime = ofGetElapsedTimeMillis();
	}

	if (momentNum == 6) {
		startTime = ofGetElapsedTimeMillis();
	}

	if (momentNum == 12) {
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
	// receive the messages and output them to the console
	cout << "Got the message: \n" + msg.message << endl;
}

/*
//--------------------------------------------------------------
void ofApp::dragEvent(ofDragInfo dragInfo){
}
*/
