/*
 *  meanMeshFbo.cpp
 *  BasicExample
 *
 *  Created by zachary lieberman on 10/11/11.
 *  Copyright 2011 __MyCompanyName__. All rights reserved.
 *
 */

#include "meanMeshRenderer.h"

#include "testApp.h"


void meanMeshRenderer::setup() {
	
	
	// --------------------------------------------- get the still tracker into an FBO
	stillTracker.setup();
	stillTracker.setRescale(1.0);
	stillTracker.setIterations(20);
	still.loadImage("face.png");
	stillDepth.loadImage("depth.png");
	stillTracker.update(ofxCv::toCv(still));
	stillTracker.update(ofxCv::toCv(still));
	stillTracker.update(ofxCv::toCv(still));
	
	// find the bounding box; 
	meanMesh = stillTracker.getMeanObjectMesh();
	
	// ---------------------------------------------  we like foreheads
	
	addForheadToFaceMesh(meanMesh);	// forehead time!
	
	// --------------------------------------------- compute min max on the points, and figure out the w/h of this fbo
	
	ofPoint min, max;
	for (int i = 0; i < meanMesh.getVertices().size(); i++){
		if (i == 0){
			min.x = meanMesh.getVertices()[i].x;
			min.y =meanMesh.getVertices()[i].y;
			max.x =meanMesh.getVertices()[i].x;
			max.y =meanMesh.getVertices()[i].y;
		} else {
			min.x = MIN(min.x,meanMesh.getVertices()[i].x);
			min.y = MIN(min.y,meanMesh.getVertices()[i].y);
			max.x = MAX(max.x,meanMesh.getVertices()[i].x);
			max.y = MAX(max.y,meanMesh.getVertices()[i].y);
		}
		
		meanMesh.getVertices()[i].z = 0; // set it to zero, for this purpose
	}
	
	
	for (int i = 0; i < meanMesh.getVertices().size(); i++){
		meanMesh.getVertices()[i].x -= min.x;
		meanMesh.getVertices()[i].y -= min.y;
	}
	
	width = (max.x - min.x) * 10.0;
	height = (max.y - min.y) * 10.0;
	
	// --------------------------------------------- draw the normal fbo
	
	meanMeshFbo.allocate(width, height, GL_RGBA, 4);
	meanMeshFbo.begin();
	ofClear(0,0,0,255);
	ofSetColor(255,255,255);
	ofPushMatrix();
	//ofTranslate(-min.x*5, -min.y*5, 0); //, -min.y, 0);
	ofScale(10,10,1);
	ofSetColor(255,127,0);
	meanMesh.draw();
	ofPopMatrix();
	meanMeshFbo.end();
	
	// --------------------------------------------- depth stuff
	
	pixelsDepth.allocate(width, height, OF_PIXELS_RGB);
	depthImage.allocate(width, height, OF_IMAGE_COLOR);
	
	meanMeshAsDepthFbo.allocate(width, height, GL_RGBA, 4);
	meanMeshAsDepthFbo.begin();
	ofClear(0,0,0,255);
	
	stillDepth.getTextureReference().bind();
	ofPushMatrix();
	//ofTranslate(-min.x*5, -min.y*5, 0); //, -min.y, 0);
	ofScale(10,10,1);
	ofSetColor(0,0,0);
	ofFill();
	ofRect(0,0,width, height);
	ofSetColor(255,255,255);
	meanMesh.draw();
	ofPopMatrix();
	stillDepth.getTextureReference().unbind();
	meanMeshAsDepthFbo.end();
	
	// --------------------------------------------- getting the dpeth pixels out for useful usage
	
	meanMeshAsDepthFbo.readToPixels(pixelsDepth, 0);
	depthImage.setFromPixels(pixelsDepth);
	ofxCv::blur(depthImage, 3);ofxCv::blur(depthImage, 3);
	//ofxCv::blur(ofxCv::toCv(depthImage), 3);
	depthImage.setImageType(OF_IMAGE_GRAYSCALE);
	depthImage.update();
	
	// ---------------------------------------------	let's have out internal mesh match the FBO coodinates with textures
	//													so it's easy to draw this to different points. 
	
	// now update the coords, 
	for (int i = 0; i < meanMesh.getTexCoords().size(); i++){
		meanMesh.getTexCoords()[i].x = meanMesh.getVertices()[i].x * 10;
		meanMesh.getTexCoords()[i].y = meanMesh.getVertices()[i].y * 10;
	}
}



void meanMeshRenderer::setupMPAMesh(){

	// ---------------------------------------------  once mpa exists let's try to make a mesh internally with alot more points (for depth lookup)
	
	ofMesh temp = meanMesh;
	for (int i = 0; i < temp.getVertices().size(); i++){
		temp.getVertices()[i] *= 10;
	}
	ofMesh nonindices = convertFromIndices(temp);
	meanMeshThroughMPA = ((testApp*)ofGetAppPtr())->MPA.returnMeshWarpedToThisMesh(nonindices);
}

void meanMeshRenderer::clear(){
	
	// --------------------------------------------- this is some clearing and testing drawing somethign funky
	meanMeshFbo.begin();
	ofClear(0,0,0,0);
	ofSetColor(255,255,255);
	ofPushMatrix();
	ofScale(10,10,1);
	ofSetColor(255,255,255);
	//meanMesh.draw();
	ofPopMatrix();
	for (int i = 0; i < 10; i++){
		ofSetColor(255,0,0);
		ofFill();
		ofCircle(width * (0.5*sin(ofGetElapsedTimef()*(3+i/7.0) ) + 0.5), height * (0.5*sin(ofGetElapsedTimef()*(6+i/3.0) ) + 0.5), 40);
	}
	meanMeshFbo.end();
}

void meanMeshRenderer::draw(){
		
	ofSetColor(255);
	depthImage.draw(0,0);
}


void meanMeshRenderer::draw(ofxFaceTracker & tracker){
	
	// --------------------------------------------- draw this fbo to someone elses face, ie remap the position based on this tracker. 
	
	ofMesh target = tracker.getImageMesh();
	addForheadToFaceMesh(target);
	if (target.getVertices().size() != meanMesh.getVertices().size()) return;
	ofMesh copy = meanMesh;
	for (int i =0; i < copy.getVertices().size(); i++){
		copy.getVertices()[i] = target.getVertices()[i];
	}
	// copy vertices;
	meanMeshFbo.getTextureReference().bind();
	ofSetColor(255,255,255);
	copy.draw();
	meanMeshFbo.getTextureReference().unbind();
}

