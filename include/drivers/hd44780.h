#pragma once
#include <cstdint>
#include <string>

//mode 4 pins pour le moment
template <typename RS, typename ENABLE, typename D4, typename D5, typename D6, typename D7, uint8_t lineNumber, uint8_t colSize>
	class HD44780 {
	public:

        HD44780() {
        }

		 void init() {
			
			RS::setOutput();
			ENABLE::setOutput();
			D4::setOutput();
			D5::setOutput();
			D6::setOutput();
			D7::setOutput();
			
			os_delay_us(650000);
			
			RS::clear();
			ENABLE::clear();
			
			// Only for 4 bits mode
			Write(0x03);
			os_delay_us(4500);
			Write(0x03);
			os_delay_us(4500);
			Write(0x03);
			os_delay_us(150);
			Write(0x02);
			
			
			uint8_t functionSet = 0;
			if (lineNumber > 1)
			{
				functionSet |=Display_Function::D_TWO_LINES;
			}
			if ((lineNumber == 1) && (colSize > 10))
			{
				functionSet |=Display_Function::D_MATRICE_5_11;
			}
			IssueCommand(Mode::FUNCTION_SET, functionSet);
            Clear();
            SetDirection(true, false);
            SetDisplay(true, true, false);
		}
		
		 void SetDisplay(bool display, bool visibleCursor, bool blinkCursor)
		{
			if (display)
			{
                currentDisplay |= Display_Control::DISPLAY_ON;
            }
            else {
                currentDisplay &= ~Display_Control::DISPLAY_ON;
            }
			if (visibleCursor)
			{
                currentDisplay |= Display_Control::CURSOR_VISIBLE;
			}
            else {
                currentDisplay &= ~Display_Control::CURSOR_VISIBLE;
            }
			if (blinkCursor)
			{
                currentDisplay |= Display_Control::BLINK_CURSOR;
            }
            else {
                currentDisplay |= Display_Control::BLINK_CURSOR;
            }
			IssueCommand(Commands::DISPLAY_CONTROL, currentDisplay);
		}

         void ShowText() {
             currentDisplay |= Display_Control::DISPLAY_ON;
             IssueCommand(Commands::DISPLAY_CONTROL, currentDisplay);
         }

         void HideText() {
             currentDisplay &= ~Display_Control::DISPLAY_ON;
             IssueCommand(Commands::DISPLAY_CONTROL, currentDisplay);
         }

         void ShowCursor() {
             currentDisplay |= Display_Control::CURSOR_VISIBLE;
             IssueCommand(Commands::DISPLAY_CONTROL, currentDisplay);
         }

         void HideCursor() {
             currentDisplay &= ~Display_Control::CURSOR_VISIBLE;
             IssueCommand(Commands::DISPLAY_CONTROL, currentDisplay);
         }

         void BlinkCursor() {
             currentDisplay |= Display_Control::BLINK_CURSOR;
             IssueCommand(Commands::DISPLAY_CONTROL, currentDisplay);
         }

         void StaticCursor() {
             currentDisplay &= ~Display_Control::BLINK_CURSOR;
             IssueCommand(Commands::DISPLAY_CONTROL, currentDisplay);
         }
		
		 void Clear() {
			IssueCommand(Commands::CLEAR_DISPLAY);
            currentCol = 0;
            currentRow = 0;
			os_delay_us(2000);
		}

         void Home() {
             IssueCommand(Commands::RETURN_HOME);
             currentCol = 0;
             currentRow = 0;
             os_delay_us(2000);
         }

         void SetDirection(bool writeRight,bool shiftAdress) {
            uint8_t args = 0;
            if (writeRight) {
                args |= Display_Mode::DIRECTION;
            }
            if (shiftAdress) {
                args |=Display_Mode::SHIFT;
            }
           
            IssueCommand(Commands::ENTRY_MODE_SET, args);
        }

         void DisplayDigit(char character) {
             if (currentCol >= colSize) {
                 ChangeRow();
             }
             RS::set();
             Write(character);
             currentCol++;
        }

         void DisplayDigit(uint8_t colNumber, uint8_t rowNumber, char character) {
             SetCursor(colNumber, rowNumber);
             RS::set();
             Write(character);
             currentCol++;
         }

         void SetCursor(uint8_t colNumber, uint8_t rowNumber) {
             uint8_t args = 0;
             if (rowNumber > 0) {
                 args = row_addresses[rowNumber];
             }
             args += colNumber;
             IssueCommand(Mode::SET_DDRAMADDR, args);
             currentCol = colNumber;
             currentRow = rowNumber;
         }
		
	private:
		enum Commands:uint8_t {
			CLEAR_DISPLAY   = 0x01,
			RETURN_HOME     = 0x02,
			ENTRY_MODE_SET  = 0x04,
			DISPLAY_CONTROL = 0x08
			
		};
		
		enum Mode:uint8_t {
			FUNCTION_SET    = 0x20,
			SET_CGRAMADDR   = 0x40,
			SET_DDRAMADDR	= 0x80
		};
		
		enum Display_Function:uint8_t
		{
			D_8_BITS = 0x10,
			D_TWO_LINES = 0x08,
			D_MATRICE_5_11 = 0x04
			
		};
		
		enum Display_Control:uint8_t
		{
			DISPLAY_ON     = 0x04,
			CURSOR_VISIBLE = 0x02,
			BLINK_CURSOR   = 0x01
			
		};

        enum Display_Mode :uint8_t
        {
            DIRECTION = 0x02,
            SHIFT = 0x01
        };
		

        uint8_t currentCol = 0;
        uint8_t currentRow = 0;
        uint8_t currentDisplay = 0;
        uint8_t row_addresses[lineNumber];

        void ChangeRow() {
            if ((currentRow+1) < lineNumber) {
                SetCursor(currentRow++, currentCol);
            }
            else {
                SetCursor(0, 0);
            }
        }

        void InitAddresses() {
            for (auto i = 0; i < lineNumber; i++) {
                row_addresses[i] = colSize * (i / 2) + 0x40 * (i % 2);
            }
        }

		 void IssueCommand(Mode mode, uint8_t arg) {
            RS::clear();
			Write(mode | arg);
		}
		
		 void IssueCommand(Commands command) {
            RS::clear();
			Write(command);
		}
		
		 void IssueCommand(Commands command,uint8_t arg) {
            RS::clear();
			Write(command | arg);
		}

		 void Write(uint8_t data) {
			D4::set(((1 << 4)&data) != 0 );
			D5::set(((1 << 5)&data) != 0);
			D6::set(((1 << 6)&data) != 0);
			D7::set(((1 << 7)&data) != 0);
			//ENABLE::pulseHigh();
			PulseEnable();
			D4::set(((1 << 0)&data) != 0);
			D5::set(((1 << 1)&data) != 0);
			D6::set(((1 << 2)&data) != 0);
			D7::set(((1 << 3)&data) != 0);
			//ENABLE::pulseHigh();
			PulseEnable();
		}
		
		 void PulseEnable() {
			ENABLE::clear();
			os_delay_us(1);  
			ENABLE::set();
			os_delay_us(1);  
			ENABLE::clear();
			os_delay_us(100);
		}
	};

    template <typename HD44780>
    class HD44780Printer {
    public:
        static void print(const char str[]) {
            HD44780 h;
            for (auto i = 0; str[i] != '\0'; i++) {
                h.DisplayDigit(str[i]);
            }
        }
    };
