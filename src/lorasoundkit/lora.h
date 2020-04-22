/** 
 * LoRa LMIC wrapper class header
 * Marcel Meek april 2020
 * 
 * define here your keys and LoRa RFM95 pinning
 */

// specify here TTN keys 
#define APPEUI "0001020304050607"
#define DEVEUI "0000000000000001"
#define APPKEY "000102030405060708090A0B0C0D0E0F"

// define RFM 95 pins
#define NSS 16
#define RXTX LMIC_UNUSED_PIN
#define RST 5
#define DIO1 26
#define DIO2 33
#define DIO3 32


class LoRa {
  public:
    LoRa();
    ~LoRa();
    void sendMsg( int port, uint8_t* buf, int len);
    void loop();

  private:
    //void onEvent (ev_t ev);
    
};
