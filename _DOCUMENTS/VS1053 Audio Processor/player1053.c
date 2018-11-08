/*

  VLSI Solution generic microcontroller example player / recorder for
  VS1053.

  v1.10 2016-05-09 HH  Modified quick sanity check registers
  v1.03 2012-12-11 HH  Recording command 'p' was VS1063 only -> removed
                       Added chip type recognition
  v1.02 2012-12-04 HH  Command '_' incorrectly printed VS1063-specific fields
  v1.01 2012-11-28 HH  Untabified
  v1.00 2012-11-27 HH  First release

*/

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "player.h"
/* Download the latest VS1053a Patches package and its
   vs1053b-patches-flac.plg. If you want to use the smaller patch set
   which doesn't contain the FLAC decoder, use vs1053b-patches.plg instead.
   The patches package is available at
   http://www.vlsi.fi/en/support/software/vs10xxpatches.html */
#include "vs1053b-patches-flac.plg"


/* We also want to have the VS1053b Ogg Vorbis Encoder plugin. To get more
   than one plugin included, we'll have to include it in a slightly more
   tricky way. To get the plugin included below, download the latest version
   of the VS1053 Ogg Vorbis Encoder Application from
   http://www.vlsi.fi/en/support/software/vs10xxapplications.html */
#define SKIP_PLUGIN_VARNAME
const u_int16 encoderPlugin[] = {
#include "venc44k2q05.plg"
};
#undef SKIP_PLUGIN_VARNAME


/* VS1053b IMA ADPCM Encoder Fix, available at
   http://www.vlsi.fi/en/support/software/vs10xxpatches.html */
#define SKIP_PLUGIN_VARNAME
const u_int16 imaFix[] = {
#include "imafix.plg"
};
#undef SKIP_PLUGIN_VARNAME


#define FILE_BUFFER_SIZE 512
#define SDI_MAX_TRANSFER_SIZE 32
#define SDI_END_FILL_BYTES_FLAC 12288
#define SDI_END_FILL_BYTES       2050
#define REC_BUFFER_SIZE 512


/* How many transferred bytes between collecting data.
   A value between 1-8 KiB is typically a good value.
   If REPORT_ON_SCREEN is defined, a report is given on screen each time
   data is collected. */
#define REPORT_INTERVAL 4096
#define REPORT_INTERVAL_MIDI 512
#if 1
#define REPORT_ON_SCREEN
#endif

/* Define PLAYER_USER_INTERFACE if you want to have a user interface in your
   player. */
#if 1
#define PLAYER_USER_INTERFACE
#endif

/* Define RECORDER_USER_INTERFACE if you want to have a user interface in your
   player. */
#if 1
#define RECORDER_USER_INTERFACE
#endif


#define min(a,b) (((a)<(b))?(a):(b))



enum AudioFormat {
  afUnknown,
  afRiff,
  afOggVorbis,
  afMp1,
  afMp2,
  afMp3,
  afAacMp4,
  afAacAdts,
  afAacAdif,
  afFlac,
  afWma,
  afMidi,
} audioFormat = afUnknown;

const char *afName[] = {
  "unknown",
  "RIFF",
  "Ogg",
  "MP1",
  "MP2",
  "MP3",
  "AAC MP4",
  "AAC ADTS",
  "AAC ADIF",
  "FLAC",
  "WMA",
  "MIDI",
};


/*
  Read 32-bit increasing counter value from addr.
  Because the 32-bit value can change while reading it,
  read MSB's twice and decide which is the correct one.
*/
u_int32 ReadVS10xxMem32Counter(u_int16 addr) {
  u_int16 msbV1, lsb, msbV2;
  u_int32 res;

  WriteSci(SCI_WRAMADDR, addr+1);
  msbV1 = ReadSci(SCI_WRAM);
  WriteSci(SCI_WRAMADDR, addr);
  lsb = ReadSci(SCI_WRAM);
  msbV2 = ReadSci(SCI_WRAM);
  if (lsb < 0x8000U) {
    msbV1 = msbV2;
  }
  res = ((u_int32)msbV1 << 16) | lsb;
  
  return res;
}


/*
  Read 32-bit non-changing value from addr.
*/
u_int32 ReadVS10xxMem32(u_int16 addr) {
  u_int16 lsb;
  WriteSci(SCI_WRAMADDR, addr);
  lsb = ReadSci(SCI_WRAM);
  return lsb | ((u_int32)ReadSci(SCI_WRAM) << 16);
}


/*
  Read 16-bit value from addr.
*/
u_int16 ReadVS10xxMem(u_int16 addr) {
  WriteSci(SCI_WRAMADDR, addr);
  return ReadSci(SCI_WRAM);
}


/*
  Write 16-bit value to given VS10xx address
*/
void WriteVS10xxMem(u_int16 addr, u_int16 data) {
  WriteSci(SCI_WRAMADDR, addr);
  WriteSci(SCI_WRAM, data);
}

/*
  Write 32-bit value to given VS10xx address
*/
void WriteVS10xxMem32(u_int16 addr, u_int32 data) {
  WriteSci(SCI_WRAMADDR, addr);
  WriteSci(SCI_WRAM, (u_int16)data);
  WriteSci(SCI_WRAM, (u_int16)(data>>16));
}




static const u_int16 linToDBTab[5] = {36781, 41285, 46341, 52016, 58386};

/*
  Converts a linear 16-bit value between 0..65535 to decibels.
    Reference level: 32768 = 96dB (largest VS1053b number is 32767 = 95dB).
  Bugs:
    - For the input of 0, 0 dB is returned, because minus infinity cannot
      be represented with integers.
    - Assumes a ratio of 2 is 6 dB, when it actually is approx. 6.02 dB.
*/
static u_int16 LinToDB(unsigned short n) {
  int res = 96, i;

  if (!n)               /* No signal should return minus infinity */
    return 0;

  while (n < 32768U) {  /* Amplify weak signals */
    res -= 6;
    n <<= 1;
  }

  for (i=0; i<5; i++)   /* Find exact scale */
    if (n >= linToDBTab[i])
      res++;

  return res;
}




/*

  Loads a plugin.

  This is a slight modification of the LoadUserCode() example
  provided in many of VLSI Solution's program packages.

*/
void LoadPlugin(const u_int16 *d, u_int16 len) {
  int i = 0;

  while (i<len) {
    unsigned short addr, n, val;
    addr = d[i++];
    n = d[i++];
    if (n & 0x8000U) { /* RLE run, replicate n samples */
      n &= 0x7FFF;
      val = d[i++];
      while (n--) {
        WriteSci(addr, val);
      }
    } else {           /* Copy run, copy n samples */
      while (n--) {
        val = d[i++];
        WriteSci(addr, val);
      }
    }
  }
}









enum PlayerStates {
  psPlayback = 0,
  psUserRequestedCancel,
  psCancelSentToVS10xx,
  psStopped
} playerState;





/*

  This function plays back an audio file.

  It also contains a simple user interface, which requires the following
  funtions that you must provide:
  void SaveUIState(void);
  - saves the user interface state and sets the system up
  - may in many cases be implemented as an empty function
  void RestoreUIState(void);
  - Restores user interface state before exit
  - may in many cases be implemented as an empty function
  int GetUICommand(void);
  - Returns -1 for no operation
  - Returns -2 for cancel playback command
  - Returns any other for user input. For supported commands, see code.

*/
void VS1053PlayFile(FILE *readFp) {
  static u_int8 playBuf[FILE_BUFFER_SIZE];
  u_int32 bytesInBuffer;        // How many bytes in buffer left
  u_int32 pos=0;                // File position
  int endFillByte = 0;          // What byte value to send after file
  int endFillBytes = SDI_END_FILL_BYTES; // How many of those to send
  int playMode = ReadVS10xxMem(PAR_PLAY_MODE);
  long nextReportPos=0; // File pointer where to next collect/report
  int i;
#ifdef PLAYER_USER_INTERFACE
  static int earSpeaker = 0;    // 0 = off, other values strength
  int volLevel = ReadSci(SCI_VOL) & 0xFF; // Assume both channels at same level
  int c;
  static int rateTune = 0;      // Samplerate fine tuning in ppm
#endif /* PLAYER_USER_INTERFACE */

#ifdef PLAYER_USER_INTERFACE
  SaveUIState();
#endif /* PLAYER_USER_INTERFACE */

  playerState = psPlayback;             // Set state to normal playback

  WriteSci(SCI_DECODE_TIME, 0);         // Reset DECODE_TIME


  /* Main playback loop */

  while ((bytesInBuffer = fread(playBuf, 1, FILE_BUFFER_SIZE, readFp)) > 0 &&
         playerState != psStopped) {
    u_int8 *bufP = playBuf;

    while (bytesInBuffer && playerState != psStopped) {

      if (!(playMode & PAR_PLAY_MODE_PAUSE_ENA)) {
        int t = min(SDI_MAX_TRANSFER_SIZE, bytesInBuffer);

        // This is the heart of the algorithm: on the following line
        // actual audio data gets sent to VS10xx.
        WriteSdi(bufP, t);

        bufP += t;
        bytesInBuffer -= t;
        pos += t;
      }

      /* If the user has requested cancel, set VS10xx SM_CANCEL bit */
      if (playerState == psUserRequestedCancel) {
        unsigned short oldMode;
        playerState = psCancelSentToVS10xx;
        printf("\nSetting SM_CANCEL at file offset %ld\n", pos);
        oldMode = ReadSci(SCI_MODE);
        WriteSci(SCI_MODE, oldMode | SM_CANCEL);
      }

      /* If VS10xx SM_CANCEL bit has been set, see if it has gone
         through. If it is, it is time to stop playback. */
      if (playerState == psCancelSentToVS10xx) {
        unsigned short mode = ReadSci(SCI_MODE);
        if (!(mode & SM_CANCEL)) {
          printf("SM_CANCEL has cleared at file offset %ld\n", pos);
          playerState = psStopped;
        }
      }


      /* If playback is going on as normal, see if we need to collect and
         possibly report */
      if (playerState == psPlayback && pos >= nextReportPos) {
#ifdef REPORT_ON_SCREEN
        u_int16 sampleRate;
        u_int32 byteRate;
        u_int16 h1 = ReadSci(SCI_HDAT1);
#endif

        nextReportPos += (audioFormat == afMidi || audioFormat == afUnknown) ?
          REPORT_INTERVAL_MIDI : REPORT_INTERVAL;
        /* It is important to collect endFillByte while still in normal
           playback. If we need to later cancel playback or run into any
           trouble with e.g. a broken file, we need to be able to repeatedly
           send this byte until the decoder has been able to exit. */
        endFillByte = ReadVS10xxMem(PAR_END_FILL_BYTE);

#ifdef REPORT_ON_SCREEN
        if (h1 == 0x7665) {
          audioFormat = afRiff;
          endFillBytes = SDI_END_FILL_BYTES;
        } else if (h1 == 0x4154) {
          audioFormat = afAacAdts;
          endFillBytes = SDI_END_FILL_BYTES;
        } else if (h1 == 0x4144) {
          audioFormat = afAacAdif;
          endFillBytes = SDI_END_FILL_BYTES;
        } else if (h1 == 0x574d) {
          audioFormat = afWma;
          endFillBytes = SDI_END_FILL_BYTES;
        } else if (h1 == 0x4f67) {
          audioFormat = afOggVorbis;
          endFillBytes = SDI_END_FILL_BYTES;
        } else if (h1 == 0x664c) {
          audioFormat = afFlac;
          endFillBytes = SDI_END_FILL_BYTES_FLAC;
        } else if (h1 == 0x4d34) {
          audioFormat = afAacMp4;
          endFillBytes = SDI_END_FILL_BYTES;
        } else if (h1 == 0x4d54) {
          audioFormat = afMidi;
          endFillBytes = SDI_END_FILL_BYTES;
        } else if ((h1 & 0xffe6) == 0xffe2) {
          audioFormat = afMp3;
          endFillBytes = SDI_END_FILL_BYTES;
        } else if ((h1 & 0xffe6) == 0xffe4) {
          audioFormat = afMp2;
          endFillBytes = SDI_END_FILL_BYTES;
        } else if ((h1 & 0xffe6) == 0xffe6) {
          audioFormat = afMp1;
          endFillBytes = SDI_END_FILL_BYTES;
        } else {
          audioFormat = afUnknown;
          endFillBytes = SDI_END_FILL_BYTES_FLAC;
        }

        sampleRate = ReadSci(SCI_AUDATA);
        byteRate = ReadVS10xxMem(PAR_BYTERATE);
        /* FLAC:   byteRate = bitRate / 32
           Others: byteRate = bitRate /  8
           Here we compensate for that difference. */
        if (audioFormat == afFlac)
          byteRate *= 4;

        printf("\r%ldKiB "
               "%1ds %1.1f"
               "kb/s %dHz %s %s"
               " %04x   ",
               pos/1024,
               ReadSci(SCI_DECODE_TIME),
               byteRate * (8.0/1000.0),
               sampleRate & 0xFFFE, (sampleRate & 1) ? "stereo" : "mono",
               afName[audioFormat], h1
               );
          
        fflush(stdout);
#endif /* REPORT_ON_SCREEN */
      }
    } /* if (playerState == psPlayback && pos >= nextReportPos) */
  


    /* User interface. This can of course be completely removed and
       basic playback would still work. */

#ifdef PLAYER_USER_INTERFACE
    /* GetUICommand should return -1 for no command and -2 for CTRL-C */
    c = GetUICommand();
    switch (c) {

      /* Volume adjustment */
    case '-':
      if (volLevel < 255) {
        volLevel++;
        WriteSci(SCI_VOL, volLevel*0x101);
      }
      break;
    case '+':
      if (volLevel) {
        volLevel--;
        WriteSci(SCI_VOL, volLevel*0x101);
      }
      break;

      /* Show some interesting registers */
    case '_':
      {
        u_int32 mSec = ReadVS10xxMem32Counter(PAR_POSITION_MSEC);
        printf("\nvol %1.1fdB, MODE %04x, ST %04x, "
               "HDAT1 %04x HDAT0 %04x\n",
               -0.5*volLevel,
               ReadSci(SCI_MODE),
               ReadSci(SCI_STATUS),
               ReadSci(SCI_HDAT1),
               ReadSci(SCI_HDAT0));
        printf("  sampleCounter %lu, ",
               ReadVS10xxMem32Counter(0x1800));
        if (mSec != 0xFFFFFFFFU) {
          printf("positionMSec %lu, ", mSec);
        }
        printf("config1 0x%04x", ReadVS10xxMem(PAR_CONFIG1));
        printf("\n");
      }
      break;

      /* Adjust play speed between 1x - 4x */
    case '1':
    case '2':
    case '3':
    case '4':
      /* FF speed */
      printf("\nSet playspeed to %dX\n", c-'0');
      WriteVS10xxMem(PAR_PLAY_SPEED, c-'0');
      break;

      /* Ask player nicely to stop playing the song. */
    case 'q':
      if (playerState == psPlayback)
        playerState = psUserRequestedCancel;
      break;

      /* Forceful and ugly exit. For debug uses only. */
    case 'Q':
      RestoreUIState();
      printf("\n");
      exit(EXIT_SUCCESS);
      break;

      /* EarSpeaker spatial processing adjustment. */
    case 'e':
      earSpeaker = (earSpeaker+1) & 3;
      {
        u_int16 t = ReadSci(SCI_MODE) & ~(SM_EARSPEAKER_LO|SM_EARSPEAKER_HI);
        if (earSpeaker & 1)
          t |= SM_EARSPEAKER_LO;
        if (earSpeaker & 2)
          t |= SM_EARSPEAKER_HI;
        WriteSci(SCI_MODE, t);
      }
      printf("\nSet earspeaker to %d\n", earSpeaker);
      break;

      /* Toggle mono mode. Implemented in the VS1053b Patches package */
    case 'm':
      playMode ^= PAR_PLAY_MODE_MONO_ENA;
      printf("\nMono mode %s\n",
             (playMode & PAR_PLAY_MODE_MONO_ENA) ? "on" : "off");
      WriteVS10xxMem(PAR_PLAY_MODE, playMode);
      break;

      /* Toggle differential mode */
    case 'd':
      {
        u_int16 t = ReadSci(SCI_MODE) ^ SM_DIFF;
        printf("\nDifferential mode %s\n", (t & SM_DIFF) ? "on" : "off");
        WriteSci(SCI_MODE, t);
      }
      break;

      /* Adjust playback samplerate finetuning, this function comes from
         the VS1053b Patches package. Note that the scale is different
         in VS1053b and VS1063a! */
    case 'r':
      if (rateTune >= 0) {
        rateTune = (rateTune*0.95);
      } else {
        rateTune = (rateTune*1.05);
      }
      rateTune -= 1;
     if (rateTune < -160000)
        rateTune = -160000;
      WriteVS10xxMem(0x5b1c, 0);                 /* From VS105b Patches doc */
      WriteSci(SCI_AUDATA, ReadSci(SCI_AUDATA)); /* From VS105b Patches doc */
      WriteVS10xxMem32(PAR_RATE_TUNE, rateTune);
      printf("\nrateTune %d ppm*2\n", rateTune);
      break;
    case 'R':
      if (rateTune <= 0) {
        rateTune = (rateTune*0.95);
      } else {
        rateTune = (rateTune*1.05);
      }
      rateTune += 1;
      if (rateTune > 160000)
        rateTune = 160000;
      WriteVS10xxMem32(PAR_RATE_TUNE, rateTune);
      WriteVS10xxMem(0x5b1c, 0);                 /* From VS105b Patches doc */
      WriteSci(SCI_AUDATA, ReadSci(SCI_AUDATA)); /* From VS105b Patches doc */
      printf("\nrateTune %d ppm*2\n", rateTune);
      break;
    case '/':
      rateTune = 0;
      WriteVS10xxMem(SCI_WRAMADDR, 0x5b1c);      /* From VS105b Patches doc */
      WriteVS10xxMem(0x5b1c, 0);                 /* From VS105b Patches doc */
      WriteVS10xxMem32(PAR_RATE_TUNE, rateTune);
      printf("\nrateTune off\n");
      break;

      /* Show help */
    case '?':
      printf("\nInteractive VS1053 file player keys:\n"
             "1-4\tSet playback speed\n"
             "- +\tVolume down / up\n"
             "_\tShow current settings\n"
             "q Q\tQuit current song / program\n"
             "e\tSet earspeaker\n"
             "r R\tR rateTune down / up\n"
             "/\tRateTune off\n"
             "m\tToggle Mono\n"
             "d\tToggle Differential\n"
             );
      break;

      /* Unknown commands or no command at all */
    default:
      if (c < -1) {
        printf("Ctrl-C, aborting\n");
        fflush(stdout);
        RestoreUIState();
        exit(EXIT_FAILURE);
      }
      if (c >= 0) {
        printf("\nUnknown char '%c' (%d)\n", isprint(c) ? c : '.', c);
      }
      break;
    } /* switch (c) */
#endif /* PLAYER_USER_INTERFACE */
  } /* while ((bytesInBuffer = fread(...)) > 0 && playerState != psStopped) */


  
#ifdef PLAYER_USER_INTERFACE
  RestoreUIState();
#endif /* PLAYER_USER_INTERFACE */

  printf("\nSending %d footer %d's... ", endFillBytes, endFillByte);
  fflush(stdout);

  /* Earlier we collected endFillByte. Now, just in case the file was
     broken, or if a cancel playback command has been given, write
     lots of endFillBytes. */
  memset(playBuf, endFillByte, sizeof(playBuf));
  for (i=0; i<endFillBytes; i+=SDI_MAX_TRANSFER_SIZE) {
    WriteSdi(playBuf, SDI_MAX_TRANSFER_SIZE);
  }

  /* If the file actually ended, and playback cancellation was not
     done earlier, do it now. */
  if (playerState == psPlayback) {
    unsigned short oldMode = ReadSci(SCI_MODE);
    WriteSci(SCI_MODE, oldMode | SM_CANCEL);
    printf("ok. Setting SM_CANCEL, waiting... ");
    fflush(stdout);
    while (ReadSci(SCI_MODE) & SM_CANCEL)
      WriteSdi(playBuf, 2);
  }

  /* That's it. Now we've played the file as we should, and left VS10xx
     in a stable state. It is now safe to call this function again for
     the next song, and again, and again... */
  printf("ok\n");
}









u_int8 adpcmHeader[60] = {
  'R', 'I', 'F', 'F',
  0xFF, 0xFF, 0xFF, 0xFF,
  'W', 'A', 'V', 'E',
  'f', 'm', 't', ' ',
  0x14, 0, 0, 0,          /* 20 */
  0x11, 0,                /* IMA ADPCM */
  0x1, 0,                 /* chan */
  0x0, 0x0, 0x0, 0x0,     /* sampleRate */
  0x0, 0x0, 0x0, 0x0,     /* byteRate */
  0, 1,                   /* blockAlign */
  4, 0,                   /* bitsPerSample */
  2, 0,                   /* byteExtraData */
  0xf9, 0x1,              /* samplesPerBlock = 505 */
  'f', 'a', 'c', 't',     /* subChunk2Id */
  0x4, 0, 0, 0,           /* subChunk2Size */
  0xFF, 0xFF, 0xFF, 0xFF, /* numOfSamples */
  'd', 'a', 't', 'a',
  0xFF, 0xFF, 0xFF, 0xFF
};

u_int8 pcmHeader[44] = {
  'R', 'I', 'F', 'F',
  0xFF, 0xFF, 0xFF, 0xFF,
  'W', 'A', 'V', 'E',
  'f', 'm', 't', ' ',
  0x10, 0, 0, 0,          /* 16 */
  0x1, 0,                 /* PCM */
  0x1, 0,                 /* chan */
  0x0, 0x0, 0x0, 0x0,     /* sampleRate */
  0x0, 0x0, 0x0, 0x0,     /* byteRate */
  2, 0,                   /* blockAlign */
  0x10, 0,                /* bitsPerSample */
  'd', 'a', 't', 'a',
  0xFF, 0xFF, 0xFF, 0xFF
};

void Set32(u_int8 *d, u_int32 n) {
  int i;
  for (i=0; i<4; i++) {
    *d++ = (u_int8)n;
    n >>= 8;
  }
}

void Set16(u_int8 *d, u_int16 n) {
  int i;
  for (i=0; i<2; i++) {
    *d++ = (u_int8)n;
    n >>= 8;
  }
}


/*
  This function records an audio file in Ogg, MP3, or WAV formats.
  If recording in WAV format, it updates the RIFF length headers
  after recording has finished.
*/
void VS1053RecordFile(FILE *writeFp) {
  static u_int8 recBuf[REC_BUFFER_SIZE];
  u_int32 nextReportPos=0;      // File pointer where to next collect/report
  u_int32 fileSize = 0;
  int volLevel = ReadSci(SCI_VOL) & 0xFF;
  int c;
  int ch = 2;
  int adpcm = 0;
  int dataNeededInBuffer = REC_BUFFER_SIZE;  /* max size of IMA ADPCM block */
  int adpcmBlocksPerWrite = 2/ch;
  u_int32 adpcmBlocks = 0;
  u_int16 sampleRate = 8000;


  playerState = psPlayback;

  printf("VS1053RecordFile\n");

  /* Initialize recording */

  /* Set clock to a known, high value. */
  WriteSci(SCI_CLOCKF,
           HZ_TO_SC_FREQ(12288000) | SC_MULT_53_45X | SC_ADD_53_00X);

#if 1
  /* Ogg Vorbis recording from line in. */
  dataNeededInBuffer = 2;

  /* First reset VS1053 to remove any patches. */
  WriteSci(SCI_MODE, ReadSci(SCI_MODE) | SM_RESET);

  /* Disable interrupts as instructed in the VS1053b Ogg Vorbis Encoder
     documentation. */
  WriteVS10xxMem(0xc01a, 0x2);

  /* Load the plugin */
  LoadPlugin(encoderPlugin, sizeof(encoderPlugin)/sizeof(encoderPlugin[0]));

  /* Turn SCI_MODE bits. */
  WriteSci(SCI_MODE, ReadSci(SCI_MODE) | SM_ADPCM | SM_LINE1);

  WriteSci(SCI_RECGAIN,   1024); /* 1024 = gain 1 = best quality */
  WriteSci(SCI_AICTRL3, 0);

  /* Activate recording */
  WriteSci(SCI_AIADDR, 0x34);

  /* Check what samplerate the plugin is running the ADC. This is not
     necessarily the same as recording samplerate. E.g. at a 44100 Hz
     profile this will read as 48000 Hz. */
  sampleRate = ReadSci(SCI_AUDATA) & ~1;

  /* Reset VU meter */
  WriteSci(SCI_AICTRL0, 0x8080);

  audioFormat = afOggVorbis;
#elif 1
  /* Voice quality ADPCM recording from left channel at 8 kHz.
     This will result in a 32.44 kbit/s bitstream. */
  sampleRate = 8000;
  ch = 1;

  adpcmBlocksPerWrite = 2/ch;
  adpcm = 1;

  WriteSci(SCI_RECRATE, sampleRate);
  WriteSci(SCI_RECGAIN,          0); /* 1024 = gain 1 = best quality */
  WriteSci(SCI_RECMAXAUTO,    4096); /* if RECGAIN = 0, define max auto gain */
  if (ch == 2) {
    WriteSci(SCI_RECMODE,
             RM_53_FORMAT_IMA_ADPCM | RM_53_ADC_MODE_JOINT_AGC_STEREO);
  } else {
    WriteSci(SCI_RECMODE, RM_53_FORMAT_IMA_ADPCM | RM_53_ADC_MODE_LEFT);
  }
  /* Fill values according to VS1053b Datasheet Chapter "Adding
     an IMA ADPCM RIFF Header". */
  Set16(adpcmHeader+22, ch);
  Set32(adpcmHeader+24, sampleRate);
  Set32(adpcmHeader+28, (u_int32)sampleRate*ch*256/505);
  Set16(adpcmHeader+32, 256*ch);
  fwrite(adpcmHeader, sizeof(adpcmHeader), 1, writeFp);
  fileSize = sizeof(adpcmHeader);

  /* Start the encoder */
  WriteSci(SCI_MODE, ReadSci(SCI_MODE) | SM_LINE1 | SM_ADPCM | SM_RESET);
  LoadPlugin(imaFix, sizeof(imaFix)/sizeof(imaFix[0]));

  audioFormat = afRiff;
#else
  /* HiFi stereo quality PCM recording in stereo 48 kHz.
     This will result in a really fast 1536 kbit/s bitstream. Because
     there is a 100% overhead in reading from SCI, and because the data
     often has to be written to an SD card or similar using the same
     bus, the SPi speed must be really high and the software streamlined
     for there to be a chance for uninterrupted recording.

     For the absolute best quality possible on VS1053, you should use
     the VS1053 WAV PCM Recorder Application, available at
     http://www.vlsi.fi/en/support/software/vs10xxapplications.html */
  sampleRate = 48000;
  ch = 2;

  WriteSci(SCI_RECRATE, sampleRate);
  WriteSci(SCI_RECGAIN,          0); /* 1024 = gain 1 = best quality */
  WriteSci(SCI_RECMAXAUTO,    4096); /* if RECGAIN = 0, define max auto gain */
  if (ch == 2) {
    WriteSci(SCI_RECMODE, RM_53_FORMAT_PCM | RM_53_ADC_MODE_JOINT_AGC_STEREO);
  } else {
    WriteSci(SCI_RECMODE, RM_53_FORMAT_PCM | RM_53_ADC_MODE_LEFT);
  }
  /* Fill values according to VS1053b Datasheet Chapter "Adding
     a PCM RIFF Header. */
  Set16(pcmHeader+22, ch);
  Set32(pcmHeader+24, sampleRate);
  Set32(pcmHeader+28, 2L*sampleRate*ch);
  Set16(pcmHeader+32, 2*ch);
  fwrite(pcmHeader, sizeof(pcmHeader), 1, writeFp);
  fileSize = sizeof(pcmHeader);

  /* Start the encoder */
  WriteSci(SCI_MODE, ReadSci(SCI_MODE) | SM_LINE1 | SM_ADPCM | SM_RESET);
  LoadPlugin(imaFix, sizeof(imaFix)/sizeof(imaFix[0]));

  audioFormat = afRiff;
#endif



#ifdef RECORDER_USER_INTERFACE
  SaveUIState();
#endif /* RECORDER_USER_INTERFACE */

  while (playerState != psStopped) {
    int n;

#ifdef RECORDER_USER_INTERFACE
    {
      c = GetUICommand();
      
      switch(c) {
      case 'q':
        if (playerState == psPlayback) {
          printf("\nSwitching encoder off...\n");
          if (audioFormat == afOggVorbis) {
            WriteSci(SCI_AICTRL3, ReadSci(SCI_AICTRL3) | 1);
            playerState = psUserRequestedCancel;
          } else {
            playerState = psStopped;
          }
        }
        break;
      case '-':
        if (volLevel < 255) {
          volLevel++;
          WriteSci(SCI_VOL, volLevel*0x101);
        }
        break;
      case '+':
        if (volLevel) {
          volLevel--;
          WriteSci(SCI_VOL, volLevel*0x101);
        }
        break;
        break;
      case '_':
        printf("\nvol %4.1f\n", -0.5*volLevel);
        if (audioFormat == afOggVorbis) {
          printf("sampleCounter %ld\n", ReadVS10xxMem32Counter(0x1800));
        }
        break;
      case '?':
        printf("\nInteractive VS1053 file recorder keys:\n"
               "- +\tVolume down / up\n"
               "_\tShow current settings\n"
               "q\tQuit recording\n"
               );
        break;
      default:
        if (c < -1) {
          printf("Ctrl-C, aborting\n");
          fflush(stdout);
          RestoreUIState();
          exit(EXIT_FAILURE);
        }
        if (c >= 0) {
          printf("\nUnknown char '%c' (%d)\n", isprint(c) ? c : '.', c);
        }
        break;  
      }
      
    }
#endif /* RECORDER_USER_INTERFACE */


    /* See if there is some data available */
    if ((n = ReadSci(SCI_RECWORDS)) > dataNeededInBuffer) {
      int i;
      u_int8 *rbp = recBuf;

      if (audioFormat == afOggVorbis) {
        /* Always leave at least one word unread if Ogg Vorbis format */
        n = min(n-1, REC_BUFFER_SIZE/2);
      } else {
        /* Always writes one or two IMA ADPCM block(s) at a time */
        n = dataNeededInBuffer/2;
        adpcmBlocks += adpcmBlocksPerWrite;
      }
      if (audioFormat == afOggVorbis || adpcm) {
        for (i=0; i<n; i++) {
          u_int16 w = ReadSci(SCI_RECDATA);
          *rbp++ = (u_int8)(w >> 8);
          *rbp++ = (u_int8)(w & 0xFF);
        }
      } else {
        /* Make little-endian conversion for 16-bit PCM .WAV files */
        for (i=0; i<n; i++) {
          u_int16 w = ReadSci(SCI_RECDATA);
          *rbp++ = (u_int8)(w & 0xFF);
          *rbp++ = (u_int8)(w >> 8);
        }
      }
      fwrite(recBuf, 1, 2*n, writeFp);
      fileSize += 2*n;
    } else {
      /* This code is only for Ogg Vorbis recording. */
      if (playerState == psUserRequestedCancel && (ReadSci(SCI_AICTRL3) & 2)) {
        playerState = psStopped;
      }
    }

    if (fileSize - nextReportPos >= REPORT_INTERVAL) {
      nextReportPos += REPORT_INTERVAL;
      printf("\r%ldKiB ", fileSize/1024);
      if (audioFormat == afOggVorbis) {
        printf("%lds ", ReadVS10xxMem32Counter(0x8));
      }
      printf("%uHz %s %s ",
             sampleRate, (ch == 2) ? "stereo" : "mono", afName[audioFormat]);
      if (audioFormat == afOggVorbis) {
        printf("%3.1f kbit/s, ", ReadVS10xxMem32(0xC) * 0.001);
        /* Read VU meter and determine from here if the Ogg file has been
           stereo or mono. */
        u_int16 lr = ReadSci(SCI_AICTRL0);
        if ((lr & 0x8080) == 0x8080) {
          printf("l ???dB, r ???dB %04x ", lr);
        } else {
          WriteSci(SCI_AICTRL0, 0x8080);
          if (lr & 0x80) {
            ch = 1;
            printf("vu %3ddB ",
                   LinToDB(lr & 0x7F00)-95);
          } else {
            ch = 2;
            printf("l %3ddB, r %3ddB ",
                   LinToDB(lr & 0x7F00)-95,
                   LinToDB(256 * (lr&0x7F))-95);
          }
        }
      }
      fflush(stdout);
    }
  } /* while (playerState != psStopped) */


  if (audioFormat == afOggVorbis) {
    /* Correctly read and write final bytes of an Ogg Vorbis file */
    int wordsLeft = ReadSci(SCI_RECWORDS);
    while (wordsLeft--) {
      u_int16 w = ReadSci(SCI_RECDATA);
      u_int16 toWrite = 2;
      recBuf[0] = (u_int8)(w >> 8);
      recBuf[1] = (u_int8)(w & 0xFF);
      if (!wordsLeft) {
        ReadSci(SCI_AICTRL3);
        w = ReadSci(SCI_AICTRL3);
        if (w & 4) {
          toWrite = 1;
          printf("\nOdd length Ogg Vorbis recording\n");
        } else {
          printf("\nEven length Ogg Vorbis recording\n");
        }
      }
      fwrite(recBuf, 1, toWrite, writeFp);
    }
  } else if (adpcm) {
    /* Update file sizes for an RIFF IMA ADPCM .WAV file */
    fseek(writeFp, 0, SEEK_SET);
    Set32(adpcmHeader+4, fileSize-8);
    Set32(adpcmHeader+48, adpcmBlocks*505);
    Set32(adpcmHeader+56, fileSize-60);
    fwrite(adpcmHeader, sizeof(adpcmHeader), 1, writeFp);
  } else {
    /* Update file sizes for an RIFF PCM .WAV file */
    fseek(writeFp, 0, SEEK_SET);
    Set32(pcmHeader+4, fileSize-8);
    Set32(pcmHeader+40, fileSize-36);
    fwrite(pcmHeader, sizeof(pcmHeader), 1, writeFp);

  }

#ifdef RECORDER_USER_INTERFACE
  RestoreUIState();
#endif /* RECORDER_USER_INTERFACE */

  /* Finally, reset the VS10xx software, including realoading the
     patches package, to make sure everything is set up properly. */
  VSTestInitSoftware();

  printf("ok\n");
}





/*

  Hardware Initialization for VS1053.

  
*/
int VSTestInitHardware(void) {
  /* Write here your microcontroller code which puts VS10xx in hardware
     reset anc back (set xRESET to 0 for at least a few clock cycles,
     then to 1). */
  return 0;
}



/* Note: code SS_VER=2 is used for both VS1002 and VS1011e */
const u_int16 chipNumber[16] = {
  1001, 1011, 1011, 1003, 1053, 1033, 1063, 1103,
  0, 0, 0, 0, 0, 0, 0, 0
};

/*

  Software Initialization for VS1053.

  Note that you need to check whether SM_SDISHARE should be set in
  your application or not.
  
*/
int VSTestInitSoftware(void) {
  u_int16 ssVer;

  /* Start initialization with a dummy read, which makes sure our
     microcontoller chips selects and everything are where they
     are supposed to be and that VS10xx's SCI bus is in a known state. */
  ReadSci(SCI_MODE);

  /* First real operation is a software reset. After the software
     reset we know what the status of the IC is. You need, depending
     on your application, either set or not set SM_SDISHARE. See the
     Datasheet for details. */
  WriteSci(SCI_MODE, SM_SDINEW|SM_SDISHARE|SM_TESTS|SM_RESET);

  /* A quick sanity check: write to two registers, then test if we
     get the same results. Note that if you use a too high SPI
     speed, the MSB is the most likely to fail when read again. */
  WriteSci(SCI_AICTRL1, 0xABAD);
  WriteSci(SCI_AICTRL2, 0x7E57);
  if (ReadSci(SCI_AICTRL1) != 0xABAD || ReadSci(SCI_AICTRL2) != 0x7E57) {
    printf("There is something wrong with VS10xx SCI registers\n");
    return 1;
  }
  WriteSci(SCI_AICTRL1, 0);
  WriteSci(SCI_AICTRL2, 0);

  /* Check VS10xx type */
  ssVer = ((ReadSci(SCI_STATUS) >> 4) & 15);
  if (chipNumber[ssVer]) {
    printf("Chip is VS%d\n", chipNumber[ssVer]);
    if (chipNumber[ssVer] != 1053) {
      printf("Incorrect chip\n");
      return 1;
    }
  } else {
    printf("Unknown VS10xx SCI_MODE field SS_VER = %d\n", ssVer);
    return 1;
  }

  /* Set the clock. Until this point we need to run SPI slow so that
     we do not exceed the maximum speeds mentioned in
     Chapter SPI Timing Diagram in the Datasheet. */
  WriteSci(SCI_CLOCKF,
           HZ_TO_SC_FREQ(12288000) | SC_MULT_53_35X | SC_ADD_53_10X);


  /* Now when we have upped the VS10xx clock speed, the microcontroller
     SPI bus can run faster. Do that before you start playing or
     recording files. */

  /* Set up other parameters. */
  WriteVS10xxMem(PAR_CONFIG1, PAR_CONFIG1_AAC_SBR_SELECTIVE_UPSAMPLE);

  /* Set volume level at -6 dB of maximum */
  WriteSci(SCI_VOL, 0x0c0c);

  /* Now it's time to load the proper patch set. */
  LoadPlugin(plugin, sizeof(plugin)/sizeof(plugin[0]));

  /* We're ready to go. */
  return 0;
}





/*
  Main function that activates either playback or recording.
*/
int VSTestHandleFile(const char *fileName, int record) {
  if (!record) {
    FILE *fp = fopen(fileName, "rb");
    printf("Play file %s\n", fileName);
    if (fp) {
      VS1053PlayFile(fp);
    } else {
      printf("Failed opening %s for reading\n", fileName);
      return -1;
    }
  } else {
    FILE *fp = fopen(fileName, "wb");
    printf("Record file %s\n", fileName);
    if (fp) {
      VS1053RecordFile(fp);
    } else {
      printf("Failed opening %s for writing\n", fileName);
      return -1;
    }
  }
  return 0;
}
