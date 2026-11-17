#include "tusb.h"
#include "pico/unique_id.h"
#include <string.h>
static tusb_desc_device_t const device = {
 .bLength=sizeof(tusb_desc_device_t), .bDescriptorType=TUSB_DESC_DEVICE,
 .bcdUSB=0x0200, .bDeviceClass=TUSB_CLASS_MISC, .bDeviceSubClass=MISC_SUBCLASS_COMMON,
 .bDeviceProtocol=MISC_PROTOCOL_IAD, .bMaxPacketSize0=CFG_TUD_ENDPOINT0_SIZE,
 .idVendor=0xcafe, .idProduct=0x4015, .bcdDevice=0x0110,
 .iManufacturer=1, .iProduct=2, .iSerialNumber=3, .bNumConfigurations=1
};
uint8_t const *tud_descriptor_device_cb(void) { return (uint8_t const *)&device; }
static uint8_t const hid[] = { TUD_HID_REPORT_DESC_KEYBOARD() };
uint8_t const *tud_hid_descriptor_report_cb(uint8_t instance) { (void)instance; return hid; }
enum { ITF_CDC, ITF_CDC_DATA, ITF_HID, ITF_COUNT };
#define CONFIG_LEN (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN + TUD_HID_DESC_LEN)
static uint8_t const config[] = {
 TUD_CONFIG_DESCRIPTOR(1, ITF_COUNT, 0, CONFIG_LEN, 0, 100),
 TUD_CDC_DESCRIPTOR(ITF_CDC, 4, 0x81, 8, 0x02, 0x82, 64),
 TUD_HID_DESCRIPTOR(ITF_HID, 5, HID_ITF_PROTOCOL_KEYBOARD, sizeof(hid), 0x83, 16, 1)
};
uint8_t const *tud_descriptor_configuration_cb(uint8_t index) { (void)index; return config; }
uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
 (void)langid;
 static uint16_t out[64];
 static char serial[PICO_UNIQUE_BOARD_ID_SIZE_BYTES*2+1];
 if(index==0) { out[0]=(TUSB_DESC_STRING<<8)|4; out[1]=0x0409; return out; }
 pico_get_unique_board_id_string(serial,sizeof serial);
 const char *s;
 switch(index) {
  case 1: s="Privates Bastelprojekt"; break;
  case 2: s="KeePass-Taster Pico 2"; break;
  case 3: s=serial; break;
  case 4: s="Passwort einrichten"; break;
  case 5: s="Deutsche Tastatur"; break;
  default: return NULL;
 }
 size_t n=strlen(s); if(n>63) n=63;
 out[0]=(TUSB_DESC_STRING<<8)|(2*n+2);
 for(size_t i=0;i<n;i++) out[i+1]=(uint8_t)s[i];
 return out;
}
