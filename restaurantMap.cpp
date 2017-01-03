/**
	Alan(Xutong) Zhao	1430631
	Yue Ma				1434071
	Section: EA1
	
	Acknowledgement: I received help from Yu Zhu.
	Acknowledgement: I used some funcitons that our professor(Junfeng Wen)
					developed in class, as well as functions on eClass.
	
	Note to TA: The position of the cursor reative to the map can be traced using serial monitor.
				cursor_x_image, cursor_y_iamge and SEL are printed to the serial monitor.
				
	Note to TA: Sometimes overflow occurs after several cases are tested 
			(e.g.	1. the program is not able to switch from enterListMode() back to 
						enterMapMode() if the program has switched between these two modes 
						back and forth for several times.
				or	2. the path of the cursor is displayed after switching from enterListMode()
						back to enterMapMode if the program has switched between these two modes 
						back and forth for several times.)
				
				The position of the cursor can still be traced even if one of these two cases happened.
				
				EVERYTHING else works properly. 
				
				Please press the Reset button on Arduino board if one of the two cases mentioned above 
				happens and you still want to test more cases. I apologize for the inconvenience. 
	
	Note to TA: Please move the joystick after the list of restaurants has been displayed on the screen.
				
				
			
			
*/

/** ========================= Initialization =========================*/
#include <Arduino.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7735.h> // Hardware-specific library
#include <SPI.h>
#include <SD.h>
#include "lcd_image.h"       


/** standard U of A library settings, assuming Atmel Mega SPI pins */
#define SD_CS    5  // Chip select line for SD card
#define TFT_CS   6  // Chip select line for TFT display
#define TFT_DC   7  // Data/command line for TFT
#define TFT_RST  8  // Reset line for TFT (or connect to +5V)

/** Just like "tft" for screen, we have "card" for SD
	Make it global so that every function can use it */
Sd2Card card;

// LCD Screen
Adafruit_ST7735 tft = Adafruit_ST7735(TFT_CS, TFT_DC, TFT_RST);

// Joystick Pins
const int VERT = 0;		// analog input
const int HORIZ = 1;	// analog input
const int SEL = 9;		// digital input

// setup LED and potentiometer pins
// didn't copy professor's code
int ledPin1 = 2;		// LED1 is attached to pin 2
int ledPin2 = 3;		// LED1 is attached to pin 3
int ledPin3 = 4;		// LED1 is attached to pin 4
int ledPin4 = 10;		// LED1 is attached to pin 10
int ledPin5 = 11; 		// LED1 is attached to pin 11
int analogInPin = 2;	// slider of potentiometer attached to analog input pin A2
/** ====================== Initialization ends =======================*/

/** ========== Constants, new data types and other variables =========*/
const uint32_t RESTAURANT_START_BLOCK = 4000000;
const int NUM_RESTAURANTS = 1066;

int cursor_width = 3;                           // width of the cursor
int initMapPosX = 680;							// init horiz map starting position
int initMapPosY = 600;							// init verti map starting position
int old_cursor_x_image, old_cursor_y_image;     // position wrt map from previous iteraiton
int cursor_x_lcd = 64;							// init horiz coordi wrt lcd
int cursor_y_lcd = 80;       					// init verti coordi wrt lcd
int cursor_x_image = initMapPosX + cursor_x_lcd;        // init horiz coordi wrt map
int cursor_y_image = initMapPosY + cursor_y_lcd;        // init verti coordi wrt map
int lcd_x = 128, lcd_y = 160;                   // dimension of lcd 
int old_cursor_x_lcd, old_cursor_y_lcd;         // position wrt lcd from previous iteraiton


/** These constants are for the 2048 by 2048 map. */
const int16_t map_width = 2048;
const int16_t map_height = 2048;
const int32_t lat_north = 5361858;
const int32_t lat_south = 5340953;
const int32_t lon_west = -11368652;
const int32_t lon_east = -11333496;

// map of Edmonton
lcd_image_t map_image = { "yeg-big.lcd", map_width, map_height };

// Global variable for Restaurant list
char rest_list[20][55];
int selection, old_selection;

/** new data type: restaurant, RestDist*/
typedef struct {
	int32_t lat;		// Stored in 1/100,000 degrees
	int32_t lon;		// Stored in 1/100,000 degrees
	uint8_t rating;	// 0-10: 2 = 1 star, 10 = 5 stars
	char name[55];	// always null terminated
} restaurant;
restaurant r;

typedef struct {
	uint16_t index;     // index of restaurant from 0 to NUM_RESTAURANTS-1
	uint16_t dist;      // Manhatten distance to cursor position
} RestDist;
RestDist rest_dist[NUM_RESTAURANTS];

int ratingChoose;
/** ======= Constants, new data types and other variables ends =======*/


/** ==================== Functions for various uses ==================*/

/** Declaration of global variables (declared outside a function) */  
uint32_t previousBlock = RESTAURANT_START_BLOCK;	// previousBlock = 4000000
restaurant buffer[8];	
/** Avoid starting from the very beginning for each iteration
	Only improve the case where consecutive calls to get_restaurant are from the same block */
void get_restaurant_fast(restaurant* ptr_r, int i) {
	uint32_t block; 
	block = RESTAURANT_START_BLOCK + i/8 ; 

	if (previousBlock != block) { 
		previousBlock = block;	// previousBlock to be the block in this iteration
		while (!card.readBlock(block, (uint8_t*)buffer)) {
			Serial.println("Read block failed! Trying again.");
		}
	}
	// special case i==0 (the very first block)
	else if (i == 0){
		while (!card.readBlock(block, (uint8_t*)buffer)) {
			Serial.println("Read block failed! Trying again.");
		}
	}
	else {}

	*ptr_r = buffer[i%8]; 
}

/** Swap two restaurants of RestDist struct */
void swap_rest(RestDist *ptr_rest1, RestDist *ptr_rest2) {
	RestDist tmp = *ptr_rest1;
	*ptr_rest1 = *ptr_rest2;
	*ptr_rest2 = tmp;
}

// pick_pivot of an array
int pick_pivot(int len) {
	int pivotIdx = (len-1)/2;
	return pivotIdx;
}

int partition( RestDist* array, int len, int pivot_idx ) {
	int pivot = array[pivot_idx].dist;
	swap_rest(&array[pivot_idx], &array[len-1]);
	int i = 0;      // low-scanner
	int j = len-2;  // high-scanner
	while (i <= j) {
		while (array[i].dist < pivot){
			i++;
		}
		while (array[j].dist > pivot) {
			j--;
		}
		if (i <= j) {
			swap_rest(&array[i], &array[j]);
			i++;
			j--;
		}   
	}
	swap_rest(&array[i], &array[len-1]);

	return i;
}

// qsort eclass version
void qsort(RestDist* array, int len) {
    if (len > 1) {
		int pivot_idx = pick_pivot(len);
		pivot_idx = partition( array, len, pivot_idx );
		qsort( array, pivot_idx );
		qsort( array+pivot_idx+1, len-pivot_idx-1 );
    }  
    else {
		return;
  }
}

/** Print restaurant names to the LCD screen*/
void print_rest(int start, int end) {
	tft.fillScreen(0x0000);	// clear the screen with black
	tft.setCursor(0,0);		// set the cursor, indicates where to display
	tft.setTextWrap(false);	// no wrap

	// print  restaurant names 
	for (int i = start; i <= end; i++){
		// highlight selected text
		if (i == selection){	
			tft.setTextColor(0x0000, 0xFFFF); // black char on white bg
		} 
		// do not highlight text if it is not the selction
		else { 
			tft.setTextColor(0xFFFF, 0x0000); // white char on black bg
		}
		tft.print(rest_list[i]);
		tft.print('\n');
	}
}

/** Set curcor to the current text position (highlight the text) */
void update_rest() {
	// replace/cover old selection text
	tft.setTextColor(0xFFFF, 0x0000);			// white char on black bg
	tft.setCursor(0, (old_selection % 20)*8);	// set cursor to the position of old selection
	tft.print(rest_list[old_selection % 20]);

	// print new selection text
	tft.setTextColor(0x0000, 0xFFFF);			// black char on white bg
	tft.setCursor(0, (selection % 20)*8);		// set cursor to the new selection line
	tft.print(rest_list[selection % 20]);
}

/** Update the restaurant list */
void update_rest_list(){
	
	int tmp = selection;

	// check if user scrolls the joystick up or down
	if (selection % 20 == 19){
		selection -= 19;
	}

	// Store next closest 20 restaurants into an array 
	for (int i = 0; i < 20; i++) {
		get_restaurant_fast(&r,rest_dist[selection].index);
		strcpy(rest_list[i],r.name); 
		selection += 1;
	}

	selection = tmp;		// the selected position

	print_rest(0,19);		// print the next closest 20 restaurant list  
}


/** These functions convert between x/y map position and lat/lon
	(and vice versa.) */
int32_t x_to_lon(int16_t x) {
    return map(x, 0, map_width, lon_west, lon_east);
}
int32_t y_to_lat(int16_t y) {
    return map(y, 0, map_height, lat_north, lat_south);
}
int16_t lon_to_x(int32_t lon) {
    return map(lon, lon_west, lon_east, 0, map_width);
}
int16_t lat_to_y(int32_t lat) {
    return map(lat, lat_north, lat_south, 0, map_height);
}

/** configure LED pins to be digital outputs */
void setupLED() {
	pinMode(ledPin1, OUTPUT);
	pinMode(ledPin2, OUTPUT);
	pinMode(ledPin3, OUTPUT);
	pinMode(ledPin4, OUTPUT);
	pinMode(ledPin5, OUTPUT);
}
    
    
/** read the voltage on the analog input and convert into an integer value in the range [0, 1023]
	divide the range into 6 equal intervals to determine which LED is turned on */
int voltInterv = 1023/6; 

void ledOnOff(int volt1, int volt2, int volt3, int volt4, int volt5) {
	digitalWrite(ledPin1, volt1);
	digitalWrite(ledPin2, volt2);
	digitalWrite(ledPin3, volt3);
	digitalWrite(ledPin4, volt4);
	digitalWrite(ledPin5, volt5);
}

void controlLED() {
	if (analogRead(analogInPin) <= voltInterv) {	// star rating is 0
		ledOnOff(LOW, LOW, LOW, LOW, LOW);
		ratingChoose = 0;
	}
	else if (analogRead(analogInPin) > voltInterv && analogRead(analogInPin) <= 2*voltInterv) {	// star rating is 1
		ledOnOff(HIGH, LOW, LOW, LOW, LOW);
		ratingChoose = 1;
	}
	else if (analogRead(analogInPin) > 2*voltInterv && analogRead(analogInPin) <= 3*voltInterv) {	// star rating is 2
		ledOnOff(HIGH, HIGH, LOW, LOW, LOW);
		ratingChoose = 2;
	}
	else if (analogRead(analogInPin) > 3*voltInterv && analogRead(analogInPin) <= 4*voltInterv) {	// star rating is 3
		ledOnOff(HIGH, HIGH, HIGH, LOW, LOW);
		ratingChoose = 3;
	}
	else if (analogRead(analogInPin) > 4*voltInterv && analogRead(analogInPin) <= 5*voltInterv) {	// star rating is 4
		ledOnOff(HIGH, HIGH, HIGH, HIGH, LOW);
		ratingChoose = 4;
	}
	else if (analogRead(analogInPin) > 5*voltInterv) {	// star rating is 5
		ledOnOff(HIGH, HIGH, HIGH, HIGH, HIGH);
		ratingChoose = 5; 
	}	 
}

/** restrict specifically LCD and Map boundary */
void LCDMapBound() {
	// LCD left edge
	if ( cursor_x_lcd < 0 ){
		// Map left edge
		if (cursor_x_image <= lcd_x) {
			// redraw a portion of the small Edmonton Map
			lcd_image_draw(&map_image, &tft, 0, (cursor_y_image-cursor_y_lcd), 0, 0, lcd_x, lcd_y); 
			cursor_x_lcd = cursor_x_image;
		}
		else {
			cursor_x_lcd = lcd_x-cursor_width;
			// redraw a portion of the small Edmonton Map
			lcd_image_draw(&map_image, &tft, (cursor_x_image-lcd_x), (cursor_y_image-cursor_y_lcd), 0, 0, lcd_x, lcd_y); 
			cursor_x_image -= cursor_width; 
		}
	}
	// LCD right edge
	else if ( cursor_x_lcd > (lcd_x-cursor_width) ){
		// Map right edge
		if ((map_width - cursor_x_image) <= lcd_x) {
			// redraw a portion of the small Edmonton Map
			lcd_image_draw(&map_image, &tft, (map_width - lcd_x), (cursor_y_image-cursor_y_lcd), 0, 0, lcd_x, lcd_y); 
			cursor_x_lcd = cursor_x_image - (map_width - lcd_x);
		}
		else {
			cursor_x_lcd = 0;
			// redraw a portion of the small Edmonton Map
			lcd_image_draw(&map_image, &tft, (cursor_x_image+cursor_width),(cursor_y_image-cursor_y_lcd),0,0,lcd_x,lcd_y);
			cursor_x_image += cursor_width; 
		}
	}
	// LCD top edge
	if ( cursor_y_lcd < 0 ){
		// Map top edge
		if (cursor_y_image <= lcd_y) {
			// redraw a portion of the small Edmonton Map
			lcd_image_draw(&map_image, &tft, (cursor_x_image-cursor_x_lcd), 0, 0, 0, lcd_x, lcd_y); 
			cursor_y_lcd = cursor_y_image;
		}
		else {
			cursor_y_lcd = lcd_y-cursor_width;
			// redraw a portion of the small Edmonton Map
			lcd_image_draw(&map_image, &tft, (cursor_x_image-cursor_x_lcd), (cursor_y_image-lcd_y),0,0,lcd_x,lcd_y);
			cursor_y_image -= cursor_width;
		}
	}
	// LCD bottom edge
	else if (cursor_y_lcd > (lcd_y-cursor_width) ){
		// Map bottom edge
		if ((map_height - cursor_y_image) <= lcd_y) {
			// redraw a portion of the small Edmonton Map
			lcd_image_draw(&map_image, &tft, (cursor_x_image-cursor_x_lcd), (map_height - lcd_y), 0, 0, lcd_x, lcd_y); 
			cursor_y_lcd = cursor_y_image - (map_height - lcd_y);
		}
		else {
			cursor_y_lcd = 0;
			// redraw a portion of the small Edmonton Map
			lcd_image_draw(&map_image, &tft, (cursor_x_image-cursor_x_lcd),(cursor_y_image+cursor_width),0,0,lcd_x,lcd_y);
			cursor_y_image += cursor_width;
		}
	}
}

/** discuss two special boundary cases about displaying the selected restaurant */
void twoRestBound() {
	/** case 1: If the selected restaurant is close to the map boundary, 
	then the cursor does not have to be placed at the center of the screen. 
	case 2: If the selected restaurant is not in the range of the map, 
	then the cursor should be placed on the map location that is closest to the selected restaurant.	 
	*/

	// case 1 for x
	if( cursor_x_image <= (0.5 * lcd_x) && cursor_x_image >= 0) {
		cursor_x_lcd = cursor_x_image;
	}
	else if ((map_width - cursor_x_image) <= (0.5*lcd_x) && cursor_x_image <= (map_width - cursor_width)) {
		cursor_x_lcd = lcd_x - (map_width - cursor_x_image);
	}
	// case 2 for x
	else if (cursor_x_image < 0) {
		cursor_x_lcd = 0;
	}
	else if (cursor_x_image > (map_width - cursor_width)) {
		cursor_x_lcd = lcd_x-cursor_width;
	}
	// general case for x
	else {
		cursor_x_lcd = 64;
	}

	// case 1 for y
	if( cursor_y_image <= (0.5 * lcd_y) && cursor_y_image >= 0) {
		cursor_y_lcd = cursor_y_image;
	}
	else if ((map_height - cursor_y_image) <= (0.5*lcd_y) && cursor_y_image <= (map_height - cursor_width)) {
		cursor_y_lcd = lcd_y - (map_height - cursor_y_image);
	}
	// case 2 for y
	else if (cursor_y_image < 0) {
		cursor_y_lcd = 0;
	}
	else if (cursor_y_image > (map_height - cursor_width)) {
		cursor_y_lcd = lcd_y-cursor_width;
	}
	// general case for y
	else {
		cursor_y_lcd = 80;
	}
}

/** ================= Functions for various uses ends ================*/

/** ==================== Loop two major functions ====================*/
/** Fucntion forward declaration*/
void enterListMode();

/** ----------------------- enterMapMode function---------------------*/
void enterMapMode() {
	int init_vert, init_horiz; 
	init_horiz = analogRead(HORIZ);      
	init_vert = analogRead(VERT);
	
	while(true){
		// Turn on/off LEDs
		controlLED();
		
		old_cursor_x_lcd = cursor_x_lcd;
		old_cursor_y_lcd = cursor_y_lcd;
		old_cursor_x_image = cursor_x_image;
		old_cursor_y_image = cursor_y_image;

		int vertical, horizontal, select;
		horizontal = analogRead(HORIZ);		// will be 0-1023
		vertical= analogRead(VERT);			// will be 0-1023
		select = digitalRead(SEL);			// HIGH(1) if not pressed
											// LOW(0) if pressed
		
		int delta_vert, delta_horiz;
		delta_vert = vertical - init_vert;		// verti change 
		delta_horiz = horizontal - init_horiz;	// horiz change 


		cursor_x_lcd = cursor_x_lcd + delta_horiz/100;  
		cursor_y_lcd = cursor_y_lcd + delta_vert/100;  
		cursor_x_image = cursor_x_image + delta_horiz/100;
		cursor_y_image = cursor_y_image + delta_vert/100;

		// draw a tiny patch of the image, replacing the old cursor
		if (cursor_x_image != old_cursor_x_image || cursor_y_image != old_cursor_y_image){
			lcd_image_draw(&map_image, &tft, old_cursor_x_image, old_cursor_y_image, old_cursor_x_lcd, old_cursor_y_lcd, cursor_width, cursor_width );
		} 

		// Map Boundary in general: using constrain function to limit the range of cursor values
		cursor_x_image = constrain(cursor_x_image, 0, map_width - cursor_width);
		cursor_y_image = constrain(cursor_y_image, 0, map_height - cursor_width);

		// LCD and Map Boundary specifically
		LCDMapBound();

		// draw the new cursor
		tft.fillRect(cursor_x_lcd, cursor_y_lcd, cursor_width, cursor_width, 0xFFE0);

		// test: print out the values of coordinates to serial-mon
		Serial.print(" cursor_x_image: ");
		Serial.print(cursor_x_image);
		Serial.print(" cursor_y_image: ");
		Serial.print(cursor_y_image);
		Serial.print(" SEL= ");
		Serial.print(select,DEC);
		Serial.println();
		delay(50);
		
		// enterListMode if joystick is pressed
		if (select == 0){
			delay(50);
			enterListMode(); 
		}

	}
	
}
/** -------------------- enterMapMode function ends-------------------*/


/** ---------------------- enterListMode function---------------------*/
void enterListMode() {
	int init_vert_listMode, init_horiz_listMode;
	int select, vertical_listMode;
	select = digitalRead(SEL);       
	
	// store the restaurant index and Manhatten distance to the cursor into struct array rest_dist
	int j = 0;
	for (int i = 0; i < NUM_RESTAURANTS; i++) {
		get_restaurant_fast(&r, i);
		//int convertRating = floor((r.rating + 1)/2);
		if (floor((r.rating + 1)/2) >= ratingChoose) {
			rest_dist[j].index = i;
			
			// Manhatten distance
			int differenceX = lon_to_x(r.lon) - cursor_x_image;
			int differenceY = lat_to_y(r.lat) - cursor_y_image;
			rest_dist[j].dist = abs(differenceX) + abs(differenceY);
			j++;
		}
	}

	// quick sort 20 nearest restaurants 
	qsort(rest_dist, j);

	
	for (int i = 0; i < 20; i++) {
		get_restaurant_fast(&r,rest_dist[i].index);
		strcpy(rest_list[i],r.name);
	}
	selection = 0;			// selected rest
	print_rest(0,19);

	delay(500);

	init_vert_listMode = analogRead(VERT);

	while(true){
		// turn on/off LEDs
		controlLED();
		

		vertical_listMode = analogRead(VERT);     
		select = digitalRead(SEL);            
		old_selection = selection;

		if (select == 0){
			delay(200);
			get_restaurant_fast(&r, rest_dist[selection].index);
			
			cursor_x_image = lon_to_x(r.lon);
			cursor_y_image = lat_to_y(r.lat);
			
			// two boundary cases about displaying the selected restaurant 
			twoRestBound();
			
			lcd_image_draw(&map_image, &tft, cursor_x_image-cursor_x_lcd, cursor_y_image-cursor_y_lcd, 0, 0, lcd_x, lcd_y);
			
			// draw the new cursor at the position of the restaurant selected
			tft.fillRect(cursor_x_lcd, cursor_y_lcd, cursor_width, cursor_width, 0xFFE0);
			enterMapMode();
			break;
		}
		if ((vertical_listMode - init_vert_listMode)<0){
			selection -= 1;
			if (selection % 20 == 19) {
				update_rest_list();
			}
			else {
				update_rest();
			}
		}
		else if ((vertical_listMode - init_vert_listMode)>0){
			selection += 1;
			if (selection % 20 == 0 && selection != 0) {
				update_rest_list();
			}
			else {
				update_rest();
			}
		}
		else{
			;  
		}
		
		if (selection < 0){
			selection = 0;
		}
		
		delay(200);        
	}
}
/** ------------------- enterListMode function ends-------------------*/


/** ================= Loop two major functions ends ==================*/

/** ========================= Main function ==========================*/
int main (){
	init();
	Serial.begin(9600);


	pinMode(SEL, INPUT);
	digitalWrite(SEL, HIGH);

	tft.initR(INITR_BLACKTAB);    // initialize a ST7735R chip

	// Initializing SD card
	Serial.print("Initializing SD card...");
	if (!SD.begin(SD_CS)) {
		Serial.println("failed!");
		while(true){};              // hang on, something is wrong
	}
	else {
		Serial.println("OK!");
	}
	
	// Some more initialization
	Serial.print("Doing raw initialization...");
	if (!card.init(SPI_HALF_SPEED, SD_CS)) {
		Serial.println("failed!");
		while(true) {} // something is wrong    
	} 
	else {
		Serial.println("OK!");
	}
	
	// initializing LEDs
	setupLED();

	// draw a portion of the small Edmonton Map 
	lcd_image_draw(&map_image, &tft, initMapPosX, initMapPosY, 0, 0, lcd_x, lcd_y);

	enterMapMode();

	return 0;
}
/** ======================= Main function ends =======================*/

