Alan(Xutong) Zhao	1430631
Yue Ma			1434071

	
Acknowledgement: I received help from Yu Zhu.
Acknowledgement: I used some funcitons that our professor(Junfeng Wen)
		developed in class, as well as functions on eClass


Accessories:
* Arduino Mega 2560 Board
* USB 2.0 A-B cable
* Wires
* Joystick
* LCD screen
* 560 Ohm (Grn Blu Brown) resistor
* LEDs
* Potentiometer


Wiring instructions:
*Power 5V to BB Power bus
*Power GND to BB GND bus
LCD screen: 
	*GND to BB GND bus
	*VCC to BB positive bus
	*RESET to Pin 8
	*D/C (Data/Command) to Pin 7
	*CARD_CS (Card Chip Select) to Pin 5
	*TFT_CS (TFT/screen Chip Select) to Pin 6
	*MOSI (Master Out Slave In) to Pin 51
	*SCK (Clock) to Pin 52
	*MISO (Master In Slave Out) to 50
	*LITE (Backlite) to BB positive bus
Joystick:
	*VCC to BB positive bus
	*VERT to Pin A0
	*HOR to Pin A1
	*SEL to Pin 9
	*GND to BB GND bus 
Potentiometer:
	*middle pin (slider) to Pin A2
	*outer pins: one to BB GND bus, the other one to BB Power bus
LEDs:
	* Arduino Pin 2 <--> longer LED lead |LED| Shorter LED lead  <--> Resistor <--> Arduino GND Bus
	* Arduino Pin 3 <--> longer LED lead |LED| Shorter LED lead  <--> Resistor <--> Arduino GND Bus
	* Arduino Pin 4 <--> longer LED lead |LED| Shorter LED lead  <--> Resistor <--> Arduino GND Bus
	* Arduino Pin 10 <--> longer LED lead |LED| Shorter LED lead <--> Resistor <--> Arduino GND Bus
	* Arduino Pin 11 <--> longer LED lead |LED| Shorter LED lead <--> Resistor <--> Arduino GND Bus
 


Code description:
	For the Part II, three major improvement are added to the Part I--- implementation of quickSort, scrollable restaurant list and rating selector.

Sorting an array of integers in increasing order using quickSort can be done by the following steps: 
	Step 1: Pick a pivot. It can be done in a randomized way, or 			deterministic way (like the middle element of the 			array).
	Step 2: Partition the whole array according to their relations 			to the pivot. After such partition, you should have 			something like this: { elements <= pivot, pivot, 			elements > pivot }
	Step 3: Assign the two sub-arrays to two quick sort workers.

In order to partition the whole array, we need to do following steps in the partition() function: 
	1. Put away the pivot by swapping it with the last element of 			the array.
	2. Search from both ends to out-of-order elements in the 			subarray array[0:len-2] by repeating:
		2.1 the low-scanner searches for first high element 				from the beginning of the array but do not go 				beyond the high-scanner's position
		2.2 the high-scanner searches for first low element 				from the end of the array but do not go beyond 				the low-scanner's position
		2.3 swap if found a pair of eligible elements, move 				scanners
	3. Put the pivot back to its final position, return the final 			position.

For Part I, we only need to display as many restaurants as will fit in the 20 llines of the LCD screen (by default, the screen can only fit 20 lines). For this part, however, we're supposed to use the joystick to scroll through more than the restaurants that can be displayed on one screen. To do this, an array is created to store the list of restaurant. Every time the cursor moves down when it is already at the bottom of the screen, the list is updated so that the next 20 nearest restaurants are displayed. If the cursor moves up when it is already at the top of the screen, the list is updated in the opposite way. Here's one thing we need to pay attention: we need to update the whole list only if the cursor is about to move "out of the page".

In this part we also need to add a potentiometer to select the minimum rating of restaurants displayed. Since the rating is stored on the scale 0-10, we assume the natural conversion to a 5 point scale for x ∈ {0,1,2,...,10} : s = floor((x+1)/2).

To display what minimum rating is currently selected, five LEDs are used to display the current rating. The number of LEDs that are turned on indicates the minimum star rating we currently choose. If we connect the middle pin of the potentiometer to an analog input pin, we are able to read the voltage on the pin as an integer value that varies from 0 (at 0V) to 1023 (at 5V). Therefore, we can divide this range (0~1023) into 6 equal intervals, each of which corresponds to a star rating (0,1,2,3,4,5), so that we are able to choose the rating based on the value from the potentiometer. Thus, only restaurants with rating that is equal to or above the rating value from the potentiometer are displayed on the LCD screen. Notice: the rating needs to be set in the enterMapMode(). You can still change the rating using potentiometer in enterListMode(0 but the displayed list will not change. 


