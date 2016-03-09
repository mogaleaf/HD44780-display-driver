/*
 * Hello_World.cpp
 *
 * Created: 15/01/2014 18:14:54
 *  Author: Ambroise Leclerc
 */ 


#define F_CPU 16000000                      // CPU Frequency
#define __DELAY_BACKWARD_COMPATIBLE__       // Avoid any complaints in util/delay.h about "__builtin_avr_delay_cycles expects an integer constant"

#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>

#define ETL_FREESTORE_LOG_DEPTH 64
#include <etl.h>


#include "gfxlite.h"
#include "drivers/ili9320.h"
#include "drivers/ads7843.h"
#include "font6x10.h"
#include "vectorfontlcd.h"
#include <etl/ioports.h>
#include <etl/metautils.h>
#include <etl/watchdog.h>
#include <etl/interrupts.h>

#include "drivers/enc28j60.h"

#include "ethernet.h"
#include "ip.h"
#include "arp.h"
#include "dhcp.h"
#include "event.h"
#include "network.h"
#include "clock.h"

#include <memory>


/*
  00 0C 29 35 11 80 VMWare            192.168.2.200
  00 20 00 71 67 69 Imprimante        192.168.2.18
  00 24 D4 7E 33 82 Freebox Player    192.168.2.50
  F4 6D 04 74 57 C5 i7                192.168.2.102
  1C AB A7 8D FB 14 ipad3             192.168.2.48
  60 03 08 DE E3 05 ipad5             192.168.2.46
  D8 30 62 79 97 75 ipad1             192.168.2.7
  D4 3D 7E 33 EA DB Marge             192.168.2.100
  00 18 7F 00 40 2B Zibase            192.168.2.10
  14 0C 76 66 C5 50 Freebox Server    192.168.2.254
  */
    
  /* 60 bytes
  
  01 80 C2 00 00 0E      D4 3E 7E 33 EA DB     88 CC 02 07 04 D4 3D 7E  // Spanning Tree multicast address
  01 00 5E 43 28 5B      1C AB A7 8D FB 14     08 00 45 00 00 27 63 20 
  FF FF FF FF FF FF      A8 7A FC 67 D5 B6     10 0C 00 02 10 00 0C 08
  FF FF FF FF FF FE      28 18 EC CD 8A A0     10 0C 00 02 10 00 0C 08
  01 00 5E 43 2B 5B      60 03 08 DE E3 05     08 00 45 00 00 27 83 64  // Internet multicast mac      type 8 192.168.2.46-> 
  
  */


using namespace etl;

template<typename TIMER>
class TestTimerInterrupts  {
 public:
  static void Init(uint8_t duty_cycle_percent) {
    Interrupts::Disable();
    TIMER::SetInterruptMask(TIMER::COMPARE_MATCH_A);
    TIMER::SetOutputCompareValueA(duty_cycle_percent);
    TIMER::SetInterruptMask(TIMER::COMPARE_MATCH_B);
    TIMER::SetOutputCompareValueB(100);
    TIMER::SetPrescaler(TIMER::CLK_DIV_1024);
    Interrupts::Enable();
  }
};


/*
void Timer1::ISRNaked::CompareMatchA() {
  PinB1::Toggle();
  reti();
}

void Timer1::ISRNaked::CompareMatchB() {
  SetValue(0);
  PinB1::Toggle();  
  reti();
}  

void Timer0::ISR::CompareMatchA() { PinB1::Clear(); }
void Timer0::ISR::CompareMatchB() { SetValue(0); PinB1::Set(); }
void Timer2::ISR::CompareMatchA() { PinB0::Clear(); }
void Timer2::ISR::CompareMatchB() { SetValue(0); PinB0::Set(); }
*/

class Application {
 public:
 
  static volatile uint16_t isr_counter_;
 
  void Setup() {
/*    //PinB1::SetOutput(); PinB1::Clear();
    PinB0::SetOutput(); PinB0::Set();
    PinD7::SetInput(); PinD7::Clear();
    PinB1::SetInput(); PinB1::Clear();
    Interrupts::Disable();
    //PinChangeInt2::Enable(PCINT23);
    PinChangeIRQ0::EnableSource(PCINT1);
    Interrupts::Enable();
    
    TestTimerInterrupts<Timer0>::Init(100);
    TestTimerInterrupts<Timer2>::Init(100);
   */ 
    tft_.background_color_=tft_.WHITE;
    Clock<F_CPU, 1> clock;
    clock.StartChrono(0);
    tft_.Cls();
    uint16_t elapsed = clock.GetChronoMilliseconds(0);
    tft_.Printf(0,210, tft_.WHITE, Font6x10, "Cls Time : %d ms", elapsed);
    
  }
  
  void Start() {
    
    VectorFontLCD<decltype(tft_)> LCDFont(90, 10, tft_.CYAN, tft_.BLACK);
    //Watchdog<8000> watchdog;
    
    
    uint8_t data_line = 180;
    bool configured = false;
    bool time_configured = false;
    while (true) {
      //watchdog.Rearm();
      
      
      
      network_.clock_.StartChrono(0);   
      Event net_event = network_.GetEvent();
      uint16_t elapsed = network_.clock_.GetChronoMilliseconds(0);
      if (elapsed != 0) tft_.Printf(0, 230, tft_.RED, Font6x10, "Network event clock %d ms", elapsed);
      tft_.Printf(0, 180, tft_.BLUE, Font6x10, "Longueur queue : %d ", network_.task_queue_.size());
      tft_.Printf(0, 170, tft_.BLUE, Font6x10, "event : %d timing:%d ms Clock : %02d:%02d:%02d ", net_event.id_,
                  elapsed, network_.clock_.hour_, network_.clock_.min_, network_.clock_.sec_);
                  
      tft_.Printf(0, 160, tft_.BLUE, Font6x10, "Queue : %d  FreeStore : %d bytes  frag: %d %%", network_.task_queue_.size(),
       etl::FreeStore::GetMemorySize(), etl::FreeStore::GetMemoryFragmentation());
      tft_.Printf(0, 150, tft_.BLUE, Font6x10, "Freemem : %d bytes", FreeStore::GetFreeMemory());
	  tft_.Printf(0, 130, tft_.BLUE, Font6x10, "malloc_heap : 0x%04X   : 0x%04X", __malloc_heap_start, __malloc_heap_end);
    
    
	  //tft_.Printf(0, 120, tft_.BLUE, Font6x10, "bss : 0x%04X   : 0x%04X", __bss_end, __heap_start);
    
      uint8_t logx = 0, logy = 120;
      for (uint8_t line=0; line<FreeStore::FreeStoreDebugTrace::log_counter; line++) {
        tft_.Printf(logx * 40, logy, tft_.BLUE, Font6x10, "%c%04X", FreeStore::FreeStoreDebugTrace::log_operation[logx], FreeStore::FreeStoreDebugTrace::log_address[logx]);
        logx++;
        if (logx == 7) {
          logx=0;
          logy -= 10;
        }          
      }        
  
      
      switch (net_event.id_) {
        case Event::NET_IP_ATTRIBUTED:
          tft_.Printf(0, 190, tft_.MAGENTA, Font6x10, "Network ok : %d.%d.%d.%d ", net_event.data_[0], net_event.data_[1], net_event.data_[2], net_event.data_[3]);
          configured = true;
          break;
          
        case Event::NET_ARP:
          tft_.Printf(0, 180, tft_.MAGENTA, Font6x10, "ARP : %02X:%02X:%02X:%02X:%02X:%02X ", net_event.data_[0], net_event.data_[1], net_event.data_[2], net_event.data_[3]
          , net_event.data_[4], net_event.data_[5]);
          break;
        default: {}
      }
      
      if (configured) {
        tft_.Printf(170, 180, tft_.MAGENTA, Font6x10, "DNS : %d.%d.%d.%d ", network_.dhcp_client_.GetDNSIp()[0], network_.dhcp_client_.GetDNSIp()[1],
                  network_.dhcp_client_.GetDNSIp()[2], network_.dhcp_client_.GetDNSIp()[3]);
        if (!time_configured) {
          using TimeRequest = UDPprotocol<IPv4<>>;
          auto buffer = std::make_unique<uint8_t[]>(sizeof(TimeRequest));
          auto message = reinterpret_cast<TimeRequest*>(buffer.get());
          message->InitializeUDP(8000, UDPPort::TIME, 0);
          message->InitializeIPv4(sizeof(UDPprotocol<>), IPv4<>::Protocol::UDP);
          network_.SendIPPacket(std::move(buffer), sizeof(TimeRequest));
        }          
      }                  
      
      tft_.FillRect(0, data_line, 1, 10, tft_.background_color_);
      data_line -= 8;
      if (data_line<100) {
        data_line = 180;
      }
      tft_.FillRect(0, data_line, 1, 10, tft_.RED);
    
    }   
  }    
 private:
  GfxLite<ILI9320_Controller<SPILink_ILI<PinD2, PinD3, SPIMasterOnUSART<>>>, ADS7843_TouchController<PortC, PC0, PC1, PC2, PC3, PC4>> tft_;
  Network<0x000011ED0001> network_;
};


int main(void) {
  Application app;
    
  app.Setup();
  app.Start();  
}

