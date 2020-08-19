import _thread
from imutils.video import VideoStream
import imutils
import time
import numpy as np
import cv2
 

cap = VideoStream('./demoVideo/'+'output22020-08-12-09-23-35.mp4').start()
n=0
while True:
    img = cap.read()
    n=n+1
    cv2.imshow('frame11', img)
    cv2.waitKey(1)
cap.release()

