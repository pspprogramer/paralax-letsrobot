/**
* This is the main twitchBot program file.
*/
#define USING_360_SERVOS

#include "simpletools.h"
#include "fdserial.h"
#include "abdrive360.h"
#include "ping.h"
#ifdef USING_360_SERVOS
  #include "servo360.h"
#else
  #include "servo.h"
#endif  
#include "ws2812.h"

//#define STRING //Defines whether its String Fury or not
#ifdef STRING
    #define firing_interval CLKFREQ *5 //Time between shots
    #define firing_duration CLKFREQ /2 //Duration of shots
    uint32_t firing_timer = 0;
    uint8_t fire_flag = 0;
    uint32_t firing_duration_timer = 0;
    int16_t can_position = 200;
#endif

#define LED_PIN     8
#define LED_COUNT   18



void Step(int leftSpeed, int rightSpeed);
void Stop(void);
void led_blink();
void eyes_blink();
void motor_controller();
void neopixel_controller();
void set_motor_controller(int leftSpeed, int rightSpeed);
void set_neopixel_group(uint32_t color);
void set_neopixel(uint8_t pixel_num, uint32_t color);
void pause(int ms);
void refresh_eyes();
void increase_brightness();
void decrease_brightness();

uint32_t ledColors[LED_COUNT];
uint32_t dim_array[LED_COUNT];

ws2812_t driver;
int ticks_per_ms;
uint8_t brightness = 10;
uint32_t eye_color = 0xFFFFFF;
  

fdserial *term; //enables full-duplex serilization of the terminal (In otherwise, 2 way signals between this computer and the robot)
//int global vars
int ticks = 12; //each tick makes the wheel move by 3.25mm, 64 ticks is a full wheel rotation (or 208mm)
int turnTick = 6; //Turning is at half the normal rate
int maxSpeed = 128; //the maximum amount of ticks the robot can travel with the "drive_goto" function
int minSpeed = 2; //the lowest amount of ticks the robot can travel with the "drive_goto" function
int maxTurnSpeed = 64;
int minTurnSpeed = 2;
int gripDegree = 0; //Angle of the servo that controls the gripper
int gripState = -1;
int commandget = 0;

int ledmask[18];
int tmask[18];
int nmask0[18] = {1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1}; // all on
int nmask1[18] = {1,1,1,1,0,1,1,1,1,1,1,1,1,0,1,1,1,1}; // o.o
int nmask2[18] = {0,0,0,1,1,1,0,0,0,0,0,0,1,1,1,0,0,0}; // -.-
int nmask3[18] = {0,1,0,1,0,1,0,0,0,0,1,0,1,0,1,0,0,0}; // v.v
int nmask4[18] = {0,0,0,1,0,1,0,1,0,0,0,0,1,0,1,0,1,0}; // ^.^
int nmask5[18] = {1,0,1,0,1,0,1,0,1,1,0,1,0,1,0,1,0,1}; // x.x
int nmask6[18] = {0,1,1,1,1,0,1,0,0,1,1,0,0,1,1,0,0,1}; // \./
int nmask7[18] = {0,1,0,0,1,0,1,1,1,0,1,0,0,1,0,1,1,1}; // T.T
int nmask8[18] = {1,1,1,1,0,1,1,0,1,1,1,1,1,0,1,1,0,1}; // U.U
int nmask9[18] = {1,0,1,1,0,1,1,1,1,1,0,1,1,0,1,1,1,1}; // n.n
uint32_t cop1[18] = {0xFF0000 ,0xFF0000 ,0xFF0000 ,0xFF0000 ,0xFF0000 ,0xFF0000 ,0xFF0000 ,0xFF0000 ,0xFF0000 ,0x0000FF ,0x0000FF ,0x0000FF ,0x0000FF ,0x0000FF ,0x0000FF ,0x0000FF ,0x0000FF ,0x0000FF};
uint32_t cop2[18] = {0x0000FF ,0x0000FF ,0x0000FF ,0x0000FF ,0x0000FF ,0x0000FF ,0x0000FF ,0x0000FF ,0x0000FF ,0xFF0000 ,0xFF0000 ,0xFF0000 ,0xFF0000 ,0xFF0000 ,0xFF0000 ,0xFF0000 ,0xFF0000 ,0xFF0000};
uint32_t lrn[18] = {0xA652AF ,0x000000 ,0x5F79FF ,0xF16B74 ,0x000000 ,0x21BCE5 ,0xF9AA67 ,0xF3EB48 ,0x97E062 ,0xA652AF ,0x000000 ,0x5F79FF ,0xF16B74 ,0x000000 ,0x21BCE5 ,0xF9AA67 ,0xF3EB48 ,0x97E062};
volatile int current_leftspd = 0;
volatile int current_rightspd = 0;
volatile int motor_flag = 0;





int defaultStraightSpeed = 60;
int defaultTurnSpeed = 15;

int pingDistance;

int verbose = 1;


int main()
{
  
   //variables for adjusting the encoded servos:
   //setKpMin(6);
   //setKpDistDen(8);
  // setKilncrement(1);
//   setScaleXferFunct(200);
   
	//access the simpleIDE terminal
	simpleterm_close();
	//set full-duplex serialization for the terminal
	term = fdserial_open(31, 30, 0, 9600);
 
   ticks_per_ms = CLKFREQ / 1000;
  
	cog_run(motor_controller,128);
    // load the LED driver
    if (ws2812b_init(&driver) < 0)
       return 1;
       pause(500);
       set_neopixel_group(0xFFFFFF);
         memcpy(tmask, nmask2, sizeof(nmask2));
   eyes_blink();
   #ifdef STRING
   servo_angle(17,can_position);
   servo_angle(1,0);
   #endif    

	char c;

	//servo_angle(1, gripDegree); //Orient gripper to half open on start
	
 //pause(3000);
  
  int inputStringLength = 20;
	
//  int i = 0;
//  while(i<=inputStingLength)
//  {
  char inputString[inputStringLength];
//  i++;	
//  }
	int sPos = 0;

	while (1)
	{


		if (fdserial_rxReady(term)!=0)
		{
			c = fdserial_rxChar(term); //Get the character entered from the terminal

			if (c != -1) {
				dprint(term, "%d", (int)c);
				if ((int)c == 13 || (int)c == 10) {
					dprint(term, "received line:");
					dprint(term, inputString);
					dprint(term, "\n");
					if (strcmp(inputString, "l") == 0) {
						dprint(term, "left");
						set_motor_controller(-defaultTurnSpeed,defaultTurnSpeed);
					}          
					if (strcmp(inputString, "r") == 0) {
						dprint(term, "right");
						set_motor_controller(defaultTurnSpeed, -defaultTurnSpeed);
					}          
					if (strcmp(inputString, "f") == 0) {
						dprint(term, "forward");
						set_motor_controller(defaultStraightSpeed, defaultStraightSpeed);
					}          
					if (strcmp(inputString, "b") == 0) {
						dprint(term, "back");
						set_motor_controller(-defaultStraightSpeed, -defaultStraightSpeed);
					}
					if (strcmp(inputString, "l_up") == 0) {
						dprint(term, "left_stop");
						Stop();
					}          
					if (strcmp(inputString, "r_up") == 0) {
						dprint(term, "right_stop");
						Stop();
					}          
					if (strcmp(inputString, "f_up") == 0) {
						dprint(term, "forward_stop");
						Stop();
					}          
					if (strcmp(inputString, "b_up") == 0) {
						dprint(term, "back_stop");
						Stop();
					}
					if (strcmp(inputString, "brightness_up") == 0) {
						increase_brightness();
                dprint(term,"brightness increased");
					}
     
     			if (strcmp(inputString, "brightness_down") == 0) {
						decrease_brightness();
               dprint(term,"brightness decreased");
					}
          	if (strcmp(inputString, "ms0") == 0) {
						  memcpy(ledmask, nmask0, sizeof(nmask0));
               refresh_eyes();
               dprint(term,"mask 0");
					}
            if (strcmp(inputString, "ms1") == 0) {
						  memcpy(ledmask, nmask1, sizeof(nmask1));
               refresh_eyes();
               dprint(term,"mask 1");
					}
            if (strcmp(inputString, "ms2") == 0) {
						  memcpy(tmask, nmask2, sizeof(nmask2));
               eyes_blink();
               dprint(term,"mask 2");
					}
            if (strcmp(inputString, "ms3") == 0) {
						  memcpy(ledmask, nmask3, sizeof(nmask3));
               refresh_eyes();
               dprint(term,"mask 3");
					}
            if (strcmp(inputString, "ms4") == 0) {
						  memcpy(ledmask, nmask4, sizeof(nmask4));
               refresh_eyes();
               dprint(term,"mask 4");
					}
            if (strcmp(inputString, "ms5") == 0) {
						  memcpy(ledmask, nmask5, sizeof(nmask5));
               refresh_eyes();
               dprint(term,"mask 5");
					}
            if (strcmp(inputString, "ms6") == 0) {
						  memcpy(ledmask, nmask6, sizeof(nmask6));
               set_neopixel_group(0xFF0000);
               refresh_eyes();
               dprint(term,"mask 6");
					}
            if (strcmp(inputString, "ms7") == 0) {
						  memcpy(ledmask, nmask7, sizeof(nmask7));
               refresh_eyes();
               dprint(term,"mask 7");
					}
            if (strcmp(inputString, "ms8") == 0) {
						  memcpy(ledmask, nmask8, sizeof(nmask8));
               refresh_eyes();
               dprint(term,"mask 8");
					}
            if (strcmp(inputString, "ms9") == 0) {
						  memcpy(ledmask, nmask9, sizeof(nmask9));
               refresh_eyes();
               dprint(term,"mask 9");
					}
            if (strcmp(inputString, "lrn") == 0) {
						  memcpy(ledmask, nmask9, sizeof(nmask9));
                memcpy(ledColors, lrn, sizeof(lrn));
               refresh_eyes();
               dprint(term,"mask 9");
					}
            if (strcmp(inputString, "cop") == 0) {
                memcpy(ledmask, nmask0, sizeof(nmask0));
                int a=0;
                for (a=0; a <= 5; ++a)
                {
						  memcpy(ledColors, cop1, sizeof(cop1));
               refresh_eyes();
                pause(400);
                memcpy(ledColors, cop2, sizeof(cop2));
                refresh_eyes();
                pause(400);
                }                
                set_neopixel_group(0xFFFFFF);
                memcpy(ledmask, nmask1, sizeof(nmask1));
                refresh_eyes();
               dprint(term,"cop");
					}
     #ifdef STRING
     		  if (strcmp(inputString, "u") == 0) {
             can_position += 50;
             if (can_position >= 900)
             {can_position = 900;
             
					}
             servo_angle(2,can_position);
            }     
     
          	if (strcmp(inputString, "d") == 0) {
            can_position -= 50;
             if (can_position <= 0)
             {can_position = 0;
					}
            servo_angle(2,can_position);
            }    
             
            if (strcmp(inputString, "s") == 0) {
              if(CNT>=firing_timer)
              {
               dprint(term,"fire");
               fire_flag = 1;
               firing_timer = CNT+firing_interval;
               firing_duration_timer=CNT;
               servo_angle(1,500);
              }               
              
            }              
     #endif
          		if (strncmp(inputString, "led",3) == 0) 
              { 
               char * pBeg = &inputString;
               char * pEnd;
               uint8_t pixel = strtol(pBeg+4, &pEnd,10);
               uint32_t color = strtol(pEnd, &pEnd,16);
               dprint(term,"%d\n",color);
               if((pixel < LED_COUNT)&&(color<=0xFFFFFF))
               set_neopixel(pixel,color); 
              }			
              
              	if (strncmp(inputString, "leds",4) == 0) 
              { 
               char * pBeg = &inputString;
               char * pEnd;
               uint32_t color = strtol(pBeg+5, &pEnd,16);
               dprint(term,"%d\n",color);
               if((color<=0xFFFFFF))
               set_neopixel_group(color); 
              }					
					sPos = 0;
					inputString[0] = 0; // clear string
				} else if (sPos < inputStringLength - 1) {
					// record next character
					inputString[sPos] = c;
					sPos += 1;
					inputString[sPos] = 0; // make sure last element of string is 0
					dprint(term, inputString);
					dprint(term, " ok \n");
				}  
			}            
		}      
	}   
}
void Step(int leftSpeed, int rightSpeed)
{
	drive_speed(leftSpeed, rightSpeed);
}  

void set_motor_controller(int leftSpeed, int rightSpeed)
{

	current_leftspd =leftSpeed;
	current_rightspd = rightSpeed;
	motor_flag = 1;
}  

void Stop(void)
{
	//drive_feedback(0);
  //drive_close();
  drive_speed(0, 0);

}  

void led_blink()                            // Blink function for other cog
{
	while(1)                              // Endless loop for other cog
	{
		high(26);                           // P26 LED on
		pause(1000);                         // ...for 0.1 seconds
		low(26);                            // P26 LED off
		pause(1000);                         // ...for 0.1 seconds
	}
}

void motor_controller()
{
	uint32_t last_ms = 0;
	uint32_t current_ms = 0;
	uint32_t wait_ms = 10;
	uint32_t clk_wait = 80000*wait_ms;
	uint32_t timeout_timer = 0;
	uint32_t timeout_ms = 80000*500;
 
	while(1)
	{

		current_ms = CNT;  
		if(current_ms-last_ms >= clk_wait )
		{            
			last_ms = current_ms;
			if(motor_flag == 1) 
			{
				Step(current_leftspd,current_rightspd);
				motor_flag =0;
				timeout_timer = current_ms;
			}
			if(current_ms-timeout_timer >= timeout_ms)
			{
				Stop();
			}   
   
        #ifdef STRING
        if(fire_flag == 1)
        {
        if(current_ms - firing_duration_timer >= firing_duration)
        {
          servo_angle(16,0);
          fire_flag = 0;
        }       
      }           
        #endif       
		}        
	}  
}


void pause(int ms)
{
    waitcnt(CNT + ms * ticks_per_ms);
}

void set_neopixel_group(uint32_t color)
{
        int i;
        for (i = 0; i < LED_COUNT; ++i)
        {
          ledColors[i] = color;
        }   
        refresh_eyes();
      
}

void refresh_eyes()
{
        int i2;
        
        for (i2 = 0; i2 < LED_COUNT; ++i2)
        {
          if (ledmask[i2] == 1)
          {
          uint32_t red = (ledColors[i2]>>16) & 0xFF;
          red = red*brightness/255;
          uint32_t green = (ledColors[i2]>>8) & 0xFF;
          green = green*brightness/255;
          uint32_t blue = (ledColors[i2]) & 0xFF;
          blue = blue*brightness/255;
          uint32_t scaled_color = (red << 16)+(green << 8)+(blue);
          dim_array[i2] = scaled_color;   
          }
          else
          {
          dim_array[i2] = 0x000000;
          }                         
        }   
        
        ws2812_refresh(&driver, LED_PIN, dim_array, LED_COUNT);
}  

void increase_brightness()
{
  int16_t temp_brightness = brightness+10;
  if(temp_brightness>255)
  {
  temp_brightness = 255;
  } 
  brightness = temp_brightness;
  refresh_eyes();
}  

void decrease_brightness()
{
  int16_t temp_brightness = brightness-10;
  if(temp_brightness<=1)
  {
  temp_brightness = 2;
  } 
  brightness = temp_brightness;
  refresh_eyes();
} 

void set_neopixel(uint8_t pixel_num, uint32_t color)
{
        if(pixel_num <LED_COUNT)
        {
          ledColors[pixel_num] = color;
        }          
           refresh_eyes();
}


void eyes_blink()
{
  memcpy(ledmask, nmask1, sizeof(nmask1));
  refresh_eyes();
  pause(500);
  memcpy(ledmask, tmask, sizeof(tmask));
  refresh_eyes();
  pause(500);
  memcpy(ledmask, nmask1, sizeof(nmask1));
  refresh_eyes(); 
           pause(1);
                   
}  