1. Download and install USB driver for ST-LINK: http://www.st.com/en/development-tools/stsw-link009.html
1. Open the Arduino IDE
1. `File` > `Preferences`
  a. Add `https://raw.githubusercontent.com/stm32duino/BoardManagerFiles/master/STM32/package_stm_index.json` to Additional Board Manager URLs
  a. (Optional) Check verbose compilation and upload
  a. (Optional) Compiler warnings `All`
1. `Tools` > `Board` > `Board Manager...`
1. Select `STM32 Cores by ST-Microelectronics` then `Install` then `Close`
1. `Tools` > `Board` > `Nucleo 64`
1. `Tools` > `Board Part Number` > `Nucleo F103RB`
1. `Tools` > `Upload Method` > `STLink`
1. `Tools` > `Port` > Select a COM port
1. Upload the following sketch to test (first time will take a while to compile)

		void setup() {
		  Serial.begin(9600);
		}

		void loop() {
		  Serial.println("Hello");
		  delay(1000);
		}

1. Open serial port monitor. You should see "Hello" printed once a second
