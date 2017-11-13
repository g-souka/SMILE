#include "ofMain.h"
#include "ofApp.h"

#define GLUTTY
#ifdef GLUTTY
    #include "ofAppGlutWindow.h"
#endif

//========================================================================
int main( ){
    
    #ifdef GLUTTY
        ofAppGlutWindow window;
        ofSetupOpenGL(&window, 1280,580, OF_WINDOW);
        // this kicks off the running of my app
        // can be OF_WINDOW or OF_FULLSCREEN
        // pass in width and height too:
    
    #else
        ofSetupOpenGL(1280,580, OF_WINDOW);
    #endif
        ofRunApp(new ofApp());
    
}
