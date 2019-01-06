#include "ofMain.h"
#include <linux/input.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <stdio.h>
#include <syslog.h>
#include <fcntl.h>

#define EVENT_TYPE      EV_ABS
#define EVENT_CODE_X    ABS_X
#define EVENT_CODE_Y    ABS_Y
#define EVENT_CODE_P    ABS_PRESSURE
#define TOUCH_ADDED 	1
#define TOUCH_REMOVED 	2 

class ofxPiMultiTouch : public ofThread {
	public:

  	struct input_event ev;
	int fd, size;
	char name[256];
	char *device;
	int _pX, _pY;
	int _resX,_resY;
    int x,y,pressur;
	int releasedID;
	int activeSlot = 0;
	int lastTouchEvent = -1;

	std::list<int> activeTouches;
	bool trackPointsChanged = false;
	int slots[9];


	int init (string devicename, int w, int h, int valEvX=4095, int valEvY=4095){
		// &devicename[0u] == convert str to char*
		return init(&devicename[0u],  ofGetWidth(), ofGetHeight() ) ;  
	}

	int init(char * d, int w, int h, int valEvX=4095, int valEvY=4095){// for valEvX and valEvY check value Min Max evtest /dev/input/event0
		size = sizeof (struct input_event);
		name[0]='U';
		name[1]='n';
		name[2]='k';
		name[3]='n';
		name[4]='o';
		name[5]='w';
		name[6]='n';
		device = d;

        if ((fd = open(device, O_RDONLY)) < 0) {
            return 1;
		}
		_resX=w;
		_resY=h;
		_pX=valEvX;
		_pY=valEvY;

		ofLog() << "TouchName: " << getName() << endl;

		startThread();

		return 0;
	}



	string getName(){
		ioctl (fd, EVIOCGNAME (sizeof (name)), name);
		string str(name);
		return str;
	}

	void exit(){
		stopThread();
	}

	void threadedFunction(){

		while(isThreadRunning()) {
			const size_t ev_size = sizeof(struct input_event);
			ssize_t size;
			size = read(fd, &ev, ev_size);
			if (size < ev_size) {
				ofLog()<<"Error size!\n";
			}
			switch(ev.type){
				case EV_ABS:
					
					switch(ev.code){
						
						case ABS_MT_SLOT:
							activeSlot =  ev.value;
						break;

						case ABS_MT_TRACKING_ID:
							if(ev.value == -1){
								releasedID = slots[activeSlot];
								lastTouchEvent  = TOUCH_REMOVED;
							}else{
								lastTouchEvent  = TOUCH_ADDED;
							}
							trackPointsChanged = true;
							slots[activeSlot] = ev.value;
						
						break;

						case  ABS_MT_POSITION_X:
						case  ABS_X:
							x = ofMap(ev.value, 0,_pX,0,_resX);
						break;
						
						case  ABS_MT_POSITION_Y:
						case  ABS_Y:
							y = ofMap(ev.value, 0,_pY,0,_resY);
						break;
						
						case  ABS_PRESSURE:
							pressur = ev.value;
						break;
						
						default:
							ofLog() << "EV_ABS, other " <<  ev.code << " " << ev.value;
						break;
					}
					pos.set(x,y,pressur);
					
				
				break;

				case EV_KEY:
					if( ev.value==1){
						//ofLog() << "First Finger Touched";
					}else if(ev.value==0){
						//ofLog() << "All Fingers Released";
					}
					//ofLog() << "EV_KEY " << ev.code << " " << ev.value ;
				break;

				case EV_SYN:

					switch(ev.code){

						case SYN_REPORT:{
							ofTouchEventArgs touchArgs;
							touchArgs.x = pos.x;
							touchArgs.y = pos.y;
							if(trackPointsChanged){
								trackPointsChanged = false;
								if(lastTouchEvent == TOUCH_REMOVED){
									touchArgs.id = releasedID;
									ofNotifyEvent(ofEvents().touchUp, touchArgs, this);
								}else if(lastTouchEvent == TOUCH_ADDED){
									touchArgs.id = slots[activeSlot];
									ofNotifyEvent(ofEvents().touchDown, touchArgs, this);
								}else{
									//
								}
							}else{
								touchArgs.id = slots[activeSlot];
								ofNotifyEvent(ofEvents().touchMoved, touchArgs, this);
							}
							//ofLog() << "EV_SYN lastEvent: " << lastTouchEvent << " ,id " <<  slots[activeSlot] << " " << pos.x << " " << pos.y;
							lastTouchEvent = -1;
						break;
						}

						case SYN_DROPPED:
							ofLog() << "HELP! can't keep up";
						break;

						default:
						break;
					}
				break;

				default:{
					ofLog() << ev.type;
				}
				break;
					
			}
		}
	}

	ofVec3f pos;
	ofVec3f getCoordTouch(){
		return pos;
	}
};
