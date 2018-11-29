/*
 * VS1053.cpp
 *
 *  Created on: Nov 21, 2018
 *      Author: John (Offline)
 */

#include <VS1053.h>

#define min(a,b) (((a)<(b))?(a):(b))

void VS1053::init(LPC1758_GPIO_Type pinDREQ, LPC1758_GPIO_Type pin_CS, LPC1758_GPIO_Type pin_DCS)
{
     DREQ = new GPIO(pinDREQ);
     DREQ->setAsInput();
     _CS  = new GPIO(pin_CS);
     _CS->setAsOutput();
     _CS->setHigh();
     _DCS = new GPIO(pin_DCS);
     _DCS->setAsOutput();
     _DCS->setHigh();
     //SPI0.initialize(8, LabSpi0::SPI, 17);
     SPI0.initialize(8, LabSpi0::SPI, 14);
     playingMusic = false;
}

uint8_t VS1053::spiread(void)
{
    //return SPI0.transfer(0x00);
    return SPI0.transfer(0x00);
}

uint16_t VS1053::sciRead(uint8_t addr) {
  uint16_t data;
  _CS->setLow();
  spiwrite(0x03);
  spiwrite(addr);
  vTaskDelay(10);
  data = spiread();
  data <<= 8;
  data |= spiread();
  _CS->setHigh();
  return data;
}

void VS1053::sciWrite(uint8_t addr, uint16_t data)
{
    _CS->setLow();
    spiwrite(0x02);
    spiwrite(addr);
    spiwrite(data >> 8);
    spiwrite(data & 0xFF);
    _CS->setHigh();
}

void VS1053::spiwrite(uint8_t c)
{
  uint8_t x __attribute__ ((aligned (32))) = c;
  spiwrite(&x, 1);
}

void VS1053::spiwrite(uint8_t *c, uint16_t num)
{
    while (num--)
    {
      //SPI0.transfer(c[0]);
      SPI0.transfer(c[0]);
      c++;
    }
}

void VS1053::soft_reset(void){
    uint16_t mode = sciRead(SCI_MODE);
    mode |= SM_RESET;
    sciWrite(SCI_MODE, mode);
}

void VS1053::sineTest(uint8_t n, uint16_t ms) {
  soft_reset();
  uint16_t mode = sciRead(SCI_MODE);
  mode |= 0x0020;
  sciWrite(SCI_MODE, mode);

  while (DREQ->read()==0);
  _DCS->setLow();
  spiwrite(0x53);
  spiwrite(0xEF);
  spiwrite(0x6E);
  spiwrite(n);
  spiwrite(0x00);
  spiwrite(0x00);
  spiwrite(0x00);
  spiwrite(0x00);
  _DCS->setHigh();
  vTaskDelay(500);
  _DCS->setLow();
  spiwrite(0x45);
  spiwrite(0x78);
  spiwrite(0x69);
  spiwrite(0x74);
  spiwrite(0x00);
  spiwrite(0x00);
  spiwrite(0x00);
  spiwrite(0x00);
  _DCS->setHigh();
  vTaskDelay(500);
}

void VS1053::setVolume(uint8_t left, uint8_t right) {
  uint16_t volume;
  volume = left;
  volume <<= 8;
  volume |= right;
  sciWrite(SCI_VOL, volume);
}

bool VS1053::startPlayMP3File(const char *track)
{
//This function is still laggy, need to improve it.
    soft_reset();
    // reset playback
    sciWrite(SCI_MODE, SM_LINE1 | SM_SDINEW);
    // resync
    sciWrite(SCI_WRAMADDR, 0x1e29);
    sciWrite(SCI_WRAM, 0);
    setVolume(40,40);
    //Open file
    f_open(&currentTrack, track, FA_READ);
    if (&currentTrack == 0)
    {
        return false;
    }
    sciWrite(SCI_DECODE_TIME, 0x00);
    sciWrite(SCI_DECODE_TIME, 0x00);
    playingMusic = true;
    UINT br;
    //
    SPI0.setdivider(4);
    while (DREQ->read()==0);
    while (playingMusic)
    {
      //printf("Inside loop 1\n");
      f_read(&currentTrack, musicBuffer, 512, &br);
      while (DREQ->read()==0);
      if(br == 0) //No more bytes read in the file, we're done.
      {
          playingMusic = false;
          f_close(&currentTrack);
          break;
      }
      u_int8 *bufP = musicBuffer;
      _DCS->setLow();
      while(br)
      {
          while (DREQ->read()==0);

          int t = min(32, br);
          spiwrite(bufP, t);
          bufP += t;
          br -= t;

      }
      _DCS->setHigh();
    }
    SPI0.setdivider(14);
}

void VS1053::getSongs(){
    DIR directory;
    static FILINFO fno;
    FRESULT res;
    char LF_name[128];
    fno.lfname = LF_name;
    fno.lfsize = sizeof(LF_name);

    // printf("asdf: %s\n", LF_name);
    res = f_opendir(&directory, "1:");
    
    if(res == FR_OK){
        while(1){
            res = f_readdir(&directory, &fno);
            if(res != FR_OK || fno.fname[0] == 0) break;

            //If file name ends in .mp3 or .MP3 store in string array
            if(strstr(fno.lfname, ".mp3") || strstr(fno.lfname, ".MP3")){
                int length = strnlen(fno.lfname, 128);
                songs[number_of_songs] = new char[length + 1];
                strcpy(songs[number_of_songs], fno.lfname);
                printf("Song %i: %s\n", number_of_songs, songs[number_of_songs]);
                number_of_songs++;
            }
        }
        number_of_songs = number_of_songs/2; //All files generate .__file.ext. Divide by 2 to get an accurate count
        printf("Number of songs: %i\n", number_of_songs);
        f_closedir(&directory);
    }
}

char *VS1053::getCurrentSong(){
    printf("current Song: %s\n", songs[number_of_songs]);
    return songs[current_song_index];
}

VS1053::VS1053(){}

VS1053::~VS1053(){}
