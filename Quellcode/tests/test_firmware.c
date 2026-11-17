/* Führt den echten Firmwarecode mit simuliertem USB, Flash und BOOTSEL aus. */
#define KEEPASS_HOST_TEST
#include "../firmware/main.c"
#include <assert.h>
#include <stdlib.h>
uint8_t persisted[FLASH_SECTOR_SIZE];
static uint32_t clock_ms;
static bool physical_button,mounted,suspended,inflight,stall_usb,flash_fail,led;
static unsigned flash_writes;
static uint32_t ack_due;
static uint8_t queued_mod,queued_keys[6];
static struct {uint32_t time;uint8_t modifier,keys[6];} reports[4096];
static unsigned reports_count;
static uint8_t rx[8192];
static unsigned rxlen,rxpos;
static char tx[8192];
uint32_t get_absolute_time(void){return clock_ms;}
uint32_t to_ms_since_boot(uint32_t time){return time;}
uint32_t save_and_disable_interrupts(void){return 0;}
void restore_interrupts(uint32_t state){(void)state;}
void flash_range_erase(uint32_t offset,size_t count){
 assert(offset==STORE_OFFSET&&count==FLASH_SECTOR_SIZE);memset(persisted,0xff,count);
}
void flash_range_program(uint32_t offset,const uint8_t *data,size_t count){
 assert(offset==STORE_OFFSET&&count%FLASH_PAGE_SIZE==0&&count<=FLASH_SECTOR_SIZE);
 ++flash_writes;if(!flash_fail)memcpy(persisted,data,count);
}
void tud_cdc_write_str(const char *text){assert(strlen(tx)+strlen(text)<sizeof tx);strcat(tx,text);}
void tud_cdc_write_flush(void){}
unsigned tud_cdc_available(void){return rxlen-rxpos;}
int tud_cdc_read_char(void){assert(rxpos<rxlen);return rx[rxpos++];}
void tud_cdc_read_flush(void){rxpos=rxlen;}
bool tud_mounted(void){return mounted;}
bool tud_suspended(void){return suspended;}
bool tud_hid_ready(void){return mounted&&!suspended&&!inflight;}
bool tud_hid_keyboard_report(uint8_t id,uint8_t modifier,const uint8_t keys[6]){
 assert(id==0);if(!tud_hid_ready())return false;
 inflight=true;ack_due=clock_ms+1;queued_mod=modifier;memcpy(queued_keys,keys,6);return true;
}
void tud_task(void){
 if(inflight&&mounted&&!suspended&&!stall_usb&&(int32_t)(clock_ms-ack_due)>=0){
  assert(reports_count<4096);reports[reports_count].time=clock_ms;
  reports[reports_count].modifier=queued_mod;memcpy(reports[reports_count++].keys,queued_keys,6);
  inflight=false;tud_hid_report_complete_cb(0,NULL,8);
 }
}
void gpio_put(unsigned pin,bool state){assert(pin==25);led=state;}
static bool bootsel_pressed(void){return physical_button;}
static void tick(unsigned ms){while(ms--){++clock_ms;app_tick();}}
static void restart(bool erase){
 if(erase){memset(persisted,0xff,sizeof persisted);flash_writes=0;}
 clock_ms=0;physical_button=false;mounted=true;suspended=false;inflight=false;stall_usb=false;flash_fail=false;led=false;
 typing=false;key_down=false;report_pending=false;release_pending=false;
 due=last_progress=led_until=setup_until=0;led_feedback=false;setup_enabled=false;pos=0;caps=0;
 reports_count=0;rxlen=rxpos=0;tx[0]=0;memset(&button,0,sizeof button);reset_line();
 load_record();tud_mount_cb();tick(45);reports_count=0;
}
static void raw_bytes(const void *bytes,size_t n){
 assert(n<sizeof rx);memcpy(rx,bytes,n);rxlen=(unsigned)n;rxpos=0;tx[0]=0;
 tick((unsigned)((n+63)/64)+1);
}
static void run(const char *text){
 char buffer[4096];assert(strlen(text)+2<=sizeof buffer);snprintf(buffer,sizeof buffer,"%s\n",text);
 raw_bytes(buffer,strlen(buffer));
}
static void held(unsigned ms){physical_button=true;tick(ms);physical_button=false;tick(40);}
static void unlock(void){held(2100);assert(setup_open(clock_ms));assert(!typing);}
static void tap(void){held(60);}
static void store_password(unsigned delay,bool enter){
 char text[100];unlock();snprintf(text,sizeof text,"SET %u %u 04020500",delay,enter?1:0);run(text);
 assert(!strcmp(tx,"OK SAVED\n"));assert(!setup_open(clock_ms));
}
static unsigned nonzero_reports(void){unsigned n=0;for(unsigned i=0;i<reports_count;i++)if(reports[i].keys[0])++n;return n;}
static void test_protocol_and_storage(void){
 restart(true);assert(valid_record(&record));assert(record.delay_ms==3&&!record.count);
 run("STATUS");assert(!strcmp(tx,"EMPTY DE V2 DELAY=3 ENTER=0 LOCKED\n"));
 run("SET 3 1 0400");assert(!strcmp(tx,"ERR LOCKED\n"));assert(!flash_writes);
 store_password(3,true);assert(record.append_enter==1&&record.delay_ms==3&&record.count==2);
 record_t before=record;restart(false);assert(!memcmp(&before,&record,sizeof record));
 const char *attacks[]={"GET","READ","DUMP","TYPE","TYPE_PASSWORD","UNLOCK","BOOTSEL","RESET","REBOOT","RUN","STATUS 0","SET 3 1 0400","CONFIG 50 0","CLEAR"};
 for(unsigned i=0;i<sizeof attacks/sizeof *attacks;i++){run(attacks[i]);assert(!strncmp(tx,"ERR ",4));}
 assert(!memcmp(&before,&record,sizeof record));assert(!nonzero_reports());
 unlock();unsigned writes=flash_writes;
 const char *bad[]={"SET 2 1 0400","SET 51 0 0400","SET -3 1 0400","SET 03 1 0400","SET 9999999999 1 0400","SET 3 2 0400","SET 3 1","SET 3 1 ","SET 3 1 040","SET 3 1 0x00","SET 3 1 0401","SET 3 1 0100","SET 3 1 2800","SET 3 1 2900","SET 3 1 2b00","SET 3 1 6540","CONFIG 5 0 extra","CONFIG 5","CONFIG ","SET "};
 for(unsigned i=0;i<sizeof bad/sizeof *bad;i++){
  run(bad[i]);assert(!strncmp(tx,"ERR ",4));assert(!memcmp(&before,&record,sizeof record));assert(flash_writes==writes);
 }
 const unsigned char nul[]="SET 3 1 0400\0STATUS\n";raw_bytes(nul,sizeof nul-1);assert(!strcmp(tx,"ERR FRAME\n"));
 uint8_t overlong[2200];memset(overlong,'A',sizeof overlong);overlong[sizeof overlong-1]='\n';
 raw_bytes(overlong,sizeof overlong);assert(!strcmp(tx,"ERR FRAME\n"));assert(flash_writes==writes);
 raw_bytes("SET 3 ",6);tick(1001);raw_bytes("1 0400\n",7);assert(!strcmp(tx,"ERR FRAME\n"));assert(flash_writes==writes);
 char maximum[1100]="SET 3 1 ";for(unsigned i=0;i<MAX_KEYS;i++)strcat(maximum,"3500");
 run(maximum);assert(!strcmp(tx,"OK SAVED\n"));assert(record.count==MAX_KEYS&&record.append_enter==1);
 restart(false);assert(record.count==MAX_KEYS&&record.delay_ms==3&&record.append_enter==1);
 for(unsigned delay=3;delay<=50;delay++){
  unlock();char cmd[30];snprintf(cmd,sizeof cmd,"CONFIG %u %u",delay,delay%2);run(cmd);assert(!strcmp(tx,"OK CONFIG\n"));
  restart(false);assert(record.count==MAX_KEYS&&record.delay_ms==delay&&record.append_enter==delay%2);
 }
 unlock();run("CLEAR");assert(!strcmp(tx,"OK CLEARED\n"));assert(record.delay_ms==50&&record.count==0&&!record.append_enter);
 restart(false);assert(record.delay_ms==50&&record.count==0&&!record.append_enter);tap();tick(1000);assert(!nonzero_reports());
 puts("PASS: Protokoll, USB-Lesesperre, 48 persistente Tempostufen, Enter, Löschung, Eingabegrenzen");
}
static void test_legacy(void){
 restart(true);legacy_record_t old={.magic=MAGIC_V1,.count=3,.pairs={4,2,5,0,0x28,0}};
 old.checksum=pair_hash(old.pairs,old.count);memcpy(persisted,&old,sizeof old);restart(false);
 assert(record.count==2&&record.delay_ms==3&&record.append_enter==1);assert(!memcmp(record.pairs,old.pairs,4));
 unlock();run("CONFIG 17 1");restart(false);assert(record.delay_ms==17&&record.append_enter==1&&record.count==2);
 old.count=2;old.checksum=pair_hash(old.pairs,old.count);memcpy(persisted,&old,sizeof old);restart(false);
 assert(record.count==2&&record.delay_ms==3&&!record.append_enter);
 old.checksum++;memcpy(persisted,&old,sizeof old);restart(false);assert(!record.count);
 old.count=MAX_KEYS+1;memcpy(persisted,&old,sizeof old);restart(false);assert(!record.count);
 store_password(3,true);persisted[16]=2;restart(false);assert(!record.count&&record.delay_ms==3);
 store_password(3,true);persisted[12]=2;restart(false);assert(!record.count);
 store_password(3,true);persisted[8]^=1;restart(false);assert(!record.count);
 puts("PASS: Originalformat übernommen, Enter erhalten, beschädigte Datensätze abgelehnt");
}
static void test_button_and_hid(void){
 for(unsigned delay=3;delay<=50;delay+=47){
  restart(true);store_password(delay,true);reports_count=0;tap();tick(1000);
  const uint8_t expect[]={4,0,5,0,0x28,0};assert(reports_count==6);
  for(unsigned i=0;i<6;i++){
   assert(reports[i].keys[0]==expect[i]);assert(reports[i].modifier==(i==0?2:0));
   if(i)assert(reports[i].time-reports[i-1].time==delay+1);
  }
  assert(!typing);unsigned n=reports_count;tick(2000);assert(reports_count==n);
 }
 restart(true);store_password(3,false);reports_count=0;physical_button=true;tick(1000);assert(!nonzero_reports());physical_button=false;tick(1000);assert(nonzero_reports()==2);
 reports_count=0;for(unsigned i=0;i<10;i++){physical_button=true;tick(5);physical_button=false;tick(5);}tick(500);assert(!nonzero_reports());
 reports_count=0;unlock();tick(1000);assert(!nonzero_reports());tap();assert(!setup_open(clock_ms));tick(1000);assert(!nonzero_reports());
 unlock();tick(SETUP_WINDOW_MS);assert(!setup_open(clock_ms));run("CONFIG 4 1");assert(!strcmp(tx,"ERR LOCKED\n"));
 /* Eine während des USB-Verbindens gehaltene Taste darf keine Ausgabe starten. */
 physical_button=true;tud_umount_cb();inflight=false;tud_mount_cb();tick(100);physical_button=false;tick(1000);assert(!nonzero_reports());
 /* Caps-Lock-Ausgaben/GET_REPORT dürfen keine Tastaturausgabe auslösen. */
 uint8_t report[16];memset(report,0x7b,sizeof report);
 for(unsigned type=0;type<5;type++)for(unsigned id=0;id<256;id++){
  assert(!tud_hid_get_report_cb(0,(uint8_t)id,(hid_report_type_t)type,report,sizeof report));
  assert(report[0]==0x7b);tud_hid_set_report_cb(0,(uint8_t)id,(hid_report_type_t)type,report,1);
 }
 tick(1000);assert(!nonzero_reports());
 reports_count=0;uint8_t caps_on=2;tud_hid_set_report_cb(0,0,HID_REPORT_TYPE_OUTPUT,&caps_on,1);tap();tick(1000);
 assert(reports[0].keys[0]==4&&reports[0].modifier==0);assert(reports[2].keys[0]==5&&reports[2].modifier==2);
 /* Maximalfolge plus Enter muss ohne Abbruch funktionieren. */
 restart(true);unlock();char cmd[1100]="SET 50 1 ";for(unsigned i=0;i<256;i++)strcat(cmd,"0400");run(cmd);
 reports_count=0;tap();tick(28000);assert(!typing&&nonzero_reports()==257&&reports_count==514);
 /* Auch 32-Bit-Zeitüberlauf darf keine spontane Ausgabe auslösen. */
 restart(false);clock_ms=UINT32_MAX-100;memset(&button,0,sizeof button);button.changed=clock_ms;button.last_poll=clock_ms;
 tick(50);reports_count=0;tap();tick(28000);assert(nonzero_reports()==257);
 puts("PASS: physische Klicks, Halten/Entprellen, 3/50-ms-HID-Timing, Caps Lock, Enter, Zeitüberlauf");
}
static void test_usb_cancel_and_failure(void){
 restart(true);store_password(3,true);reports_count=0;tap();tick(120);assert(typing);run("CLEAR");assert(!strcmp(tx,"ERR BUSY\n"));
 suspended=true;tud_suspend_cb(false);inflight=false;tick(1000);unsigned n=nonzero_reports();
 suspended=false;tud_resume_cb();tick(1000);assert(!typing&&nonzero_reports()==n);assert(!setup_open(clock_ms));
 reports_count=0;tap();tick(120);mounted=false;tud_umount_cb();inflight=false;tick(500);n=nonzero_reports();
 mounted=true;tud_mount_cb();tick(1000);assert(!typing&&nonzero_reports()==n);
 reports_count=0;tap();tick(130);stall_usb=true;tick(500);assert(!typing);stall_usb=false;tick(500);assert(nonzero_reports()<=1);
 /* Ein schon eingereihtes USB-Paket kann vor dem Timeout noch beim Host landen.
    Der restliche Text darf nach Wiederaufnahme nicht weiterlaufen. */
 restart(false);unlock();tud_cdc_line_state_cb(0,false,false);assert(!setup_open(clock_ms));
 unlock();record_t before=record;flash_fail=true;run("SET 8 0 0600");assert(!strcmp(tx,"ERR FLASH\n"));assert(!memcmp(&before,&record,sizeof record));
 flash_fail=false;restart(false);assert(!record.count); /* Fehlerhafter Flash lädt niemals eine Teilfolge. */
 puts("PASS: Suspend/Trennen/USB-Stau brechen Ausgabe ab; Flashfehler erkannt; DTR sperrt Einrichtung");
}
static uint32_t rnd=0x8ad0ffeeu;
static unsigned next_random(void){rnd^=rnd<<13;rnd^=rnd>>17;rnd^=rnd<<5;return rnd;}
static void test_parser_fuzz(void){
 restart(true);store_password(3,true);record_t before=record;reports_count=0;
 for(unsigned i=0;i<20000;i++){
  if(i%20==0){setup_enabled=true;setup_until=clock_ms+60000;} /* Parser auch hinter der Sperre testen. */
  uint8_t data[1100];unsigned n=next_random()%sizeof data;
  for(unsigned j=0;j<n;j++)data[j]=(uint8_t)next_random();
  if(n)data[n-1]='\n';
  raw_bytes(data,n);reset_line();
  assert(!memcmp(&before,&record,sizeof record));assert(!typing);
 }
 assert(!nonzero_reports());puts("PASS: 20.000 zufällige Frames ohne Speicherfehler/Passwortausgabe");
}
int main(void){
 test_protocol_and_storage();test_legacy();test_button_and_hid();test_usb_cancel_and_failure();test_parser_fuzz();
 puts("ALLE FIRMWARETESTS BESTANDEN (simulierte Hardware)");return 0;
}
