import RPi.GPIO as GPIO
from sys import exit
import time
GPIO.setmode(GPIO.BCM)
import pygame.mixer
from time import sleep
import picamera

TRIG=23
ECHO=24

GPIO.setup(TRIG, GPIO.OUT)
GPIO.setup(ECHO, GPIO.IN)
GPIO.output(TRIG, GPIO.LOW)
time.sleep(0.3)

while True:
    GPIO.output(TRIG, True)
    time.sleep(0.00001)
    GPIO.output(TRIG, False)

    while GPIO.input(ECHO)==0:
        pulse_start = time.time()

    while GPIO.input(ECHO)==1:
        pulse_end = time.time()

    pulse_duration = pulse_end - pulse_start

    distance = pulse_duration * 17150
    distance = round(distance, 2)

    print "Distance : ", distance, "cm"
    if (distance < 40):
        break

#GPIO.cleanup()

GPIO.setmode(GPIO.BCM)
GPIO.setup(17, GPIO.IN, GPIO.PUD_UP)
GPIO.setup(21, GPIO.OUT)
GPIO.setup(12, GPIO.IN)
GPIO.setup(16, GPIO.IN)
GPIO.setup(20, GPIO.IN)

pygame.mixer.init(48000, -16, 1, 1024)
soundB = pygame.mixer.Sound("./python_games/match1.wav")
soundC = pygame.mixer.Sound("./python_games/match2.wav")

soundChannelA = pygame.mixer.Channel(2)
soundChannelA = pygame.mixer.Channel(3)

with picamera.PiCamera() as camera:
    GPIO.output(21, GPIO.HIGH)
    #camera.start_preview()
    while True:
        try:
            if (GPIO.input(16) == True):
                soundChannelA.play(soundB)
            if (GPIO.input(20) == True):
                soundChannelA.play(soundC)
            sleep(.5)
        except KeyboardInterrupt:
            camera.start_preview()
            GPIO.wait_for_edge(17, GPIO.FALLING)
            camera.capture('/home/pi/Desktop/image.jpg')
            camera.stop_preview()
            GPIO.output(21, GPIO.LOW)
            GPIO.cleanup()
            exit()

