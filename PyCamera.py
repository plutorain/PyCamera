from PyQt5 import QtCore, uic
from PyQt5.QtGui import *
from PyQt5.QtWidgets import *
import sys
import cv2
import numpy as np
import threading
from datetime import datetime
import Queue
import os
import device

running = False
capture_thread = None
get_picture = None
form_class = uic.loadUiType("simple.ui")[0]
q = Queue.Queue()
 

def grab(cam, queue, width, height, fps):
    global running
    global get_picture
    capture = cv2.VideoCapture(cam)
    capture.set(cv2.CAP_PROP_FRAME_WIDTH, width)
    capture.set(cv2.CAP_PROP_FRAME_HEIGHT, height)
    capture.set(cv2.CAP_PROP_FPS, fps)

    while(running):
        frame = {}        
        capture.grab()
        retval, img = capture.retrieve(0)
        frame["img"] = img

        if queue.qsize() < 10:
            queue.put(frame)
            if(get_picture): #picture save
                now = datetime.now()       
                name = now.isoformat()
                name = name.replace("-", "_")
                name = name.replace(":", "_")
                name = name.replace("T", "_")
                name = name[0:22] 
                resized = cv2.resize(img, (640, 360) , interpolation=cv2.INTER_LINEAR)
                cv2.imwrite(os.path.join(os.getcwd(), name+'.jpg') ,resized)
                get_picture = False
        else:
            print queue.qsize()

class OwnImageWidget(QWidget):
    def __init__(self, parent=None):
        super(OwnImageWidget, self).__init__(parent)
        self.image = None

    def setImage(self, image):
        self.image = image
        sz = image.size()
        self.setMinimumSize(sz)
        self.update()

    def paintEvent(self, event):
        qp = QPainter()
        qp.begin(self)
        if self.image:
            qp.drawImage(QtCore.QPoint(0, 0), self.image)
        qp.end()



class MyWindowClass(QMainWindow, form_class):
    def __init__(self):
        QMainWindow.__init__(self)
        self.setupUi(self)

        self.startButton.clicked.connect(self.start_clicked)
        self.captureButton.clicked.connect(self.capture_clicked)
        self.captureButton.setEnabled(False)
        
        self.window_width = self.ImgWidget.frameSize().width()
        self.window_height = self.ImgWidget.frameSize().height()
        self.ImgWidget = OwnImageWidget(self.ImgWidget)       

        self.timer = QtCore.QTimer(self)
        self.timer.timeout.connect(self.update_frame)
        self.timer.start(1)
        
              
        # Get camera list
        self.device_list = device.getDeviceList()
        print(self.device_list)
        index = 0
        for name in self.device_list:
            print(str(index) + ': ' + name)
            self.comboBox.insertItem(index, name)
            index += 1
        self.last_index = index - 1

        if self.last_index < 0:
            print("No device is connected")
            self.startButton.setEnabled(False)
        

    def start_clicked(self):
        global capture_thread
        cam_index=self.comboBox.currentIndex()
        capture_thread = threading.Thread(target=grab, args = (0, q, 1920, 1080, 30))
        global running
        running = True
        capture_thread.start()
        self.startButton.setEnabled(False)
        self.startButton.setText('Starting...')
        self.captureButton.setEnabled(True)
    
    def capture_clicked(self):
        global get_picture
        get_picture = True


    def update_frame(self):
        if not q.empty():
            self.startButton.setText('Camera is running')
            frame = q.get()
            img = frame["img"]

            img_height, img_width, img_colors = img.shape
            scale_w = float(self.window_width) / float(img_width)
            scale_h = float(self.window_height) / float(img_height)
            scale = min([scale_w, scale_h])

            if scale == 0:
                scale = 1
            
            img = cv2.resize(img, None, fx=scale, fy=scale, interpolation = cv2.INTER_CUBIC)
            img = cv2.cvtColor(img, cv2.COLOR_BGR2RGB)
            height, width, bpc = img.shape
            bpl = bpc * width
            image = QImage(img.data, width, height, bpl, QImage.Format_RGB888)
            self.ImgWidget.setImage(image)
            

    def closeEvent(self, event):
        global running
        running = False
        
        


if __name__ == "__main__":
   
    print("OpenCV version: " + cv2.__version__) 
    app = QApplication(sys.argv)
    w = MyWindowClass()
    w.setWindowTitle('USB Camera TEST')
    w.show()
    app.exec_()