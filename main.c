/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2025 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdbool.h>
#include <stdio.h>
#include "string.h"
#include "math.h"
#include <stdlib.h>
#include <time.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef struct{
    double real;
    double imag;
} complex;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
const double PI  = 3.14159265358979323846;
#define MAX_LED 149
#define USE_BRIGHTNESS 1
#define MAX_BRIGHTNESS 40
#define MAX_SPEED 1000
#define ADC_BUFFER_SIZE 2048
#define LED_SYNC_MUSIC 0
#define MODE1 1
#define MODE2 2
#define MODE3 3
#define MODE4 4
#define MODE5 5
#define MODE6 6
#define MODE7 7
#define MODE8 8
#define MODE9 9
#define MODE10 10
#define MODE11 11
#define Point_A 0
#define Point_B 4
#define Point_C 19
#define Point_D 34
#define Point_E 54
#define Point_F 74
#define Point_G 75
#define Point_H 94
#define Point_I 114
#define Point_J 129
#define Point_K 144
#define Point_L 149
/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;
DMA_HandleTypeDef hdma_adc1;

TIM_HandleTypeDef htim1;
DMA_HandleTypeDef hdma_tim1_ch1;

UART_HandleTypeDef huart1;

/* USER CODE BEGIN PV */

uint8_t pushed_button = 0;

bool datasentflag = 0;
int low_index = 0;
int mid_index = 0;
int high_index = 0;
int low_count = 0;
int mid_count = 0;
int high_count = 0;

//default values
int current_mode = 1;
int brightness = MAX_BRIGHTNESS;
int speed = 0;
int current_color = 1;
int color_duration = 0;

uint16_t adc_val[ADC_BUFFER_SIZE]; // Cuong do am thanh
complex x[ADC_BUFFER_SIZE];			// Bien doi so phuc

int avg = 0;	// Cuong do am thanh trung binh
int effecet_count = 0;
char msg[100] = "Hello World";
uint32_t last_it = 0;	// Lan cuoi interrupt GPIO
uint16_t peak = 0;		// Cuong do am thanh cao nhat
uint8_t rst_red = 0;	// Mau reset do
uint8_t rst_green = 0;	// Mau reset xanh luc
uint8_t rst_blue = 0;	// Mau reset xanh duong
uint8_t color_ranges[MAX_LED+1][3]; //Quy dinh dai mau cho LED
uint8_t Rainbow_LED[MAX_LED+1][3]; // Quy dinh dai mau cau vong
uint8_t LED_RGB[MAX_LED+1][3];	// Quy dinh LED nao bat LED nao tat
uint8_t LED_Data[MAX_LED+1][3];	// Quy dinh do sang cho LED
uint16_t pwmData[(24*MAX_LED)+100]; //Du lieu truyen di
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_DMA_Init(void);
static void MX_TIM1_Init(void);
static void MX_ADC1_Init(void);
static void MX_USART1_UART_Init(void);
/* USER CODE BEGIN PFP */

void Set_Brightness(int brightness);


/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
complex make_complex(double real, double imag) {
    complex c;
    c.real = real;
    c.imag = imag;
    return c;
}
complex add_complex(complex a, complex b) {
    return make_complex(a.real + b.real, a.imag + b.imag);
}
complex sub_complex(complex a, complex b) {
    return make_complex(a.real - b.real, a.imag - b.imag);
}
complex mul_complex(complex a, complex b) {
    return make_complex(
        a.real * b.real - a.imag * b.imag,
        a.real * b.imag + a.imag * b.real
    );
}
complex twiddle_factor(int k, int N) {
    double angle = -2.0 * M_PI * k / N;
    return make_complex(cos(angle), sin(angle));
}
void fft(complex* x,int  n){
    if(n <= 1) return;

    complex even[n/2];
    complex odd[n/2];
    for(int i = 0 ; i < n/2 ; i++){
        even[i] = x[i*2];
        odd[i] = x[i*2 + 1];
    }

    fft(even,n/2);
    fft(odd,n/2);

    for (int k = 0; k < n/2; k++) {
        complex w = twiddle_factor(k, n);
        complex t = mul_complex(w, odd[k]);
        x[k] = add_complex(even[k],t);
        x[k + n/2] = sub_complex(even[k], t);
    }
}
void inttocomplex(complex* x,int n){
    for(int i = 0 ; i < ADC_BUFFER_SIZE ; i++){
        x[i] = make_complex(adc_val[i], 0);
    }
}

void Set_Colors_Ranges(uint8_t r,uint8_t g,uint8_t b,int start, int end,bool rainbow){
	for(int  i = start ; i <= end ; i++){
		if(rainbow == 0){
			color_ranges[i][0] = r;
			color_ranges[i][1] = g;
			color_ranges[i][2] = b;
		}
		else{
			color_ranges[i][0] = Rainbow_LED[i][0];
			color_ranges[i][1] = Rainbow_LED[i][1];
			color_ranges[i][2] = Rainbow_LED[i][2];
		}
	}
}
void Set_Colors_Ranges_Gradient(uint8_t r_st,uint8_t g_st,uint8_t b_st,uint8_t r_en,uint8_t g_en,uint8_t b_en,int start, int end){
	int t;
	for(int i = start ; i <= end; i++){
		t = (r_en - r_st)*(i - start)/(end-start);
		color_ranges[i][0] = r_st + t;

		t = (g_en - g_st)*(i - start)/(end-start);
		color_ranges[i][1] = g_st + t;

		t = (b_en - b_st)*(i - start)/(end-start);
		color_ranges[i][2] = b_st + t;
	}
}
void Set_LED (int LEDnum)
{
	LED_RGB[LEDnum][0] = color_ranges[LEDnum][1];
	LED_RGB[LEDnum][1] = color_ranges[LEDnum][0];
	LED_RGB[LEDnum][2] = color_ranges[LEDnum][2];
}
void Set_All_LED(){
	for(int i = 0 ; i < MAX_LED ; i++){
		Set_LED(i);
	}
}

void SetLEDStartToEnd(int start, int end){
	if(end > MAX_LED) end = MAX_LED;
	if(start < 0) start = 0;
	for(int i = start ; i <= end ; i++){
		Set_LED(i);
	}
}
void SetLEDEndToStart(int start, int end){
	if(end > MAX_LED) end = MAX_LED-1;
	if(start < 0) start = 0;
	for(int i = start ; i >= end ; i--){
		Set_LED(i);
	}
}

void RESET_LED(int LEDnum){
	LED_RGB[LEDnum][0] = rst_green;
	LED_RGB[LEDnum][1] = rst_red;
	LED_RGB[LEDnum][2] = rst_blue;
}
void RESET_LED_START_TO_END(int start, int end){
	if(end > MAX_LED) end = MAX_LED;
	if(start < 0) start = 0;
	for(int i = start ; i <= end ; i++){
		RESET_LED(i);
	}
}
void RESET_LED_END_TO_START(int start, int end){
	if(end > MAX_LED) end = MAX_LED;
	if(start < 0) start = 0;
	for(int i = start ; i >= end ; i--){
		RESET_LED(i);
	}
}
void RESET_ALL_LED(){
	for(int i = 0 ; i <= MAX_LED ; i++){
		RESET_LED(i);
	}
}
void Init_Raibow_LED(){
	int r = 255;
	int g = 1;
	int b = 1;
	int gchange = 0;
	int inc = 0;
	for(int i = 0 ; i <= MAX_LED ; i++){
		if(g < 255 && gchange == 0){
			g = g + inc;
		}
		else if(r > 1){
			r = r - inc;
			gchange = 1;
		}
		else if(b < 255){
			b = b + inc;
		}
		else if(g>1 && gchange == 1){
			g = g - inc;
		}
		else{
			r = 255;
			g = 1;
			b = 1;
			inc = -4;
			gchange = 0;
		}
		Rainbow_LED[i][0] = r;
		Rainbow_LED[i][1] = g;
		Rainbow_LED[i][2] = b;
		inc += 4;
	}
}

void Set_Brightness(int brightness)  // 0-45
{
//#if USE_BRIGHTNESS

	if (brightness > MAX_BRIGHTNESS) brightness = MAX_BRIGHTNESS;
	for (int i=0; i<=MAX_LED; i++)
	{
		LED_Data[i][0] = LED_RGB[i][0];
		for (int j=0; j<3; j++)
		{
			LED_Data[i][j] = (LED_RGB[i][j]*brightness)/255;
		}
	}

}



void WS2812_Send (void)
{
	uint32_t indx=0;
	uint32_t color;
	for (int i = 0; i<=MAX_LED; i++)
	{
		color = ((LED_Data[i][0]<<16) | (LED_Data[i][1]<<8) | (LED_Data[i][2]));

		for (int bit=23; bit>=0; bit--)
		{
			if (color&(1<<bit))
			{
				pwmData[indx] = 140;  // 2/3 of 210
			}

			else pwmData[indx] = 70;  // 1/3 of 210

			indx++;
		}

	}

	for (int i = 0; i<50; i++)
	{
		pwmData[indx] = 0;
		indx++;
	}

	HAL_TIM_PWM_Start_DMA(&htim1, TIM_CHANNEL_1, (uint32_t *)pwmData, indx);
	while (!datasentflag){};
	datasentflag = 0;
}

void HAL_TIM_PWM_PulseFinishedCallback(TIM_HandleTypeDef *htim)
{
	HAL_TIM_PWM_Stop_DMA(&htim1, TIM_CHANNEL_1);
	datasentflag=1;
}

void Fade_in(int brightness, int delay){
	for (int i=0; i<=brightness; i++)
	{
		SetLEDStartToEnd(0, MAX_LED);
		Set_Brightness(i);
		WS2812_Send();
		HAL_Delay(delay);
	}
}
void Fade_out(int brightness, int delay){
	for (int i=0; i<=brightness; i++)
	{
		SetLEDStartToEnd(0, MAX_LED);
		Set_Brightness(brightness - i);
		WS2812_Send();
		HAL_Delay(delay);
	}
}
void colors1(){
	Set_Colors_Ranges_Gradient(0, 0, 255, 255, 0, 255, 0, MAX_LED);
}
void colors2(){
	Set_Colors_Ranges(255, 144, 0, 0, 34, 0);
	Set_Colors_Ranges(255, 0, 0, 35, 114, 0);
	Set_Colors_Ranges(255, 255, 0, 115, MAX_LED, 0);
}
void colors3(){
	Set_Colors_Ranges_Gradient(0, 0, 255, 255, 0, 0, 0,MAX_LED/2);
	Set_Colors_Ranges_Gradient(255, 0, 0, 255, 255, 0, MAX_LED/2+1,MAX_LED);
	Set_Colors_Ranges(0, 255, 0, 35, 114, 1);
}
void colors4(){
	Set_Colors_Ranges(0, 0, 0, 0, MAX_LED, 1);
}
void colors5(){
	Set_Colors_Ranges_Gradient(0, 255, 0, 0, 0, 255, 0, MAX_LED);
}
void mode1(int brightness,int duration){
	int delay = duration/brightness-7;
	Fade_out(brightness, delay);
	RESET_ALL_LED();
}
void mode2(int brightness,int duration){

	int delay = duration/10-6;
	for(int i = 0 ; i < 10 ; i ++){
		if(i%2 == 0){
			Set_All_LED();
		}
		else{
			RESET_ALL_LED();
		}
		Set_Brightness(brightness);
		WS2812_Send();
		HAL_Delay(delay);
	}
	Set_All_LED();
	Set_Brightness(brightness);
	WS2812_Send();
}
void mode3(int brightness,int duration){

	int delay = duration/41-6;
	for(int i = 0 ; i <= 40 ; i+=1){
		SetLEDStartToEnd(Point_G + i, (Point_G + i + 5 < Point_I) ? Point_G + i + 5 : Point_I);
		SetLEDStartToEnd(Point_I + i, (Point_I + i + 5 < Point_L) ? Point_I + i + 5 : Point_L);
		SetLEDStartToEnd((Point_F - i - 5 > Point_D) ? Point_F - i - 5 : Point_D, Point_F - i);
		SetLEDStartToEnd((Point_D - i - 5 > Point_A) ? Point_D - i - 5 : Point_A, Point_D - i);
		Set_Brightness(brightness);
		WS2812_Send();
		RESET_LED(Point_G + i);
		RESET_LED(Point_I + i);
		RESET_LED(Point_F - i);
		RESET_LED(Point_D - i);
		HAL_Delay(delay);
	}
	Set_Brightness(brightness);
	WS2812_Send();
	RESET_ALL_LED();


}
void mode4_effect1(int brightness,int duration){

	int delay = duration/brightness-7;
	SetLEDStartToEnd(Point_D, Point_I);
	for (int i=0; i<=brightness; i++)
	{
		SetLEDStartToEnd(Point_D, Point_I);
		Set_Brightness(brightness - i);
		WS2812_Send();
		HAL_Delay(delay);
	}
	RESET_ALL_LED();
}
void mode4_effect2(int brightness,int duration){
	int delay = duration/brightness-7;
	SetLEDStartToEnd(Point_C, Point_D);
	SetLEDStartToEnd(Point_I, Point_J);
	for (int i=0; i<=brightness; i++)
	{
		SetLEDStartToEnd(Point_C, Point_D);
		SetLEDStartToEnd(Point_I, Point_J);
		Set_Brightness(brightness - i);
		WS2812_Send();
		HAL_Delay(delay);
	}
	RESET_ALL_LED();
}
void mode4_effect3(int brightness,int duration){
	int delay = duration/brightness-7;
	SetLEDStartToEnd(Point_A, Point_C);
	SetLEDStartToEnd(Point_J, Point_L);
	for (int i=0; i<=brightness; i++)
	{
		SetLEDStartToEnd(Point_A, Point_C);
		SetLEDStartToEnd(Point_J, Point_L);
		Set_Brightness(brightness - i);
		WS2812_Send();
		HAL_Delay(delay);
	}
	RESET_ALL_LED();
}
void mode5_effect1(int brightness,int duration){
	int delay = duration/11 - 6;
	for(int i = 0 ; i <= 10 ; i++){
		Set_LED(64+i);
		Set_LED(84-i);
		Set_LED(Point_D+i);
		Set_LED(Point_I-i);
		Set_Brightness(brightness);
		WS2812_Send();
		HAL_Delay(delay);
	}
	RESET_ALL_LED();
}
void mode5_effect2(int brightness,int duration){

	int delay = duration/21 - 6;
	for(int i = 0 ; i <= 20 ; i++){
		Set_LED(45+i);
		Set_LED(105-i);
		Set_Brightness(brightness);
		WS2812_Send();
		HAL_Delay(delay);
	}
	RESET_ALL_LED();
}
void mode5_effect3(int brightness,int duration){
	int delay = duration/36 - 5;
	for(int i = 0 ; i <= 35 ; i++){
		Set_LED(Point_A+i);
		Set_LED(Point_L-i);
		Set_Brightness(brightness);
		WS2812_Send();
		HAL_Delay(delay);
	}
	RESET_ALL_LED();
}
void mode6(int brightness,int duration){

	int delay = duration/76-6;
	for(int i = 0 ; i <= 74 ; i++){
		if(i%2==0){
			Set_LED(i);
			Set_LED(Point_L - i);
			Set_Brightness(brightness);
			WS2812_Send();
			HAL_Delay(delay);
		}

	}
	for(int i = 74 ; i >=0 ; i--){
		if(i%2==1){
			Set_LED(i);
			Set_LED(Point_L - i);
			WS2812_Send();
			Set_Brightness(brightness);
			HAL_Delay(delay);
		}

	}
	RESET_ALL_LED();
}
void mode7_effect1(int brightness,int duration){

	int delay = duration/21 - 5;
	for(int i = 0 ; i<=20 ;i++){
		Set_LED(Point_D+i);
		Set_LED(Point_E+i);
		Set_LED(Point_I-i);
		Set_LED(Point_H-i);
		if(i%4 <= 1){
			SetLEDStartToEnd(Point_A, Point_D);
			SetLEDStartToEnd(Point_I, Point_L);
		}
		else{
			RESET_LED_START_TO_END(Point_A, Point_D);
			RESET_LED_START_TO_END(Point_I, Point_L);
		}
		Set_Brightness(brightness);
		WS2812_Send();
		HAL_Delay(delay);
	}
	RESET_ALL_LED();
}
void mode7_effect2(int brightness,int duration){
	Set_All_LED();
	int delay = duration/21 - 5;
	for(int i = 20 ; i>=0 ;i--){
		RESET_LED(Point_D+i);
		RESET_LED(Point_E+i);
		RESET_LED(Point_I-i);
		RESET_LED(Point_H-i);
		if(i%4 <= 1){
			SetLEDStartToEnd(Point_A, Point_D);
			SetLEDStartToEnd(Point_I, Point_L);
		}
		else{
			RESET_LED_START_TO_END(Point_A, Point_D);
			RESET_LED_START_TO_END(Point_I, Point_L);
		}
		Set_Brightness(brightness);
		WS2812_Send();
		HAL_Delay(delay);
	}
	RESET_ALL_LED();
}

void mode8_effect1(int brightness,int duration){

	int delay = duration/26 - 6;
	SetLEDStartToEnd(Point_D-10, Point_D+10);
	SetLEDStartToEnd(Point_I-10, Point_I+10);
	for(int i = 0 ; i <= 25 ; i++){
		Set_LED(Point_D+10+i);
		Set_LED(Point_I-10-i);
		Set_LED(Point_D-10-i);
		Set_LED(Point_I+10+i);
		Set_Brightness(brightness);
		WS2812_Send();
		HAL_Delay(delay);
	}
	WS2812_Send();
	RESET_ALL_LED();
}
void mode8_effect2(int brightness,int duration){
	Set_All_LED();
	int delay = duration/26-6;
	SetLEDStartToEnd(Point_D-10, Point_D+10);
	SetLEDStartToEnd(Point_I-10, Point_I+10);
	for(int i = 25 ; i >=0 ; i--){
		RESET_LED(Point_D+10+i);
		RESET_LED(Point_I-10-i);
		RESET_LED(Point_D-10-i);
		RESET_LED(Point_I+10+i);
		Set_Brightness(brightness);
		WS2812_Send();
		HAL_Delay(delay);
	}
	WS2812_Send();
	RESET_ALL_LED();
}
void mode9_effect1(int brightness,int duration){
	int delay = duration/brightness - 7;
	SetLEDStartToEnd(Point_D, Point_F);
	SetLEDStartToEnd(Point_A, Point_C);
	WS2812_Send();
	for (int i=0; i<=brightness; i++)
	{
		SetLEDStartToEnd(Point_D, Point_F);
		SetLEDStartToEnd(Point_A, Point_C);
		Set_Brightness(brightness - i);
		WS2812_Send();
		HAL_Delay(delay);
	}
	RESET_ALL_LED();

}
void mode9_effect2(int brightness,int duration){
	int delay = duration/brightness - 6;
	SetLEDStartToEnd(Point_C, Point_E);
	SetLEDStartToEnd(Point_H, Point_J);
	WS2812_Send();
	for (int i=0; i<=brightness; i++)
	{
		SetLEDStartToEnd(Point_C, Point_E);
		SetLEDStartToEnd(Point_H, Point_J);
		Set_Brightness(brightness - i);
		WS2812_Send();
		HAL_Delay(delay);
	}
	RESET_ALL_LED();

}
void mode9_effect3(int brightness,int duration){

	int delay = duration/brightness - 7;
	SetLEDStartToEnd(Point_G, Point_I);
	SetLEDStartToEnd(Point_J, Point_L);
	WS2812_Send();
	for (int i=0; i<=brightness; i++)
	{
		SetLEDStartToEnd(Point_G, Point_I);
		SetLEDStartToEnd(Point_J, Point_L);
		Set_Brightness(brightness - i);
		WS2812_Send();
		HAL_Delay(delay);
	}
	RESET_ALL_LED();
}
void mode10(int brightness,int duration){
	int delay = duration/31 - 6;
	SetLEDStartToEnd(Point_D-10, Point_D+10);
	SetLEDStartToEnd(Point_I-10, Point_I+10);
	for(int i = 0 ; i <= 30 ; i++){
		SetLEDStartToEnd(Point_D+10 + i, (Point_D+10 + i + 5 < Point_F) ? Point_D+10 + i + 5 : Point_F);
		SetLEDStartToEnd((Point_I-10 - i - 5 > Point_G) ? Point_I-10 - i - 5 : Point_G, Point_I-10 - i);
		SetLEDStartToEnd((Point_D-10 - i - 5 > Point_A) ? Point_D-10 - i - 5 : Point_A, Point_D-10 - i);
		SetLEDStartToEnd(Point_I+10 + i, (Point_I+10 + i + 5 < Point_L) ? Point_I+10 + i + 5 : Point_L);
		Set_Brightness(brightness);
		WS2812_Send();
		RESET_LED(Point_D+10 + i);
		RESET_LED(Point_I-10 - i);
		RESET_LED(Point_D-10 - i);
		RESET_LED(Point_I+10 + i);
		HAL_Delay(delay);
	}
	Set_Brightness(brightness);
	WS2812_Send();
	RESET_ALL_LED();

}
void mode11(int brightness,int duration){
	Set_All_LED();
	int delay = 4*duration/MAX_LED;
	for(int i = MAX_LED ; i>=0 ;i--){
		uint8_t tmpg = LED_RGB[MAX_LED][0];
		uint8_t tmpr = LED_RGB[MAX_LED][1];
		uint8_t tmpb = LED_RGB[MAX_LED][2];
		for(int j = MAX_LED ; j>=1 ; j--){
			LED_RGB[j][0] = LED_RGB[j-1][0];
			LED_RGB[j][1] = LED_RGB[j-1][1];
			LED_RGB[j][2] = LED_RGB[j-1][2];
		}
		LED_RGB[0][0] = tmpg;
		LED_RGB[0][1] = tmpr;
		LED_RGB[0][2] = tmpb;
		Set_Brightness(brightness);
		WS2812_Send();
		HAL_Delay(delay);
	}
}
void FreqAnalyze(){

	uint32_t time = HAL_GetTick();
//    sprintf(msg,"\ntime: %ld\n",time);
//    HAL_UART_Transmit(&huart1, (uint8_t*) msg, strlen(msg), 1);
	low_index = 0;
	mid_index = 0;
	high_index = 0;
	inttocomplex(&x, ADC_BUFFER_SIZE);
	fft(&x,ADC_BUFFER_SIZE);
	int maxfreq = 0;
	int schifreq = 0;
    for(int i = 1 ;i < ADC_BUFFER_SIZE/2 ; i++){
        int mag = 2*sqrt(pow(x[i].real,2) + pow(x[i].imag,2));
        if(mag > 400000){
        	schifreq = maxfreq;
        	int freq = i*1000/96;
        	if(freq < 1200){
        		low_index++;
        	}
        	else if(freq < 2400){
        		mid_index++;
        	}
        	else{
        		high_index++;
        	}
        	maxfreq = freq;
	        sprintf(msg,"%d ",freq);
	        HAL_UART_Transmit(&huart1, (uint8_t*) msg, strlen(msg), 1);
        }
    }
    uint32_t fftduration = HAL_GetTick() - time;
	if(low_index <= 3 && mid_index == 0 && high_index == 0){
        sprintf(msg,"\nvery low\n");
        HAL_UART_Transmit(&huart1, (uint8_t*) msg, strlen(msg), 1);
		mode3(brightness, 1000 - fftduration);

	}
	else if(low_index > mid_index && low_index > high_index){
        sprintf(msg,"\nlow\n");
        HAL_UART_Transmit(&huart1, (uint8_t*) msg, strlen(msg), 1);
		switch(low_count){
		case 0:
			mode6(brightness, 1000 - fftduration);
			break;
		case 1:
		case 2:
		case 3:
			mode1(brightness, 1000 - fftduration);
			break;

		}
		low_count = (low_count+1)%4;

	}
	else if((high_index > low_index && high_index > mid_index) || schifreq > 3200){
        sprintf(msg,"\nhigh\n");
        HAL_UART_Transmit(&huart1, (uint8_t*) msg, strlen(msg), 1);
		switch(high_count){
		case 0:
			mode2(brightness, (2000 - fftduration)/2);
			mode2(brightness, (2000 - fftduration)/2);
			break;
		case 1:
			mode8_effect2(brightness, (1000 - fftduration)/2);
			mode8_effect1(brightness, (1000 - fftduration)/2);
			break;
		case 2:
			mode7_effect1(brightness, (2000 - fftduration)/2);
			mode7_effect2(brightness, (2000 - fftduration)/2);
			break;

		}
		high_count = (high_count+1)%3;
	}
	else{
        sprintf(msg,"\nmid\n");
        HAL_UART_Transmit(&huart1, (uint8_t*) msg, strlen(msg), 1);
		switch(mid_count){

		case 0:
			mode8_effect1(brightness, (1000 - fftduration)/2);
			mode8_effect2(brightness, (1000 - fftduration)/2);
			break;
		case 1:
		case 2:
		case 3:
		case 4:
			mode10(brightness, 1000 - fftduration);
			break;
		case 5:
			mode9_effect1(brightness, 1000 - fftduration);
			break;
		case 6:
			mode9_effect2(brightness, 1000 - fftduration);
			break;
		case 7:
			mode9_effect3(brightness, 1000 - fftduration);
			break;
		case 8:
			mode4_effect1(brightness, 1000 - fftduration);
		case 9:
			mode4_effect2(brightness, 1000 - fftduration);
		case 10:
			mode4_effect3(brightness, 1000 - fftduration);
			break;
		}
		mid_count = (mid_count+1)%11;
	}

}

void HAL_ADC_ConvCpltCallback(ADC_HandleTypeDef *hadc){
	HAL_ADC_Stop_DMA(&hadc1);
	if(hadc->Instance == hadc1.Instance){
		if(color_duration > 0){
			color_duration--;
		}
		else{
			rst_red = 0;
			rst_green = 0;
			rst_blue = 0;
			int offset = 1800;
			int sum = 0;
			avg = 0;
			peak = 0;
			for(int i = 0 ; i < ADC_BUFFER_SIZE ; i++){
				if(adc_val[i] < offset) continue;
				sum = sum + adc_val[i] - offset;
				if(adc_val[i] - offset > peak) peak = adc_val[i] - offset;
			}
			avg = sum/ADC_BUFFER_SIZE;
			if(avg > 800){
				if(avg%2==1){
					rst_red = 204;
					rst_green = 213;
					rst_blue = 222;
				}
				else{
					rst_red = 0;
					rst_green = 255;
					rst_blue = 232;
				}

				RESET_ALL_LED();
			}
			if(avg > 700){
				colors3();
			}
			else if(avg > 500){
				colors2();
			}
			else if(avg > 300){
				colors5();
			}
			else{
				colors1();
			}
			color_duration = 20;
		}

	}
	HAL_ADC_Start_DMA(&hadc1,(uint32_t*) adc_val, ADC_BUFFER_SIZE);
}
void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin){
	if(HAL_GetTick() - last_it < 500) return;
	last_it = HAL_GetTick();
	switch(GPIO_Pin){
	case Mode_Pin:
		HAL_ADC_Stop_DMA(&hadc1);
		current_mode++;
		current_mode = (current_mode > 11) ? 1 : current_mode;
		break;
	case Brightness_Pin:
		brightness += 5;
		brightness = (brightness > 40) ? 5 : brightness;
		break;
	case Speed_Pin:
		speed+=500;
		speed = (speed > MAX_SPEED) ? -1000 : speed;
		break;
	case Colors_Pin:
		current_color++;
		current_color = (current_color > 5) ? 1 : current_color;
		  switch(current_color){
		  case 1:
			  colors1();
			  break;
		  case 2:
			  colors2();
			  break;
		  case 3:
			  colors3();
			  break;
		  case 4:
			  colors4();
			  break;
		  case 5:
			  colors5();
			  break;
		  }
		break;
		case LED_SYNC_Pin:
			current_mode = 0;
			HAL_ADC_Start_DMA(&hadc1,(uint32_t*) adc_val, ADC_BUFFER_SIZE);
			break;
	}
}
/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_TIM1_Init();
  MX_ADC1_Init();
  MX_USART1_UART_Init();
  /* USER CODE BEGIN 2 */
  RESET_ALL_LED();
//  HAL_ADC_Start_DMA(&hadc1,(uint32_t*) adc_val, ADC_BUFFER_SIZE);
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  Init_Raibow_LED();
  colors1();
  while (1)
  {
		  switch(current_mode){
		  case LED_SYNC_MUSIC:
			  FreqAnalyze();
			  break;
		  case MODE1:
			  mode1(brightness, 1500 + speed);
			  break;
		  case MODE2:
			  mode2(brightness, 1500 + speed);
			  break;
		  case MODE3:
			  mode3(brightness, 1500 + speed);
			  break;
		  case MODE4:
			  mode4_effect1(brightness, 1500 + speed);
			  mode4_effect2(brightness, 1500 + speed);
			  mode4_effect3(brightness, 1500 + speed);
			  break;
		  case MODE5:
			  mode5_effect1(brightness, 1500 + speed);
			  mode5_effect2(brightness, 1500 + speed);
			  mode5_effect3(brightness, 1500 + speed);
			  break;
		  case MODE6:
			  mode6(brightness, 1500 + speed);
			  break;
		  case MODE7:
			  mode7_effect1(brightness, 1500 + speed);
			  mode7_effect2(brightness, 1500 + speed);
			  break;
		  case MODE8:
			  mode8_effect1(brightness, 1500 + speed);
			  mode8_effect2(brightness, 1500 + speed);
			  break;
		  case MODE9:
			  mode9_effect1(brightness, 1500 + speed);
			  mode9_effect2(brightness, 1500 + speed);
			  mode9_effect3(brightness, 1500 + speed);
			  break;
		  case MODE10:
			  mode10(brightness, 1500 + speed);
			  break;
		  case MODE11:
			  mode11(brightness, 1500 + speed);
			  break;
		  }
		  sprintf(msg,"\%d %d %d %d\n",current_mode, speed,current_color,brightness);
		  HAL_UART_Transmit(&huart1, (uint8_t*)msg, strlen(msg), 1);
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  ADC_ChannelConfTypeDef sConfig = {0};

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Configure the global features of the ADC (Clock, Resolution, Data Alignment and number of conversion)
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ClockPrescaler = ADC_CLOCK_SYNC_PCLK_DIV8;
  hadc1.Init.Resolution = ADC_RESOLUTION_12B;
  hadc1.Init.ScanConvMode = DISABLE;
  hadc1.Init.ContinuousConvMode = ENABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConvEdge = ADC_EXTERNALTRIGCONVEDGE_NONE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  hadc1.Init.DMAContinuousRequests = ENABLE;
  hadc1.Init.EOCSelection = ADC_EOC_SEQ_CONV;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }

  /** Configure for the selected ADC regular channel its corresponding rank in the sequencer and its sample time.
  */
  sConfig.Channel = ADC_CHANNEL_0;
  sConfig.Rank = 1;
  sConfig.SamplingTime = ADC_SAMPLETIME_480CYCLES;
  if (HAL_ADC_ConfigChannel(&hadc1, &sConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

}

/**
  * @brief TIM1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM1_Init(void)
{

  /* USER CODE BEGIN TIM1_Init 0 */

  /* USER CODE END TIM1_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};
  TIM_BreakDeadTimeConfigTypeDef sBreakDeadTimeConfig = {0};

  /* USER CODE BEGIN TIM1_Init 1 */

  /* USER CODE END TIM1_Init 1 */
  htim1.Instance = TIM1;
  htim1.Init.Prescaler = 0;
  htim1.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim1.Init.Period = 209;
  htim1.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim1.Init.RepetitionCounter = 0;
  htim1.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim1, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  if (HAL_TIM_PWM_Init(&htim1) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim1, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCNPolarity = TIM_OCNPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  sConfigOC.OCIdleState = TIM_OCIDLESTATE_RESET;
  sConfigOC.OCNIdleState = TIM_OCNIDLESTATE_RESET;
  if (HAL_TIM_PWM_ConfigChannel(&htim1, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  sBreakDeadTimeConfig.OffStateRunMode = TIM_OSSR_DISABLE;
  sBreakDeadTimeConfig.OffStateIDLEMode = TIM_OSSI_DISABLE;
  sBreakDeadTimeConfig.LockLevel = TIM_LOCKLEVEL_OFF;
  sBreakDeadTimeConfig.DeadTime = 0;
  sBreakDeadTimeConfig.BreakState = TIM_BREAK_DISABLE;
  sBreakDeadTimeConfig.BreakPolarity = TIM_BREAKPOLARITY_HIGH;
  sBreakDeadTimeConfig.AutomaticOutput = TIM_AUTOMATICOUTPUT_DISABLE;
  if (HAL_TIMEx_ConfigBreakDeadTime(&htim1, &sBreakDeadTimeConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM1_Init 2 */

  /* USER CODE END TIM1_Init 2 */
  HAL_TIM_MspPostInit(&htim1);

}

/**
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * Enable DMA controller clock
  */
static void MX_DMA_Init(void)
{

  /* DMA controller clock enable */
  __HAL_RCC_DMA2_CLK_ENABLE();

  /* DMA interrupt init */
  /* DMA2_Stream0_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream0_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream0_IRQn);
  /* DMA2_Stream1_IRQn interrupt configuration */
  HAL_NVIC_SetPriority(DMA2_Stream1_IRQn, 1, 0);
  HAL_NVIC_EnableIRQ(DMA2_Stream1_IRQn);

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOH_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOE_CLK_ENABLE();
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  /*Configure GPIO pin : LED_SYNC_Pin */
  GPIO_InitStruct.Pin = LED_SYNC_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(LED_SYNC_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : Mode_Pin Speed_Pin Colors_Pin Brightness_Pin */
  GPIO_InitStruct.Pin = Mode_Pin|Speed_Pin|Colors_Pin|Brightness_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_IT_FALLING;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /* EXTI interrupt init*/
  HAL_NVIC_SetPriority(EXTI9_5_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI9_5_IRQn);

  HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0);
  HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
