# coding:utf-8
import cv2
import sys
import time
# reload(sys)
# sys.setdefaultencoding('utf8')

time = time.strftime('%Y-%m-%d-%H-%M-%S', time.localtime(time.time()))
cap = cv2.VideoCapture(0)
cap.set(3,640)
cap.set(4,480)
cap.set(1, 10.0)


#�˴�fourcc����MAC����Ч�������Ƶ����Ϊ�գ���ô���Ը�һ�������������, Ҳ������-1
fourcc = cv2.VideoWriter_fourcc('m', 'p', '4', 'v')
# �������������Ǿ�ͷ�����ģ�10Ϊ������С��10Ϊ����ͷ
out = cv2.VideoWriter('./demoVideo/output2'+time+'.avi', fourcc, 10, (640, 480))
while True:
    ret,frame = cap.read()
    if ret == True:
        frame = cv2.flip(frame, 1)
        a = out.write(frame)
        # cv2.imshow("frame", frame)
        if cv2.waitKey(1) & 0xFF == ord('q'):
            break
    else:
        break
cap.release()
out.release()
cv2.destroyAllWindows()
