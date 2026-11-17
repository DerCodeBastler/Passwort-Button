from pathlib import Path
import struct,subprocess,sys,os
build=Path(sys.argv[1] if len(sys.argv)>1 else "build")
p=(build/'keepass_pico.uf2');b=p.read_bytes();assert b and len(b)%512==0
normal=[];absolute=[]
for off in range(0,len(b),512):
 magic0,magic1,flags,address,size,num,total,family=struct.unpack_from('<8I',b,off)
 assert (magic0,magic1)==(0x0A324655,0x9E5D5157)
 assert struct.unpack_from('<I',b,off+508)[0]==0x0AB16F30
 assert size==256
 if family==0xe48bff59:
  assert flags==0x2000
  assert 0x10000000<=address<address+size<=0x103ff000
  normal.append((num,total,address,b[off+32:off+32+size]))
 else:
  assert family==0xe48bff57 and flags==0xa000 and address==0x10ffff00
  absolute.append(num)
assert len(absolute)==1
assert [x[0] for x in normal]==list(range(len(normal)))
assert all(x[1]==len(normal) for x in normal)
image=b''.join(x[3] for x in normal)
raw=(build/'keepass_pico.bin').read_bytes();assert image[:len(raw)]==raw
assert all(x==0 for x in image[len(raw):])
nm=subprocess.check_output([os.environ.get('ARM_NM','arm-none-eabi-nm'),'-n',str(build/'keepass_pico.elf')],text=True)
address=int(next(line.split()[0] for line in nm.splitlines() if line.endswith(' bootsel_pressed')),16)
assert 0x20000000<=address<0x20082000
func=(build/'keepass_pico.dis').read_text().split('<bootsel_pressed>:')[1].split('\n\n')[0]
assert 'cpsid' in func and 'PRIMASK' in func
assert not any('\tbl\t' in line or '\tblx\t' in line for line in func.splitlines())
print(f'PASS: {len(normal)} RP2350 ARM Secure flash blocks, ROM compatibility block, binary identity, no overlap with configuration sector, BOOTSEL in RAM without calls')

assert 'stdio_usb' not in nm
assert 'reset_usb_boot' not in nm
assert 'reset_interface' not in nm
assert 'stdio_uart' not in nm
assert 'otp_access' not in nm
assert len(raw)<0x3ff000
# Deskriptor aus der tatsächlich kompilierten Firmware lesen.
config_address=int(next(line.split()[0] for line in nm.splitlines() if line.endswith(' config')),16)
config_offset=config_address-0x10000000
config_len=struct.unpack_from('<H',raw,config_offset+2)[0]
desc=raw[config_offset:config_offset+config_len]
interfaces=[];endpoints=[];i=0
while i<len(desc):
 length,kind=desc[i:i+2];assert length>=2 and i+length<=len(desc)
 if kind==4: interfaces.append((desc[i+2],desc[i+5]))
 if kind==5: endpoints.append((desc[i+2],desc[i+6]))
 i+=length
assert interfaces==[(0,2),(1,10),(2,3)],interfaces
assert (0x83,1) in endpoints,endpoints
assert not any(cls in (8,255) for _,cls in interfaces)
print('PASS: HID-Abfrage 1 ms, nur CDC + Tastatur, kein Laufwerk/Reset-Interface/USB-stdio/OTP-Code')
