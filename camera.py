import pygame.mixer
from time import sleep
import time
import picamera
import RPi.GPIO as GPIO
from sys import exit

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
            exit()
