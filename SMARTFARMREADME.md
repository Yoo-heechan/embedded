Smart Farm in Cloud에 대해 소개해드리겠습니다.

main함수에서 4개의 쓰레드를 생성합니다.

monitor함수에서 온도와 빛 센서 값을 받아옵니다.

온도가 20도 이상일 때 fan쓰레드에 시그널을 보내고 빛 센서 값이 800이하 일  때
led 쓰레드에 시그널을 보냅니다.

sendsensor함수에서 mysql문을 사용하여 디비에 온도와  빛 센서 값을 10초 마다
삽입합니다.
 
turnfan쓰레드에서 온도값을 다시 받아 5초 동안 팬이 실행되도록 합니다.

turnled쓰레드에서 빛 센서 값이 800이하일 때 led 불빛이 빨강,파랑,초록이 
차례되로 불이 켜집니다.

youtube 주소: https://youtu.be/DWt1l7qbAAs