#ifndef buzzer_H
#define buzzer_H

/*---------------------INCLUDES----------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "tim.h"
// #include "struct_typedef.h"
/*---------------------DEFINES-----------------------*/
//
//#define SILENT_MODE // 取消注释以关闭所有提示音（有bug！！！）

#define PLAYING_STOP 0
#define PLAYING_INIT_MUSIC 1

#define PLAYING_WARNING_SOUND 11
#define PLAYING_ERROR_SOUND 12

#define PLAYING_GYROCALIB_SOUND 21
#define PLAYING_AUTOAIMING_SOUND 22

/*---------------------DECLARES----------------------*/
//
extern uint8_t buzzer_state;

//
extern const uint16_t song_littlestar[];
extern const uint16_t song_happybirthday[];
extern const uint16_t song_eastred[];

extern const uint16_t song_robomasteryou[];
extern const uint16_t song_robomasteryou2[];
extern const uint16_t song_robomasterlickdog[];

//
extern const uint8_t sound_warning[];
extern const uint8_t sound_error[];
extern const uint8_t sound_gyrocalibrating[];
extern const uint8_t sound_autoaiming[];

//
void systemstart_song(void);

//
void buzzerTask(void const *argument);

void SongLaoda(void);

void SongSpring(void);

//
extern void SetBuzzerOff(void);

extern void SetBuzzerFrequence(uint16_t freq);


extern void buzzer_on(uint16_t psc, uint16_t pwm);

extern void buzzer_off(void);

extern void buzzer_note(uint16_t note, float volume);

//
extern int8_t SetBuzzerState(uint8_t state);

extern void PlayingSong(const uint16_t *song, uint16_t len);

extern void PlayingSound(const uint8_t *sound, uint16_t len);

#endif
