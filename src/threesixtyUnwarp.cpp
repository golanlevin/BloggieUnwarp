#include "threesixtyUnwarp.h"


//--------------------------------------------------------------
void threesixtyUnwarp::setup(){
	
	//---------------------------
	// app properties
	ofSetVerticalSync(false);
	bMousePressed   = false;
	bCenterChanged  = false;
	bPlayerPaused   = false;
	bAngularOffsetChanged = false;
	bMousePressedInPlayer = false;
	bMousepressedInUnwarped = false;
	bSavingOutVideo = false;
	bSaveAudioToo   = false;
	nWrittenFrames  = 0;
	handyString = new char[128];
	outputFileName = "output.mov";

	//---------------------------
	// Load settings file
	if( XML.loadFile("UnwarperSettings.xml") ){
		printf("UnwarperSettings.xml loaded!\n");
	} else{
		printf("Unable to load UnwarperSettings.xml!\nPlease check 'data' folder.\n");
	}
	
	maxR_factor   = XML.getValue("MAXR_FACTOR", 0.96);
	minR_factor   = XML.getValue("MINR_FACTOR", 0.16);
	angularOffset = XML.getValue("ROTATION_DEGREES", 0.0);
	
	int loadedQuality  = XML.getValue("CODEC_QUALITY", 3);
	loadedQuality = MIN(5, MAX(0, loadedQuality));
	int codecQualities[] = {
		OF_QT_SAVER_CODEC_QUALITY_MIN,
		OF_QT_SAVER_CODEC_QUALITY_LOW,
		OF_QT_SAVER_CODEC_QUALITY_NORMAL,
		OF_QT_SAVER_CODEC_QUALITY_HIGH,
		OF_QT_SAVER_CODEC_QUALITY_MAX,             
		OF_QT_SAVER_CODEC_QUALITY_LOSSLESS
	};
	codecQuality = codecQualities[loadedQuality];
	
	player.loadMovie(XML.getValue("INPUT_FILENAME", "input.mov"));
	player.getDuration();
	
	unwarpedW = (int) XML.getValue("OUTPUT_W", 1280);
	unwarpedH = (int) XML.getValue("OUTPUT_H", 256);
	
	//if the XML element doesn't exist, create it.
	XML.setValue("OUTPUT_W", (int) unwarpedW);
	XML.setValue("OUTPUT_H", (int) unwarpedH);
	
	
	// Interpolation method: 
	// 0 = CV_INTER_NN, 1 = CV_INTER_LINEAR, 2 = CV_INTER_CUBIC.
	interpMethod = (int) XML.getValue("INTERP_METHOD", 1); 
	XML.setValue("INTERP_METHOD", (int) interpMethod);
	
	int bSaveAud = (int) XML.getValue("INCLUDE_AUDIO", 0); 
	bSaveAudioToo = (bSaveAud != 0);
	
	/*
	// straight rectilinearization
	yWarpA = -0.2047;
	yWarpB =  0.8632;
	yWarpC =  0.3578;
	yWarpA = XML.getValue("R_WARP_A", -0.2047);
	yWarpB = XML.getValue("R_WARP_B",  0.8632);
	yWarpC = XML.getValue("R_WARP_C",  0.3578);
	 */

	yWarpA =   0.1850;
	yWarpB =   0.8184;
	yWarpC =  -0.0028;
	yWarpA = XML.getValue("R_WARP_A",  0.1850);
	yWarpB = XML.getValue("R_WARP_B",  0.8184);
	yWarpC = XML.getValue("R_WARP_C", -0.0028);


	//======================================
	// create data structures for unwarping
	blackOpenCV = cvScalarAll(0);
	
	// The "warped" original source video produced by the Bloggie.
	warpedW = player.width;
	warpedH = player.height;
	int nWarpedBytes = warpedW * warpedH * 3;
	printf("warpedW = %d, warpedH = %d\n", warpedW, warpedH);
	
	warpedImageOpenCV.allocate(warpedW, warpedH);
	warpedPixels = new unsigned char[nWarpedBytes];	
	warpedIplImage = warpedImageOpenCV.getCvImage();
	cvSetImageROI(warpedIplImage, cvRect(0, 0, warpedW, warpedH));
	
	int nUnwarpedPixels = unwarpedW * unwarpedH;
	int nUnwarpedBytes  = unwarpedW * unwarpedH * 3;
	unwarpedImage.allocate(unwarpedW, unwarpedH, OF_IMAGE_COLOR);
	unwarpedPixels = new unsigned char[nUnwarpedBytes];
	unwarpedTexture.allocate(unwarpedW, unwarpedH,GL_RGB);
	
	unwarpedImageOpenCV.allocate(unwarpedW, unwarpedH);
	unwarpedImageOpenCV.setROI(0, 0, unwarpedW, unwarpedH);
	unwarpedIplImage = unwarpedImageOpenCV.getCvImage();
	
	srcxArrayOpenCV.allocate(unwarpedW, unwarpedH);
	srcyArrayOpenCV.allocate(unwarpedW, unwarpedH);
	srcxArrayOpenCV.setROI(0, 0, unwarpedW, unwarpedH);
	srcyArrayOpenCV.setROI(0, 0, unwarpedW, unwarpedH);
	
	xocvdata = (float*) srcxArrayOpenCV.getCvImage()->imageData; 
	yocvdata = (float*) srcyArrayOpenCV.getCvImage()->imageData; 
	
	playerScaleFactor = (float)(ofGetHeight() - unwarpedH)/(float)warpedH;
	savedWarpedCx = warpedCx = XML.getValue("CENTERX", warpedW / 2.0);
	savedWarpedCy = warpedCy = XML.getValue("CENTERY", warpedH / 2.0);
	savedAngularOffset = angularOffset;
	
	//if the XML element doesn't exist, create it.
	XML.setValue("CENTERX", warpedCx);
	XML.setValue("CENTERY", warpedCy);
	XML.setValue("ROTATION_DEGREES", angularOffset);
	
	
	//---------------------------
	// cylinder vizualization properties
	cylinderRes = 90;
	cylinderWedgeAngle = 360.0 / (cylinderRes-1);
	cylinderX = new float[cylinderRes];
	cylinderY = new float[cylinderRes];
	for (int i = 0; i < cylinderRes; i++) {
		cylinderX[i] = cos(ofDegToRad((float)i * cylinderWedgeAngle));
		cylinderY[i] = sin(ofDegToRad((float)i * cylinderWedgeAngle));
	}
	blurredMouseY = 0;
	blurredMouseX = 0;
	
	videoRecorder = new ofxQtVideoSaver();
	currentCodecId = 16;
	videoRecorder->setCodecType (currentCodecId);
	videoRecorder->setCodecQualityLevel (codecQuality); 
	

	//---------------------------
	// start it up
	computePanoramaProperties();
	computeInversePolarTransform(); 
	player.play();
	
	
	
}

//--------------------------------------------------------------
void threesixtyUnwarp::exit(){
	player.close();
}


//=============================================
void threesixtyUnwarp::computePanoramaProperties(){
	
	//maxR_factor = 0.9 + 0.1*(float)mouseX/(float)ofGetWidth();
	//minR_factor = 0.2 + 0.1*(float)mouseY/(float)ofGetHeight();
	//printf("maxR_factor, minR_factor	%f	%f\n", maxR_factor, minR_factor);
	
	maxR  = warpedH * maxR_factor / 2;
	minR  = warpedH * minR_factor / 2;
}


//Used for the by hand portion and OpenCV parts of the shootout. 
//For the by hand, use the normal unwarpedW width instead of the step
//For the OpenCV, get the widthStep from the CvImage and use that for quarterstep calculation
//=============================================
void threesixtyUnwarp::computeInversePolarTransform(){

	// we assert that the two arrays have equal dimensions, srcxArray = srcyArray
	float radius, angle;
	float circFactor = 0 - TWO_PI/(float)unwarpedW;
	float difR = maxR-minR;
	int   dstRow, dstIndex;
	
	xocvdata = (float*) srcxArrayOpenCV.getCvImage()->imageData; 
	yocvdata = (float*) srcyArrayOpenCV.getCvImage()->imageData; 
	
	for (int dsty=0; dsty<unwarpedH; dsty++){
		float y = ((float)dsty/(float)unwarpedH);
		float yfrac = yWarpA*y*y + yWarpB*y + yWarpC;
		yfrac = MIN(1.0, MAX(0.0, yfrac)); 

		radius = (yfrac * difR) + minR;
		dstRow = dsty * unwarpedW; 	
		
		for (int dstx=0; dstx<unwarpedW; dstx++){
			dstIndex = dstRow + dstx;
			angle    = ((float)dstx * circFactor) + (DEG_TO_RAD * angularOffset);
			
			xocvdata[dstRow + dstx] = warpedCx + radius*cosf(angle);
			yocvdata[dstRow + dstx] = warpedCy + radius*sinf(angle);
		}
	}
	
	srcxArrayOpenCV.setFromPixels(xocvdata, unwarpedW, unwarpedH);
	srcyArrayOpenCV.setFromPixels(yocvdata, unwarpedW, unwarpedH);
}



//--------------------------------------------------------------
void threesixtyUnwarp::draw(){
	// draw everything.
	ofBackground(64,64,64);	
	
	drawPlayer();
	drawUnwarpedVideo();
	drawTexturedCylinder();	
}

//--------------------------------------------------------------
void threesixtyUnwarp::drawUnwarpedVideo(){
	// draw the unwarped (corrected) video in a strip at the bottom.
	ofSetColor(0xFFFFFF);
	unwarpedImage.draw(0,ofGetHeight() - unwarpedH);
}

//--------------------------------------------------------------
void threesixtyUnwarp::drawPlayer(){
	
	
	// draw the (warped) player
	ofSetColor(0xFFFFFF);
	playerScaleFactor = (float)(ofGetHeight() - unwarpedH)/(float)warpedH;
	player.draw(0, 0, playerScaleFactor*warpedW, playerScaleFactor*warpedH);
	
	// draw the center point, as a red cross
	ofSetColor(255,0,0);
	glEnable(GL_LINE_SMOOTH);
	ofEnableAlphaBlending();
	float f = playerScaleFactor;
	ofLine((f*(warpedCx))-5, (f*(warpedCy)),   (f*(warpedCx))+5, (f*(warpedCy)));
	ofLine((f*(warpedCx))  , (f*(warpedCy))-5, (f*(warpedCx)),   (f*(warpedCy))+5);
	ofDisableAlphaBlending();
	glDisable(GL_LINE_SMOOTH);
	
	
	// draw informational text about the app.
	ofSetColor(255,255,255);
	float ty = 10;
	float dy = 11;
	ofDrawBitmapString("BLOGGIE UNWARPER (OSX)",    10,ty+=dy);file://localhost/Users/golan/Desktop/OF61_106/apps/golan_of61_osx63/BloggieUnwarp/BloggieUnwarp.xcodeproj/
	ofDrawBitmapString("Made in OpenFrameworks",    10,ty+=dy);
	ofDrawBitmapString("By G.Levin & S.Bisker",     10,ty+=dy);
	ty+=dy;
	
	ofSetColor(255,0,0);
	ofDrawBitmapString("Drag cross or use",                  10, ty+=dy);
	ofDrawBitmapString("arrow keys to recenter.",            10, ty+=dy);
	ofDrawBitmapString("Other key commands:",                 10, ty+=dy);
	ofDrawBitmapString("'s' to Save settings.",                 10, ty+=dy);
	ofDrawBitmapString("'r' to Restore settings.",              10, ty+=dy);
	ofDrawBitmapString("'v' to export Video.",                  10, ty+=dy);
	ofDrawBitmapString("cx: "   + ofToString((warpedCx)),    10, ty+=dy);
	ofDrawBitmapString("cy: "   + ofToString((warpedCy)),    10, ty+=dy);
	ofDrawBitmapString("rot:"   + ofToString((angularOffset)), 10, ty+=dy);
	
	
	if (bSavingOutVideo){
		ofSetColor(255,255,100);
		
		float pos = player.getPosition(); // nWrittenFrames
		ofDrawBitmapString("SAVING VIDEO to:",				10, playerScaleFactor*warpedH - 20);
		int currF = player.getCurrentFrame();
		int nF    = player.getTotalNumFrames();
		sprintf(handyString, "data/%s (%d/%dF / %1.1f%c)...", outputFileName.c_str(), currF,nF, (pos*100.0), '%'); 	
		ofDrawBitmapString(handyString,						10, playerScaleFactor*warpedH - 10);
	} else {
		ofSetColor(200,200,200);
		
		sprintf(handyString, "Codec (-/+): %s", (videoRecorder->getCodecName(currentCodecId)).c_str());
		ofDrawBitmapString("Press 'v' to export video.",	10, playerScaleFactor*warpedH - 10);
		ofDrawBitmapString(handyString,			            10, playerScaleFactor*warpedH - 20);
	}
	

}

//--------------------------------------------------------------
void threesixtyUnwarp::drawTexturedCylinder(){
	// draw the texture-mapped cylinder.

	if ((ofGetWidth() > (playerScaleFactor*warpedW)) && 
	    (ofGetHeight() > unwarpedH)) {
		
		if (!bMousepressedInUnwarped){ // && !bMousePressedInPlayer){
			float A = 0.90;
			float B = 1.0-A;
			blurredMouseX = A*blurredMouseX + B*mouseX;
			blurredMouseY = A*blurredMouseY + B*mouseY;
		}
		
		unwarpedTexture.bind();
		glEnable(GL_DEPTH_TEST);
		glViewport(playerScaleFactor*warpedW, unwarpedH, 
				   ofGetWidth()-playerScaleFactor*warpedW, 
				   playerScaleFactor*warpedH);
		
		// magic numbers
		float cylz =           (ofGetHeight()/768.0) * 300.0;
		float cylinderRadius = (ofGetHeight()/768.0) * 16*10.00;       //175.0;
		float cylinderHeight = (ofGetHeight()/768.0) * 16*11.44/2.0; // 80.0;
		
		glPushMatrix();
		glTranslatef(ofGetWidth()/2, ofGetHeight()/2,cylz);
		if (!bMousePressed){
			glTranslatef(0,0,0);
			ofRotateX(RAD_TO_DEG * ofMap(blurredMouseY, 0, ofGetHeight(), -PI/2, PI/2));
			ofRotateY(RAD_TO_DEG * ofMap(blurredMouseX, 0, ofGetWidth(),  -PI, PI));
		} else {
			float offsetForOpticalCenter = 0 - cylinderHeight*0.4;
			// the Bloggie optical center is not in the center of the image.
			glTranslatef(0, offsetForOpticalCenter,cylz);
			ofRotateX(RAD_TO_DEG * ofMap(blurredMouseY, 0, ofGetHeight(), -PI/4, PI/4));
			ofRotateY(RAD_TO_DEG * ofMap(blurredMouseX, 0, ofGetWidth(),  TWO_PI, -TWO_PI));
		}
		
		ofSetColor(255,255,255);
		glBegin(GL_QUAD_STRIP);
		for (int i = 0; i < cylinderRes; i++) {
			
			float x = cylinderX[i] * cylinderRadius;
			float z = cylinderY[i] * cylinderRadius;
			float u = (float) unwarpedW / (cylinderRes-1) * i;
			
			glTexCoord3f(u, 0, 0);
			glVertex3f  (x, 0-cylinderHeight, z);
			glTexCoord3f(u, unwarpedH, 0);
			glVertex3f  (x, cylinderHeight,   z);
		}
		glEnd();
		glPopMatrix();
		
		unwarpedTexture.unbind();
		glDisable(GL_DEPTH_TEST);
		glViewport(0,0, ofGetWidth(), ofGetHeight());
	}
}


//--------------------------------------------------------------
void threesixtyUnwarp::update(){
	
	// computePanoramaProperties();    // NOT NECESSARY
	// computeInversePolarTransform(); // NOT NECESSARY

	if (bSavingOutVideo == false){
		
		ofSetVerticalSync(false);
		player.update();
		if (player.isFrameNew()  || (bPlayerPaused && !player.isFrameNew())){
				
			if (bCenterChanged || bAngularOffsetChanged){
				XML.setValue("CENTERX", warpedCx);
				XML.setValue("CENTERY", warpedCy);
				XML.setValue("ROTATION_DEGREES", angularOffset); 
				
				computePanoramaProperties();
				computeInversePolarTransform();
				
				bAngularOffsetChanged = false;
				bCenterChanged = false;
			}
			
			memcpy(warpedPixels, player.getPixels(), warpedW*warpedH*3);
			warpedIplImage->imageData = (char*) warpedPixels; 
			
			cvRemap(warpedIplImage,  unwarpedIplImage, 
					srcxArrayOpenCV.getCvImage(), 
					srcyArrayOpenCV.getCvImage(), 
					interpMethod | CV_WARP_FILL_OUTLIERS, blackOpenCV );
			 
			unwarpedPixels = (unsigned char*) unwarpedIplImage->imageData;
			unwarpedImage.setFromPixels(unwarpedPixels, unwarpedW, unwarpedH, OF_IMAGE_COLOR, true);
			unwarpedTexture.loadData(unwarpedPixels, unwarpedW, unwarpedH, GL_RGB);
		}
		
	//---------------------------------------------------
	// Else if we are saving out video..
	} else {
		
		ofSetVerticalSync(true);
		
		
		// do the unwarping into the unwarpedPixels.
		memcpy(warpedPixels, player.getPixels(), warpedW*warpedH*3);
		warpedIplImage->imageData = (char*) warpedPixels; 
		cvRemap(warpedIplImage,  unwarpedIplImage, 
				srcxArrayOpenCV.getCvImage(), 
				srcyArrayOpenCV.getCvImage(), 
				interpMethod | CV_WARP_FILL_OUTLIERS, blackOpenCV );
		unwarpedPixels = (unsigned char*) unwarpedIplImage->imageData;
		unwarpedImage.setFromPixels(unwarpedPixels, unwarpedW, unwarpedH, OF_IMAGE_COLOR, true);
		unwarpedTexture.loadData(unwarpedPixels, unwarpedW, unwarpedH, GL_RGB);
		
		if (videoRecorder->bAmSetupForRecording()){
			videoRecorder->addFrame(unwarpedPixels, (1.0f/30.00f));//29.97003f));
			nWrittenFrames++;
		}
		
		// assure that we are stepping through one frame at a time.
		float currF = (float) player.getCurrentFrame();
		float nF    = (float) player.getTotalNumFrames();
		float pct = (currF + 1.0)/nF;
		player.setPosition(pct);
			
		// update our state machine, stopping the recording if necessary.
		if (player.getIsMovieDone()){
			player.setLoopState(OF_LOOP_NORMAL);
			if (videoRecorder->bAmSetupForRecording()){
				
				if (bSaveAudioToo){
					// Strip audio from the original input movie; add it.
					string audioPath = XML.getValue("INPUT_FILENAME", "input.mov");
					videoRecorder->addAudioTrack(audioPath);
				}
				videoRecorder->finishMovie();
				printf("Finished exporting movie!\n"); 
			}
			bSavingOutVideo = false;
			player.setPaused(false); 
		} 
		
	}
}


//--------------------------------------------------------------
void threesixtyUnwarp::keyPressed  (int key){ 
	
	/*
	<!-- // Press Space to toggle movie play.                      --> 
	<!-- // Press 's' to save the geometry settings.               -->
	<!-- // Press 'r' to reload the previously saved settings.     -->
	<!-- // Use the +/- keys to change the export codec.           -->
	<!-- // Press 'v' to export the unwarped video.                -->
	<!-- // Use the arrow keys to nudge the center point.          -->
	<!-- // Drag the unwarped video left or right to shift it.     -->
	 */

	
	int nCodecs = videoRecorder->getNCodecs();
	
	switch (key){
		
			
		case '0':
		case '1':
		case '2':
			interpMethod = key - '0';
			break;
			
		case 356: // arrow left
			warpedCx -= 0.25;
			bCenterChanged = true;
			break;
		case 358: // arrow right
			warpedCx += 0.25;
			bCenterChanged = true;
			break;
		case 357: // arrow up
			warpedCy -= 0.25;
			bCenterChanged = true;
			break;
		case 359: // arrow down
			warpedCy += 0.25;
			bCenterChanged = true;
			break;
			
		case ' ':
			bPlayerPaused = !bPlayerPaused;
			player.setPaused(bPlayerPaused);
			break;
		
		case 'r':
		case 'R':
			warpedCx = savedWarpedCx;
			warpedCy = savedWarpedCy;
			angularOffset = savedAngularOffset;
			computeInversePolarTransform();
			break;
			
		case 's':
		case 'S':
			XML.setValue("CENTERX", warpedCx);
			XML.setValue("CENTERY", warpedCy);
			XML.setValue("ROTATION_DEGREES", angularOffset); 
			XML.saveFile("UnwarperSettings.xml");
			savedWarpedCx      = warpedCx;
			savedWarpedCy      = warpedCy;
			savedAngularOffset = angularOffset;
			printf("Saved settings to UnwarperSettings.xml.");
			break;
			
		case '-':
		case '_':	
			currentCodecId = MAX(0, currentCodecId-1);
			videoRecorder->setCodecType(currentCodecId);
			break;
		case '+':
		case '=':
			currentCodecId = MIN(nCodecs-1, currentCodecId+1);
			videoRecorder->setCodecType(currentCodecId);
			break;
		
		case 'v':
		case 'V':
			if (bSavingOutVideo == false){
				player.setLoopState(OF_LOOP_NONE); //OF_LOOP_NORMAL
				player.setPosition(0.0);
				player.setPaused(true);
				
				string inName = XML.getValue("INPUT_FILENAME", "input.mov"); 
				int inNameLastDotIndex = inName.find_last_of('.');
				if (inNameLastDotIndex > 1){
					inName = inName.substr (0, inNameLastDotIndex);
				}
				sprintf(handyString, "%2d%2d%2d.mov", ofGetHours(), ofGetMinutes(), ofGetSeconds());
				inName += "_out_" + ofToString(ofGetHours()) + ofToString(ofGetMinutes()) + ofToString(ofGetSeconds()) + ".mov"; 
				outputFileName = inName;
				
				videoRecorder->setup(unwarpedW, unwarpedH, outputFileName);
				if (videoRecorder->bAmSetupForRecording()){
					bSavingOutVideo = true;
					nWrittenFrames = 0;
				}
			}
			break;
		
	}
	
}



//--------------------------------------------------------------
void threesixtyUnwarp::mouseDragged(int x, int y, int button){
	bMousePressed = true;
	if (bMousePressedInPlayer){
		testMouseInPlayer();
	}
	if (bMousepressedInUnwarped && !bSavingOutVideo){
		angularOffset = ofMap(mouseX, 0, ofGetWidth(), 0-180, 180, false);
		bAngularOffsetChanged = true;
	}
}

//--------------------------------------------------------------
void threesixtyUnwarp::mousePressed(int x, int y, int button){
	bMousePressed         = true;
	bMousePressedInPlayer = testMouseInPlayer();
	
	bMousepressedInUnwarped = false;
	if (mouseY > (ofGetHeight() - unwarpedH)){
		bMousepressedInUnwarped = true;
	}
}

//--------------------------------------------------------------
void threesixtyUnwarp::mouseReleased(int x, int y, int button){
	if (bMousePressedInPlayer){
		testMouseInPlayer();
	}
	bMousepressedInUnwarped = false;
	bMousePressedInPlayer = false;
	bMousePressed = false;
}

//--------------------------------------------------------------
void threesixtyUnwarp::mouseMoved(int x, int y ){
	bMousepressedInUnwarped = false;
	bMousePressedInPlayer = false;
	bMousePressed = false;
}
//--------------------------------------------------------------
void threesixtyUnwarp::keyReleased(int key){ 
}
//--------------------------------------------------------------
void threesixtyUnwarp::windowResized(int w, int h){
}

//--------------------------------------------------------------
bool threesixtyUnwarp::testMouseInPlayer(){
	bool out = false;
	
	if ((mouseX < playerScaleFactor*warpedW) && 
		(mouseY < playerScaleFactor*warpedH)){
		
		if (bSavingOutVideo == false){
			float newCx = (float)mouseX * ((float)warpedH/(float)(ofGetHeight() - unwarpedH));
			float newCy = (float)mouseY * ((float)warpedH/(float)(ofGetHeight() - unwarpedH));	
			if ((newCx != warpedCx) || (newCy != warpedCy)){
				warpedCx = newCx;
				warpedCy = newCy;
			}
			bCenterChanged = true;
			out = true;
		}
	}
	return out;
}
