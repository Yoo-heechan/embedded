#include <wiringPi.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/types.h>
#include <unistd.h>
#include <wiringPiSPI.h>
#include <signal.h>
#include <pthread.h>
#include "/usr/include/mysql/mysql.h"

#define MAXTIMINGS 85
#define MAX 5

#define CS_MCP3208 24
#define SPI_CHANNEL 0
#define SPI_SPEED 1000000

#define FAN 22
#define RED 7
#define GREEN 9
#define BLUE 8

int ret_temp;
int buffert[MAX] = {0,0,0,0,0};
int bufferl[MAX] = {0,0,0,0,0};
int fill_ptr = 0;
int use_ptr = 0;
int count = 0;
int loops = 8;
int done = 0;
int flag = 0;

int tmp;
int light;

static int DHTPIN = 11;
 
static int dht22_dat[5] = {0,0,0,0,0};
 
pthread_cond_t monitor, send, fan, led;
pthread_mutex_t mutex;
pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
pthread_cond_t c = PTHREAD_COND_INITIALIZER;

static uint8_t sizecvt(const int read)
{
 
				if (read > 255 || read < 0)
				{
								printf("Invalid data from wiringPi library\n");
								exit(EXIT_FAILURE);
				}
  
				return (uint8_t)read;
}

int read_dht22_dat()
{
				uint8_t laststate = HIGH;
				uint8_t counter = 0;
				uint8_t j = 0, i;
 
				dht22_dat[0] = dht22_dat[1] = dht22_dat[2] = dht22_dat[3] = dht22_dat[4] = 0;
 
				pinMode(DHTPIN, OUTPUT);
				digitalWrite(DHTPIN, HIGH);
				delay(10);
				digitalWrite(DHTPIN, LOW);
				delay(18);
				digitalWrite(DHTPIN, HIGH);
				delayMicroseconds(40);
				pinMode(DHTPIN, INPUT);

				for ( i=0; i< MAXTIMINGS; i++) 
				{
								counter = 0;
								while (sizecvt(digitalRead(DHTPIN)) == laststate) {
												counter++;
												delayMicroseconds(1);
												if (counter == 255) {
																break;
												}
								}
  
								laststate = sizecvt(digitalRead(DHTPIN));
 
								if (counter == 255) break;
 
								if ((i >= 4) && (i%2 == 0)) {
												dht22_dat[j/8] <<= 1;
												if (counter > 50)
																dht22_dat[j/8] |= 1;
												j++;
								}
				}
				
				if ((j >= 40) && (dht22_dat[4] == ((dht22_dat[0] + dht22_dat[1] + dht22_dat[2] + dht22_dat[3]) & 0xFF)) ) 
				{
								float t;

								t = (float)(dht22_dat[2] & 0x7F)* 256 + (float)dht22_dat[3];
								t /= 10.0;
  
								if ((dht22_dat[2] & 0x80) != 0) t *= -1;

								ret_temp = (int)t;

								return ret_temp;
				}

				else
				{
								return 0;
				}
}

int read_mcp3208_adc(unsigned char adcChannel)
{
				unsigned char buff[3];
				int adcValue = 0;
 
				buff[0] = 0x06 | ((adcChannel & 0x07) >> 2);
				buff[1] = ((adcChannel & 0x07) << 6);
				buff[2] = 0x00;
 
				digitalWrite(CS_MCP3208, 0);
				wiringPiSPIDataRW(SPI_CHANNEL, buff, 3);
 
				buff[1] = 0x0f & buff[1];
				adcValue = (buff[1] << 8 ) | buff[2];
 
				digitalWrite(CS_MCP3208, 1);
 
				return adcValue;
} 

void sig_handler(int signo)
{
				printf("process stop\n");
				digitalWrite(FAN, 0) ; 
				digitalWrite(GREEN, 0);
				digitalWrite(BLUE, 0);
				digitalWrite(RED, 0);

				exit(0);
} 

void thr_exit()
{
				pthread_mutex_lock(&m);
				done = 1;
				pthread_cond_signal(&c);
				pthread_mutex_unlock(&m);
}

void thr_join()
{
				pthread_mutex_lock(&m);
				while(done==0)
								pthread_cond_wait(&c, &m);
				pthread_mutex_unlock(&m);
}

void put(int value1, int value2)
{
				buffert[fill_ptr] = value1;
				bufferl[fill_ptr] = value2;
				fill_ptr = (fill_ptr + 1) % MAX;
				count++;
}

void get()
{
				tmp = buffert[use_ptr];
				light = bufferl[use_ptr];
}

void push()
{
				use_ptr = (use_ptr + 1) % MAX;
				count--;
}

void *Monitor(void *arg)
{
				int received_temp, a = 0;
				unsigned char adcChannel_light = 0;
				int adcValue_light = 0;
				
				while(1)
				{
								pthread_mutex_lock(&mutex);

								flag=0;
								while(count == MAX)
												pthread_cond_wait(&monitor, &mutex);
								
								while(read_dht22_dat()==0)
								{
												delay(500);
								}
								received_temp = ret_temp ;
								printf("Temperature = %d\n", received_temp);
								adcValue_light = read_mcp3208_adc(adcChannel_light);
								printf("light sensor = %u\n", adcValue_light);								

								put(received_temp, adcValue_light);

								if(received_temp>=20)
								{
												flag=2;
												pthread_cond_signal(&fan);
												pthread_mutex_unlock(&mutex);
								}

								pthread_mutex_lock(&mutex);
								while(count == MAX)
												pthread_cond_wait(&monitor, &mutex);
								
								if(adcValue_light<=800)
								{
												flag=3;
												pthread_cond_signal(&led);
								}

								delay(1);
								pthread_mutex_unlock(&mutex);

				}
}

void *Sendsensor(void *arg)
{
				int i, stat;
				char query[255];

				MYSQL *conn = mysql_init(NULL);

				if(!conn)
				{
								printf("error");
								exit(1);
				}

				conn = mysql_real_connect(conn, "localhost", "root", "root", "smartfarmdb", 0, NULL, 0);

				if(conn)
				{
							printf("Database connection success\n");
				}

				while(1)
				{
								pthread_mutex_lock(&mutex);
								printf("send start\n");
								while(count==0)
												pthread_cond_wait(&send, &mutex);
								get();
								printf("Temperature : %d°C lightness : %d data into database\n", tmp, light);

								sprintf(query, "INSERT INTO smartfarm(temperature, lightsensor) values('%d', '%u')", tmp, light);
								stat = mysql_query(conn, query);

								push();
								if(stat != 0)
												printf("insert error\n");	
								
								delay(10000);
								printf("send end\n");
								pthread_cond_signal(&monitor);
								pthread_mutex_unlock(&mutex);
				}
}

void *TurnFan(void *arg)
{
				pinMode(FAN, OUTPUT);

				while(1)
				{
								pthread_mutex_lock(&mutex);
								printf("fan start\n");

								while(flag != 2)
												pthread_cond_wait(&fan, &mutex);
												
								get();

								if(tmp>=20)
								{
												digitalWrite(FAN, 1);

												delay(5000);
												digitalWrite(FAN, 0);
								}

								pthread_cond_signal(&monitor);
								printf("fan end");
								pthread_mutex_unlock(&mutex);
				}
}

void *TurnLed(void *arg)
{
				while(1)
				{
								pthread_mutex_lock(&mutex);
								printf("led start\n");

								while(flag!=3)
												pthread_cond_wait(&led, &mutex);
												
								get();

								if(light<=800)
								{

												digitalWrite(RED, 1);
												digitalWrite(BLUE, 0);
												digitalWrite(GREEN, 0);

												delay(1000);

												digitalWrite(RED, 0);
												digitalWrite(BLUE, 1);
												digitalWrite(GREEN, 0);

												delay(1000);

												digitalWrite(RED, 0);
												digitalWrite(BLUE, 0);
												digitalWrite(GREEN, 1);
												delay(1000);

												digitalWrite(GREEN, 0);		
								}

							  flag=1;	
								pthread_cond_signal(&monitor);

								printf("led end");
								pthread_mutex_unlock(&mutex);
				}
}

int main (void)
{
				int received_temp, i=0;
				int stat;
				char query[255];
				int tem[5] = {0,0,0,0,0};

				float vout_light;
				float vout_oftemp;
				float percentrh = 0;
				float supsiondo = 0;

				signal(SIGINT, (void *)sig_handler);
				pthread_t p1, p2, p3, p4;

				if(wiringPiSPISetup(SPI_CHANNEL, SPI_SPEED) == -1)
				{
								fprintf(stdout, "wiringPiSPISetup Failed :%s\n", strerror(errno));
								return 1;
				}

				if (wiringPiSetup() == -1)
								exit(EXIT_FAILURE) ;

				if (setuid(getuid()) < 0)
				{
								perror("Dropping privileges failed\n");
								exit(EXIT_FAILURE);
				}

				pinMode(RED, OUTPUT);
				pinMode(GREEN, OUTPUT);
				pinMode(BLUE, OUTPUT);

				pthread_create(&p1, NULL, Monitor, NULL);
				pthread_create(&p2, NULL, Sendsensor, NULL);
				pthread_create(&p3, NULL, TurnFan, NULL);
				pthread_create(&p4, NULL, TurnLed, NULL);

				thr_join();

				return 0;
}
