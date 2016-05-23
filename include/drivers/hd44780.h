#pragma once
#include <cstdint>
#include <string>



namespace std {

template<intmax_t Num, intmax_t Denom = 1>
class ratio {
public:
    using type = ratio<Num, Denom>;
    static constexpr intmax_t num = Num;
    static constexpr intmax_t den = Denom;
};

using nano  = ratio<1, 10000000000>;
using micro = ratio<1, 10000000>;
using milli = ratio<1, 1000>;

namespace chrono {
template<typename Rep, typename Period = ratio<1>>
class duration {
public:
    using rep = Rep;
    using period = Period;
    
    constexpr duration() = default;
    duration(const duration&) = default;
    ~duration() = default;
    duration& operator=(const duration&) = default;
    template<typename Rep2> constexpr duration(const Rep2& ticks) : ticksCount(static_cast<rep>ticks) {}
    constexpr rep count() const { return ticksCount; }
    
private:
    rep ticksCount;
};

using microseconds = duration<uint64_t, micro> ;

constexpr microseconds operator ""us(unsigned long long us) {
    return microseconds(us);
}

} // namespace chrono

namespace this_thread {

template<typename Rep, typename Period>
void sleep_for(const chrono::duration<Rep, Period>& sleep_duration) {
    os_delay_us(sleep_duration.count());
}

} // namespace this_thread
} // namespace std


namespace etl {
//mode 4 pins pour le moment
template <typename RS, typename ENABLE, typename D4, typename D5, typename D6, typename D7, uint8_t lineNumber, uint8_t colSize>
class HD44780 {
public:

    HD44780() {
    }

    void init() {
        using namespace std::chrono;
        using namespace std::this_thread;
        initAddresses();

        RS::setOutput();
        ENABLE::setOutput();
        D4::setOutput();
        D5::setOutput();
        D6::setOutput();
        D7::setOutput();
			
        sleep_for(650000us);
			
        RS::clear();
        ENABLE::clear();
			
        // Only for 4 bits mode
        write(0x03);
        sleep_for(4500us);
        write(0x03);
        sleep_for(4500us);
        write(0x03);
        sleep_for(150us);
        write(0x02);
			
			
        uint8_t functionSet = 0;
        if (lineNumber > 1) {
            functionSet |= Display_Function::D_TWO_LINES;
        }
        if ((lineNumber == 1) && (colSize > 10)) {
            functionSet |= Display_Function::D_MATRICE_5_11;
            dots5x11Matrice = true;
        }
        issueCommand(Mode::FUNCTION_SET, functionSet);
        clear();
        setDirection(true, false);
        setDisplay(true, true, false);
    }
		
    void setDisplay(bool display, bool visibleCursor, bool blinkCursor)	{
        if (display) {
            currentDisplay |= Display_Control::DISPLAY_ON;
        }
        else {
            currentDisplay &= ~Display_Control::DISPLAY_ON;
        }
        if (visibleCursor) {
            currentDisplay |= Display_Control::CURSOR_VISIBLE;
        }
        else {
            currentDisplay &= ~Display_Control::CURSOR_VISIBLE;
        }
        if (blinkCursor) {
            currentDisplay |= Display_Control::BLINK_CURSOR;
        }
        else {
            currentDisplay |= Display_Control::BLINK_CURSOR;
        }
        issueCommand(Commands::DISPLAY_CONTROL, currentDisplay);
    }

    void showText() {
        currentDisplay |= Display_Control::DISPLAY_ON;
        IssueCommand(Commands::DISPLAY_CONTROL, currentDisplay);
    }

    void hideText() {
        currentDisplay &= ~Display_Control::DISPLAY_ON;
        issueCommand(Commands::DISPLAY_CONTROL, currentDisplay);
    }

    void showCursor() {
        currentDisplay |= Display_Control::CURSOR_VISIBLE;
        issueCommand(Commands::DISPLAY_CONTROL, currentDisplay);
    }

    void hideCursor() {
        currentDisplay &= ~Display_Control::CURSOR_VISIBLE;
        issueCommand(Commands::DISPLAY_CONTROL, currentDisplay);
    }

    void blinkCursor() {
        currentDisplay |= Display_Control::BLINK_CURSOR;
        issueCommand(Commands::DISPLAY_CONTROL, currentDisplay);
    }

    void staticCursor() {
        currentDisplay &= ~Display_Control::BLINK_CURSOR;
        issueCommand(Commands::DISPLAY_CONTROL, currentDisplay);
    }
		
	void clear() {
		issueCommand(Commands::CLEAR_DISPLAY);
        currentCol = 0;
        currentRow = 0;
        ddRamddr = true;
		os_delay_us(2000);
	}

    void home() {
        issueCommand(Commands::RETURN_HOME);
        currentCol = 0;
        currentRow = 0;
        ddRamddr = true;
        os_delay_us(2000);
    }

    void setDirection(bool writeRight,bool shiftAdress) {
        uint8_t args = 0;
        if (writeRight) {
            args |= Display_Mode::DIRECTION;
        }
        if (shiftAdress) {
            args |=Display_Mode::SHIFT;
        }
           
        issueCommand(Commands::ENTRY_MODE_SET, args);
    }

    void display(char character) {
        if (currentCol >= colSize) {
            changeRow();
        }
        if (!ddRamddr) {
            setCursor(currentCol, currentRow);
        }
        RS::set();
        write(character);
        currentCol++;
    }


    void display(uint8_t colNumber, uint8_t rowNumber, char character) {
        setCursor(colNumber, rowNumber);
        RS::set();
        write(character);
        currentCol++;
    }

    void setCursor(uint8_t colNumber, uint8_t rowNumber) {
        uint8_t args = row_addresses[rowNumber];
        args += colNumber;
        issueCommand(Mode::SET_DDRAMADDR, args);
        currentCol = colNumber;
        currentRow = rowNumber;
        ddRamddr = true;
    }

    void addChar(uint8_t location, uint8_t matrice[]) {
        if (!dots5x11Matrice && location > 8) {
            location = 8;
        }
        else if (dots5x11Matrice && location > 4) {
            location = 4;
        }
        ddRamddr = false;
        issueCommand(Mode::SET_CGRAMADDR, location << 3);
        auto length = dots5x11Matrice ? 11 : 8;
        RS::set();
        for (auto i = 0; i < length; i++) {
            write(matrice[i]);
        }
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
    bool ddRamddr = false;
    bool dots5x11Matrice = false;

    void changeRow() {
        if ((currentRow+1) < lineNumber) {
            currentRow++;
            setCursor(0, currentRow );
        }
        else {
            setCursor(0, 0);
        }
    }

    void initAddresses() {
        for (auto i = 0; i < lineNumber; i++) {
            row_addresses[i] = colSize * (i / 2) + 0x40 * (i % 2);
        }
    }

		void issueCommand(Mode mode, uint8_t arg) {
        RS::clear();
		write(mode | arg);
	}
		
		void issueCommand(Commands command) {
        RS::clear();
		write(command);
	}
		
		void issueCommand(Commands command,uint8_t arg) {
        RS::clear();
		write(command | arg);
	}

    void write(uint8_t data) {
		D4::set(((1 << 4)&data) != 0 );
		D5::set(((1 << 5)&data) != 0);
		D6::set(((1 << 6)&data) != 0);
		D7::set(((1 << 7)&data) != 0);
		//ENABLE::pulseHigh();
		pulseEnable();
		D4::set(((1 << 0)&data) != 0);
		D5::set(((1 << 1)&data) != 0);
		D6::set(((1 << 2)&data) != 0);
		D7::set(((1 << 3)&data) != 0);
		//ENABLE::pulseHigh();
		pulseEnable();
	}
		
    void pulseEnable() {
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
    static void print(HD44780& h,const char str[]) {
        for (auto i = 0; str[i] != '\0'; i++) {
            h.display(str[i]);
        }
    }
};
}