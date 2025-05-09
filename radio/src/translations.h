/*
 * Copyright (C) OpenTX
 *
 * Based on code named
 *   th9x - http://code.google.com/p/th9x
 *   er9x - http://code.google.com/p/er9x
 *   gruvin9x - http://code.google.com/p/gruvin9x
 *
 * License GPLv2: http://www.gnu.org/licenses/gpl-2.0.html
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 */

#ifndef _TRANSLATIONS_H_
#define _TRANSLATIONS_H_

#if defined(TRANSLATIONS_FR)
  #include "translations/fr.h"
  #define LEN_SPECIAL_CHARS 3
#elif defined(TRANSLATIONS_IT)
  #include "translations/it.h"
  #define LEN_SPECIAL_CHARS 1
#elif defined(TRANSLATIONS_SE)
  #include "translations/se.h"
  #define LEN_SPECIAL_CHARS 6
#elif defined(TRANSLATIONS_FI)
  #include "translations/fi.h"
  #define LEN_SPECIAL_CHARS 6
#elif defined(TRANSLATIONS_DE)
  #include "translations/de.h"
  #define LEN_SPECIAL_CHARS 6
#elif defined(TRANSLATIONS_CZ)
  #include "translations/cz.h"
  #define LEN_SPECIAL_CHARS 17
#elif defined(TRANSLATIONS_ES)
  #include "translations/es.h"
  #define LEN_SPECIAL_CHARS 0
#elif defined(TRANSLATIONS_PL)
  #include "translations/pl.h"
  #define LEN_SPECIAL_CHARS 0
#elif defined(TRANSLATIONS_PT)
  #include "translations/pt.h"
  #define LEN_SPECIAL_CHARS 0
#elif defined(TRANSLATIONS_NL)
  #include "translations/nl.h"
  #define LEN_SPECIAL_CHARS 0
#else
  #include "translations/en.h"
  #define LEN_SPECIAL_CHARS 0
#endif

#define PSIZE(x) ( sizeof(x) - 1 )
#define EOFS(x)  ( OFS_##x + sizeof(TR_##x) )

#if defined(PCBTARANIS)
  #define HINT(s) " " s " "
#else
  #define HINT(s) s
#endif

#if LCD_W >= 480
  #define TR3(x, y, z) z
  #define TR2(x, y) y
#elif LCD_W >= 212
  #define TR3(x, y, z) y
  #define TR2(x, y) y
#else
  #define TR3(x, y, z) x
  #define TR2(x, y) x
#endif

#define TR(x, y) TR2(x, y) // for compatibility

#if defined(COLORLCD)
  #define BUTTON(x) x
#else
  #define BUTTON(x) "[" x "]"
#endif

#if defined(PCBHORUS)
  #define TR_VTRAINERMODES     TR_VTRAINER_MASTER_JACK TR_VTRAINER_SLAVE_JACK TR_VTRAINER_MASTER_BATTERY TR_VTRAINER_BLUETOOTH
#elif defined(PCBX9E)
  #define TR_VTRAINERMODES     TR_VTRAINER_MASTER_JACK TR_VTRAINER_SLAVE_JACK TR_VTRAINER_MASTER_SBUS_MODULE TR_VTRAINER_MASTER_CPPM_MODULE TR_VTRAINER_MASTER_BATTERY TR_VTRAINER_BLUETOOTH
#elif defined(PCBTARANIS) && defined(BLUETOOTH)
  #define TR_VTRAINERMODES     TR_VTRAINER_MASTER_JACK TR_VTRAINER_SLAVE_JACK TR_VTRAINER_MASTER_SBUS_MODULE TR_VTRAINER_MASTER_CPPM_MODULE TR_VTRAINER_MASTER_BATTERY TR_VTRAINER_BLUETOOTH
#elif defined(PCBTARANIS)
  #define TR_VTRAINERMODES     TR_VTRAINER_MASTER_JACK TR_VTRAINER_SLAVE_JACK TR_VTRAINER_MASTER_SBUS_MODULE TR_VTRAINER_MASTER_CPPM_MODULE TR_VTRAINER_MASTER_BATTERY
#elif defined(PCBI6X)
  #if defined(SBUS_TRAINER)
  #define TR_VTRAINERMODES     TR_VTRAINER_MASTER_JACK TR_VTRAINER_MASTER_BATTERY
  #else
  #define TR_VTRAINERMODES     ""
  #endif
#else
  #define TR_VTRAINERMODES     TR_VTRAINER_MASTER_JACK TR_VTRAINER_SLAVE_JACK TR_VTRAINER_MASTER_CPPM_MODULE TR_VTRAINER_MASTER_BATTERY
#endif

#if (LCD_W == 212)
 #define LCDW_128_480_LINEBREAK        ""
#else
 #define LCDW_128_480_LINEBREAK        "\036"
#endif

// The non-0-terminated-strings

extern const char STR_OPEN9X[];

#define OFS_OFFON              0
#define OFS_MMMINV             (OFS_OFFON + sizeof(TR_OFFON))
#define OFS_NCHANNELS          (OFS_MMMINV + sizeof(TR_MMMINV))
#if defined(GRAPHICS)
  #define OFS_VBEEPMODE          (OFS_NCHANNELS + sizeof(TR_NCHANNELS))
#else
  #define OFS_VBEEPLEN           (OFS_NCHANNELS + sizeof(TR_NCHANNELS))
  #define OFS_VBEEPMODE          (OFS_VBEEPLEN + sizeof(TR_VBEEPLEN))
#endif
#if defined(ROTARY_ENCODERS)
  #define OFS_VRENAVIG           (OFS_VBEEPMODE + sizeof(TR_VBEEPMODE))
  #define OFS_VRENCODERS         (OFS_VRENAVIG + sizeof(TR_VRENAVIG))
  #define OFS_TRNMODE            (OFS_VRENCODERS + sizeof(TR_VRENCODERS))
#elif defined(ROTARY_ENCODER_NAVIGATION)
  #define OFS_VRENCODERS         (OFS_VBEEPMODE + sizeof(TR_VBEEPMODE))
  #define OFS_TRNMODE            (OFS_VRENCODERS + sizeof(TR_VRENCODERS))
#else
  #define OFS_TRNMODE            (OFS_VBEEPMODE + sizeof(TR_VBEEPMODE))
#endif
#define OFS_TRNCHN             (OFS_TRNMODE + sizeof(TR_TRNMODE))
#if defined(PCBTARANIS) || defined(PCBHORUS) || defined(PCBI6X)
  #define OFS_AUX_SERIAL_MODES       (OFS_TRNCHN + sizeof(TR_TRNCHN))
  #define OFS_SWTYPES          (OFS_AUX_SERIAL_MODES + sizeof(TR_AUX_SERIAL_MODES))
  #define OFS_POTTYPES         (OFS_SWTYPES + sizeof(TR_SWTYPES))
  #define OFS_SLIDERTYPES      (OFS_POTTYPES + sizeof(TR_POTTYPES))
#if NUM_SLIDERS > 0
  #define OFS_VTRIMINC         (OFS_SLIDERTYPES + sizeof(TR_SLIDERTYPES))
#else
  #define OFS_VTRIMINC         (OFS_SLIDERTYPES)
#endif
#else
  #define OFS_VTRIMINC         (OFS_TRNCHN + sizeof(TR_TRNCHN))
#endif
  #define OFS_VDISPLAYTRIMS    (OFS_VTRIMINC + sizeof(TR_VTRIMINC))
  #define OFS_RETA123          (OFS_VDISPLAYTRIMS + sizeof(TR_VDISPLAYTRIMS))
#define OFS_VPROTOS            (OFS_RETA123 + sizeof(TR_RETA123))
#define OFS_POSNEG             (OFS_VPROTOS + sizeof(TR_VPROTOS))
#define OFS_VBLMODE          (OFS_POSNEG + sizeof(TR_POSNEG))
#define OFS_VCURVEFUNC         (OFS_VBLMODE + sizeof(TR_VBLMODE))
#define OFS_VMLTPX             (OFS_VCURVEFUNC + sizeof(TR_VCURVEFUNC))
#define OFS_VMLTPX2            (OFS_VMLTPX + sizeof(TR_VMLTPX))
#define OFS_VMIXTRIMS          (OFS_VMLTPX2 + sizeof(TR_VMLTPX2))
#define OFS_VCSWFUNC           (OFS_VMIXTRIMS + sizeof(TR_VMIXTRIMS))
#define OFS_VFSWFUNC           (OFS_VCSWFUNC + sizeof(TR_VCSWFUNC))
#define OFS_VFSWRESET          (OFS_VFSWFUNC + sizeof(TR_VFSWFUNC))
#define OFS_FUNCSOUNDS         (OFS_VFSWRESET + sizeof(TR_VFSWRESET))
#define OFS_VTELEMCHNS         (OFS_FUNCSOUNDS + sizeof(TR_FUNCSOUNDS))
#define OFS_VTELEMUNIT         (OFS_VTELEMCHNS)
#if !defined(PCBI6X)
#define OFS_GPSFORMAT          (OFS_VTELPROTO + sizeof(TR_VTELPROTO))
#define OFS_AMPSRC             (OFS_GPSFORMAT + sizeof(TR_GPSFORMAT))
#endif
#define OFS_VSCREEN            (OFS_VTELEMUNIT + sizeof(TR_VTELEMUNIT))
#define OFS_VTEMPLATES         (OFS_VSCREEN + sizeof(TR_VTELEMSCREENTYPE))
#define OFS_VSWASHTYPE         (OFS_VTEMPLATES)
#if defined(HELI)
  #define OFS_VKEYS             (OFS_VSWASHTYPE + sizeof(TR_VSWASHTYPE))
#else
  #define OFS_VKEYS             (OFS_VSWASHTYPE)
#endif
#if defined(MENU_DIAG_ANAS_KEYS)
#define OFS_VSWITCHES           (OFS_VKEYS + sizeof(TR_VKEYS))
#else
#define OFS_VSWITCHES           (OFS_VKEYS)
#endif
#define OFS_VSRCRAW             (OFS_VSWITCHES + sizeof(TR_VSWITCHES))
#if defined(TRANSLATIONS_CZ)
  #define OFS_INPUTNAMES          (OFS_VSRCRAW + sizeof(TR_VSRCRAW))
  #define OFS_VTMRMODES           (OFS_INPUTNAMES + sizeof(TR_INPUTNAMES))
#else
  #define OFS_VTMRMODES           (OFS_VSRCRAW + sizeof(TR_VSRCRAW))
#endif
#define OFS_DATETIME            (OFS_VTMRMODES + sizeof(TR_VTMRMODES))
#if defined(RTCCLOCK)
#define OFS_VPERSISTENT         (OFS_DATETIME + sizeof(TR_DATETIME))
#else
#define OFS_VPERSISTENT         (OFS_DATETIME)
#endif
#if !defined(PCBI6X)
  #define OFS_VLCD                (OFS_VPERSISTENT + sizeof(TR_VPERSISTENT))
  #define OFS_VUNITSSYSTEM        (OFS_VLCD + sizeof(TR_VLCD))
  #define OFS_VBEEPCOUNTDOWN      (OFS_VUNITSSYSTEM + sizeof(TR_VUNITSSYSTEM))
#else
  #define OFS_VBEEPCOUNTDOWN      (OFS_VPERSISTENT + sizeof(TR_VPERSISTENT))
#endif
#define OFS_VVARIOCENTER        (OFS_VBEEPCOUNTDOWN + sizeof(TR_VBEEPCOUNTDOWN))
#if !defined(PCBI6X)
  #define OFS_COUNTRYCODES        (OFS_VVARIOCENTER + sizeof(TR_VVARIOCENTER))
  #define OFS_USBMODES            (OFS_COUNTRYCODES + sizeof(TR_COUNTRYCODES))
#else
  #define OFS_USBMODES            (OFS_VVARIOCENTER + sizeof(TR_VVARIOCENTER))
#endif
#define OFS_VFAILSAFE           (OFS_USBMODES + sizeof(TR_USBMODES))
#define OFS_VTRAINERMODES       (OFS_VFAILSAFE + sizeof(TR_VFAILSAFE))
#define OFS_TARANIS_PROTOCOLS        (OFS_VTRAINERMODES + sizeof(TR_VTRAINERMODES))
#if !defined(PCBI6X)
  #define OFS_R9M_REGION               (OFS_TARANIS_PROTOCOLS + sizeof(TR_TARANIS_PROTOCOLS))
  #define OFS_R9M_FCC_POWER_VALUES     (OFS_R9M_REGION + sizeof(TR_R9M_REGION))
  #define OFS_R9M_LBT_POWER_VALUES     (OFS_R9M_FCC_POWER_VALUES + sizeof(TR_R9M_FCC_POWER_VALUES))
  #define OFS_TELEMETRY_PROTOCOLS      (OFS_R9M_LBT_POWER_VALUES + sizeof(TR_R9M_LBT_POWER_VALUES))
#else
  #define OFS_TELEMETRY_PROTOCOLS      (OFS_TARANIS_PROTOCOLS + sizeof(TR_TARANIS_PROTOCOLS))
#endif
#if !defined(PCBI6X)
  #define OFS_XJT_PROTOCOLS            (OFS_TELEMETRY_PROTOCOLS + sizeof(TR_TELEMETRY_PROTOCOLS))
  #define OFS_DSM_PROTOCOLS            (OFS_XJT_PROTOCOLS + sizeof(TR_XJT_PROTOCOLS))
  #define OFS_I6X_PROTOCOLS            (OFS_DSM_PROTOCOLS + sizeof(TR_DSM_PROTOCOLS))
#else
  #define OFS_I6X_PROTOCOLS            (OFS_TELEMETRY_PROTOCOLS)
#endif
#if defined(MULTIMODULE)
  #define OFS_MULTI_PROTOCOLS   (OFS_I6X_PROTOCOLS + sizeof(TR_I6X_PROTOCOLS))
#endif
  #define OFS_CURVE_TYPES       (OFS_I6X_PROTOCOLS + sizeof(TR_I6X_PROTOCOLS))
  #define OFS_CRSF_ARMING_MODES (OFS_CURVE_TYPES + sizeof(TR_CURVE_TYPES))
  #define OFS_VSENSORTYPES      (OFS_CRSF_ARMING_MODES + sizeof(TR_CRSF_ARMING_MODES))
  #define OFS_VFORMULAS         (OFS_VSENSORTYPES + sizeof(TR_VSENSORTYPES))
  #define OFS_VPREC             (OFS_VFORMULAS + sizeof(TR_VFORMULAS))
  #define OFS_VCELLINDEX        (OFS_VPREC + sizeof(TR_VPREC))
#if defined(BLUETOOTH)
  #define OFS_BLUETOOTH_MODES   (OFS_VCELLINDEX + sizeof(TR_VCELLINDEX))
  #define OFS_VANTENNATYPES     (OFS_BLUETOOTH_MODES + sizeof(TR_BLUETOOTH_MODES))
#else
  #define OFS_VANTENNATYPES     (OFS_VCELLINDEX + sizeof(TR_VCELLINDEX))
#endif
  #define OFS_MAVLINK_BAUDS     (OFS_VANTENNATYPES + sizeof(TR_VANTENNATYPES))
  #define OFS_SPARE		(OFS_MAVLINK_BAUDS)

#define STR_OFFON               (STR_OPEN9X + OFS_OFFON)
#define STR_MMMINV              (STR_OPEN9X + OFS_MMMINV)
#define STR_NCHANNELS           (STR_OPEN9X + OFS_NCHANNELS)
#if !defined(GRAPHICS)
#define STR_VBEEPLEN            (STR_OPEN9X + OFS_VBEEPLEN)
#endif
#define STR_VBEEPMODE           (STR_OPEN9X + OFS_VBEEPMODE)
#define STR_TRNMODE             (STR_OPEN9X + OFS_TRNMODE)
#define STR_TRNCHN              (STR_OPEN9X + OFS_TRNCHN)
#define STR_AUX_SERIAL_MODES      (STR_OPEN9X + OFS_AUX_SERIAL_MODES)
#define STR_SWTYPES             (STR_OPEN9X + OFS_SWTYPES)
#define STR_POTTYPES            (STR_OPEN9X + OFS_POTTYPES)
#define STR_SLIDERTYPES         (STR_OPEN9X + OFS_SLIDERTYPES)
#define STR_VTRIMINC            (STR_OPEN9X + OFS_VTRIMINC)
#define STR_VDISPLAYTRIMS       (STR_OPEN9X + OFS_VDISPLAYTRIMS)
#define STR_RETA123             (STR_OPEN9X + OFS_RETA123)
#define STR_VPROTOS             (STR_OPEN9X + OFS_VPROTOS)
#define STR_POSNEG              (STR_OPEN9X + OFS_POSNEG)
#define STR_VBLMODE             (STR_OPEN9X + OFS_VBLMODE)
#define STR_VCURVEFUNC          (STR_OPEN9X + OFS_VCURVEFUNC)
#define STR_VSIDE               STR_VCURVEFUNC
#define LEN_VSIDE               LEN_VCURVEFUNC
#define STR_VMLTPX              (STR_OPEN9X + OFS_VMLTPX)
#define STR_VMLTPX2             (STR_OPEN9X + OFS_VMLTPX2)
#define STR_VMIXTRIMS           (STR_OPEN9X + OFS_VMIXTRIMS)
#define STR_VCSWFUNC            (STR_OPEN9X + OFS_VCSWFUNC)
#define STR_VFSWFUNC            (STR_OPEN9X + OFS_VFSWFUNC)
#define STR_VFSWRESET           (STR_OPEN9X + OFS_VFSWRESET)
#define STR_FUNCSOUNDS          (STR_OPEN9X + OFS_FUNCSOUNDS)
#define STR_VTELEMCHNS          (STR_OPEN9X + OFS_VTELEMCHNS)

    #define STR_VTELEMUNIT      (STR_OPEN9X + OFS_VTELEMUNIT)
    #define STR_VOLTSRC         (STR_OPEN9X + OFS_VOLTSRC)
#define STR_VALARM              (STR_OPEN9X + OFS_VALARM)
#define STR_VALARMFN            (STR_OPEN9X + OFS_VALARMFN)
#define STR_VTELPROTO           (STR_OPEN9X + OFS_VTELPROTO)
#define STR_GPSFORMAT           (STR_OPEN9X + OFS_GPSFORMAT)
#define STR_AMPSRC              (STR_OPEN9X + OFS_AMPSRC)
#define STR_VARIOSRC            (STR_OPEN9X + OFS_VARIOSRC)
#define STR_VTELEMSCREENTYPE    (STR_OPEN9X + OFS_VSCREEN)
#define STR_TELEMCHNS           (STR_OPEN9X + OFS_TELEMCHNS)


#if defined(HELI)
  #define STR_VSWASHTYPE        (STR_OPEN9X + OFS_VSWASHTYPE)
#endif

#define STR_VKEYS               (STR_OPEN9X + OFS_VKEYS)
#define STR_VSWITCHES           (STR_OPEN9X + OFS_VSWITCHES)
#define STR_VSRCRAW             (STR_OPEN9X + OFS_VSRCRAW)
#if defined(TRANSLATIONS_CZ)
#define STR_INPUTNAMES          (STR_OPEN9X + OFS_INPUTNAMES)
#endif
#define STR_VTMRMODES           (STR_OPEN9X + OFS_VTMRMODES)

#if defined(ROTARY_ENCODERS)
  #define STR_VRENAVIG          (STR_OPEN9X + OFS_VRENAVIG)
#endif

#if defined(ROTARY_ENCODER_NAVIGATION)
  #define STR_VRENCODERS        (STR_OPEN9X + OFS_VRENCODERS)
#endif

  #define STR_DATETIME          (STR_OPEN9X + OFS_DATETIME)
  #define STR_VPERSISTENT       (STR_OPEN9X + OFS_VPERSISTENT)

  #define STR_VLCD              (STR_OPEN9X + OFS_VLCD)
  #define STR_VUNITSSYSTEM      (STR_OPEN9X + OFS_VUNITSSYSTEM)
  #define STR_VBEEPCOUNTDOWN    (STR_OPEN9X + OFS_VBEEPCOUNTDOWN)
  #define STR_VVARIOCENTER      (STR_OPEN9X + OFS_VVARIOCENTER)

  #define STR_COUNTRYCODES      (STR_OPEN9X + OFS_COUNTRYCODES)
  #define STR_USBMODES          (STR_OPEN9X + OFS_USBMODES)
  #define STR_VFAILSAFE         (STR_OPEN9X + OFS_VFAILSAFE)

  #define STR_VTRAINERMODES     (STR_OPEN9X + OFS_VTRAINERMODES)
  #define STR_TARANIS_PROTOCOLS (STR_OPEN9X + OFS_TARANIS_PROTOCOLS)
  #define STR_R9M_REGION         (STR_OPEN9X + OFS_R9M_REGION)
  #define STR_R9M_FCC_POWER_VALUES     (STR_OPEN9X + OFS_R9M_FCC_POWER_VALUES)
  #define STR_R9M_LBT_POWER_VALUES     (STR_OPEN9X + OFS_R9M_LBT_POWER_VALUES)
#if !defined(PCBI6X)
  #define STR_TELEMETRY_PROTOCOLS      (STR_OPEN9X + OFS_TELEMETRY_PROTOCOLS)
#endif
  #define STR_XJT_PROTOCOLS     (STR_OPEN9X + OFS_XJT_PROTOCOLS)
  #define STR_DSM_PROTOCOLS     (STR_OPEN9X + OFS_DSM_PROTOCOLS)
  #define STR_I6X_PROTOCOLS     (STR_OPEN9X + OFS_I6X_PROTOCOLS)
#if defined(MULTIMODULE)
  #define STR_MULTI_PROTOCOLS   (STR_OPEN9X + OFS_MULTI_PROTOCOLS)
#endif
  #define STR_CURVE_TYPES       (STR_OPEN9X + OFS_CURVE_TYPES)
  #define STR_CRSF_ARMING_MODES (STR_OPEN9X + OFS_CRSF_ARMING_MODES)
  #define STR_VSENSORTYPES      (STR_OPEN9X + OFS_VSENSORTYPES)
  #define STR_VFORMULAS         (STR_OPEN9X + OFS_VFORMULAS)
  #define STR_VPREC             (STR_OPEN9X + OFS_VPREC)
  #define STR_VCELLINDEX        (STR_OPEN9X + OFS_VCELLINDEX)
  #define STR_VANTENNATYPES     (STR_OPEN9X + OFS_VANTENNATYPES)

#if defined(BLUETOOTH)
  extern const char STR_BLUETOOTH[];
extern const char STR_BLUETOOTH_DISC[];
extern const char STR_BLUETOOTH_INIT[];
extern const char STR_BLUETOOTH_DIST_ADDR[];
extern const char STR_BLUETOOTH_LOCAL_ADDR[];
  extern const char STR_BLUETOOTH_PIN_CODE[];
  #define STR_BLUETOOTH_MODES   (STR_OPEN9X + OFS_BLUETOOTH_MODES)
#endif


// The 0-terminated-strings
#define NO_INDENT(x) (x)+LEN_INDENT

extern const char STR_POPUPS_ENTER_EXIT[];

#if defined(OFS_EXIT)
  #define STR_EXIT (STR_POPUPS_ENTER_EXIT + OFS_EXIT)
#else
  extern const char STR_EXIT[];
#endif

#if !defined(PCBHORUS)
  extern const char STR_MENUWHENDONE[];
#endif

extern const char STR_FREE[];
#define LEN_FREE PSIZE(TR_FREE)
extern const char STR_DELETEMODEL[];
extern const char STR_COPYINGMODEL[];
extern const char STR_MOVINGMODEL[];
extern const char STR_LOADINGMODEL[];
extern const char STR_NAME[];
extern const char STR_BITMAP[];
extern const char STR_TIMER[];
extern const char STR_ELIMITS[];
extern const char STR_ETRIMS[];
extern const char STR_TRIMINC[];
extern const char STR_DISPLAY_TRIMS[];
extern const char STR_TTRACE[];
extern const char STR_TTRIM[];
extern const char STR_BEEPCTR[];
extern const char STR_USE_GLOBAL_FUNCS[];
extern const char STR_PROTO[];
extern const char STR_PPMFRAME[];
extern const char STR_REFRESHRATE[];
extern const char SSTR_WARN_BATTVOLTAGE[];
extern const char STR_MS[];
extern const char STR_SWITCH[];
extern const char STR_TRIMS[];
extern const char STR_FADEIN[];
extern const char STR_FADEOUT[];
extern const char STR_DEFAULT[];
extern const char STR_CHECKTRIMS[];
extern const char STR_SWASHTYPE[];
extern const char STR_COLLECTIVE[];
extern const char STR_AILERON[];
extern const char STR_ELEVATOR[];
extern const char STR_SWASHRING[];
extern const char STR_ELEDIRECTION[];
extern const char STR_AILDIRECTION[];
extern const char STR_COLDIRECTION[];
extern const char STR_MODE[];
#if defined(AUDIO) && defined(BUZZER)
extern const char STR_SPEAKER[];
extern const char STR_BUZZER[];
#else
#define STR_SPEAKER STR_MODE
#define STR_BUZZER  STR_MODE
#endif
extern const char STR_NOFREEEXPO[];
extern const char STR_NOFREEMIXER[];
extern const char STR_SOURCE[];
extern const char STR_WEIGHT[];
extern const char STR_EXPO[];
extern const char STR_SIDE[];
extern const char STR_DIFFERENTIAL[];
extern const char STR_OFFSET[];
extern const char STR_TRIM[];
extern const char STR_DREX[];
extern const char STR_CURVE[];
extern const char STR_FLMODE[];
extern const char STR_MIXWARNING[];
extern const char STR_OFF[];
extern const char STR_MULTPX[];
extern const char STR_DELAYDOWN[];
extern const char STR_DELAYUP[];
extern const char STR_SLOWDOWN[];
extern const char STR_SLOWUP[];
extern const char STR_MIXER[];
extern const char STR_CV[];
extern const char STR_GV[];
extern const char STR_ACHANNEL[];
extern const char STR_RANGE[];
extern const char STR_CENTER[];
extern const char STR_BAR[];
extern const char STR_ALARM[];
extern const char STR_USRDATA[];
extern const char STR_BLADES[];
extern const char STR_SCREEN[];
extern const char STR_SOUND_LABEL[];
extern const char STR_LENGTH[];
extern const char STR_BEEP_LENGTH[];
extern const char STR_BEEP_LENGTH[];
extern const char STR_SPKRPITCH[];
extern const char STR_HAPTIC_LABEL[];
extern const char STR_HAPTICSTRENGTH[];
extern const char STR_CONTRAST[];
extern const char STR_ALARMS_LABEL[];
extern const char STR_BATTERY_RANGE[];
extern const char STR_BATTERYWARNING[];
extern const char STR_INACTIVITYALARM[];
extern const char STR_MEMORYWARNING[];
extern const char STR_ALARMWARNING[];
extern const char STR_RSSISHUTDOWNALARM[];
extern const char STR_MODEL_STILL_POWERED[];
extern const char STR_MODEL_SHUTDOWN[];
extern const char STR_PRESS_ENTER_TO_CONFIRM[];
extern const char STR_RENAVIG[];
extern const char STR_THROTTLEREVERSE[];
extern const char STR_TIMER_NAME[];
extern const char STR_MINUTEBEEP[];
extern const char STR_BEEPCOUNTDOWN[];
extern const char STR_PERSISTENT[];
extern const char STR_BACKLIGHT_LABEL[];
extern const char STR_BLDELAY[];
#if defined(PWM_BACKLIGHT) || defined(PCBHORUS)
extern const char STR_BLONBRIGHTNESS[];
extern const char STR_BLOFFBRIGHTNESS[];
#endif
extern const char STR_SPLASHSCREEN[];
extern const char STR_THROTTLEWARNING[];
extern const char STR_SWITCHWARNING[];
extern const char STR_POTWARNINGSTATE[];
extern const char STR_POTWARNING[];
extern const char STR_SLIDERWARNING[];
extern const char STR_TIMEZONE[];
extern const char STR_ADJUST_RTC[];
extern const char STR_GPS[];
extern const char STR_GPSCOORD[];
extern const char STR_VARIO[];
extern const char STR_PITCH_AT_ZERO[];
extern const char STR_PITCH_AT_MAX[];
extern const char STR_REPEAT_AT_ZERO[];
extern const char STR_RXCHANNELORD[];
extern const char STR_STICKS[];
extern const char STR_POTS[];
extern const char STR_SWITCHES[];
extern const char STR_SWITCHES_DELAY[];
extern const char STR_SLAVE[];
extern const char STR_MODESRC[];
extern const char STR_MULTIPLIER[];
#define LEN_MULTIPLIER PSIZE(TR_MULTIPLIER)
extern const char STR_CAL[];
extern const char STR_VTRIM[];
extern const char STR_BG[];
extern const char STR_MENUTOSTART[];
extern const char STR_SETMIDPOINT[];
extern const char STR_MOVESTICKSPOTS[];
extern const char STR_RXBATT[];
extern const char STR_TX[];
#define STR_RX (STR_TX+OFS_RX)
extern const char STR_ACCEL[];
extern const char STR_NODATA[];
extern const char STR_TOTTM1TM2THRTHP[];
extern const char STR_TMR1LATMAXUS[];
extern const char STR_TMR1LATMINUS[];
extern const char STR_TMR1JITTERUS[];
extern const char STR_TMIXMAXMS[];
extern const char STR_FREESTACKMINB[];
extern const char STR_MENUTORESET[];
extern const char STR_PPM_TRAINER[];
extern const char STR_CH[];
extern const char STR_MODEL[];
extern const char STR_FM[];
extern const char STR_MIX[];
extern const char STR_EEPROMLOWMEM[];
extern const char STR_ALERT[];
extern const char STR_PRESSANYKEYTOSKIP[];
extern const char STR_THROTTLENOTIDLE[];
extern const char STR_ALARMSDISABLED[];
extern const char STR_PRESSANYKEY[];
#define LEN_PRESSANYKEY PSIZE(TR_PRESSANYKEY)
extern const char STR_BAD_RADIO_DATA[];
extern const char STR_STORAGE_FORMAT[];
extern const char STR_EEPROMOVERFLOW[];
extern const char STR_TRIMS2OFFSETS[];
extern const char STR_OUTPUTS2FAILSAFE[];
extern const char STR_MENURADIOSETUP[];
extern const char STR_MENUDATEANDTIME[];
extern const char STR_MENUTRAINER[];
extern const char STR_MENUSPECIALFUNCS[];
extern const char STR_MENUVERSION[];
extern const char STR_MENU_RADIO_SWITCHES[];
extern const char STR_MENU_RADIO_ANALOGS[];
extern const char STR_MENUCALIBRATION[];
extern const char STR_MENUMODELSEL[];
extern const char STR_MENUSETUP[];
extern const char STR_MENUFLIGHTMODE[];
extern const char STR_MENUFLIGHTMODES[];
extern const char STR_MENUHELISETUP[];
extern const char STR_MENUINPUTS[];
extern const char STR_MENULIMITS[];
extern const char STR_MENUCURVES[];
extern const char STR_MENUCURVE[];
extern const char STR_MENULOGICALSWITCH[];
extern const char STR_MENULOGICALSWITCHES[];
extern const char STR_MENUCUSTOMFUNC[];
extern const char STR_MENUCUSTOMSCRIPTS[];
extern const char STR_MENUTELEMETRY[];
extern const char STR_MENUTEMPLATES[];
extern const char STR_MENUSTAT[];
extern const char STR_MENUDEBUG[];
extern const char * const STR_MONITOR_CHANNELS[];
extern const char STR_MONITOR_SWITCHES[];
extern const char STR_MONITOR_OUTPUT_DESC[];
extern const char STR_MONITOR_MIXER_DESC[];
extern const char STR_MENUGLOBALVARS[];
extern const char STR_INVERT_THR[];
extern const char STR_AND_SWITCH[];
extern const char STR_SF[];
extern const char STR_GF[];

extern const char STR_FAS_OFFSET[];

#if defined(MULTIMODULE)
extern const char STR_MULTI_CUSTOM[];
extern const char STR_MULTI_OPTION[];
extern const char STR_MULTI_VIDFREQ[];
extern const char STR_MULTI_RFTUNE[];
extern const char STR_MULTI_TELEMETRY[];
extern const char STR_MULTI_AUTOBIND[];
extern const char STR_MULTI_DSM_AUTODTECT[];
extern const char STR_MULTI_LOWPOWER[];
extern const char STR_MODULE_NO_SERIAL_MODE[];
extern const char STR_MODULE_NO_INPUT[];
extern const char STR_MODULE_WAITFORBIND[];
extern const char STR_MODULE_NO_TELEMETRY[];
extern const char STR_MODULE_BINDING[];
extern const char STR_PROTOCOL_INVALID[];
extern const char STR_MODULE_STATUS[];
extern const char STR_MODULE_SYNC[];
extern const char STR_MULTI_SERVOFREQ[];
#if LCD_W < 212
extern const char STR_SUBTYPE[];
#endif
#endif
extern const char STR_RECEIVER_NUM[];
extern const char STR_RECEIVER[];
extern const char STR_REBIND[];

extern const char STR_SYNCMENU[];
extern const char STR_INTERNALRF[];
extern const char STR_EXTERNALRF[];
extern const char STR_MODULE_TELEMETRY[];
extern const char STR_MODULE_TELEM_ON[];
extern const char STR_FAILSAFE[];
extern const char STR_FAILSAFESET[];
extern const char STR_HOLD[];
extern const char STR_NONE[];
extern const char STR_MENUSENSOR[];
extern const char STR_SENSOR[];
extern const char STR_COUNTRYCODE[];
extern const char STR_USBMODE[];
extern const char STR_DISABLE_INTERNAL[];

#if defined(TELEMETRY_FRSKY)
extern const char STR_LIMIT[];
#endif

extern const char STR_MINRSSI[];
extern const char STR_LATITUDE[];
extern const char STR_LONGITUDE[];

extern const char STR_SHUTDOWN[];
extern const char STR_SAVEMODEL[];

#if defined(PCBX9E)
extern const char STR_POWEROFF[];
#endif

extern const char STR_BATT_CALIB[];

extern const char STR_VOLTAGE[];
extern const char STR_CURRENT[];

extern const char STR_CURRENT_CALIB[];
extern const char STR_UNITSSYSTEM[];
extern const char STR_UNITS_PPM[];
extern const char STR_VOICELANG[];
extern const char STR_MODELIDUSED[];
extern const char STR_BEEP_VOLUME[];
extern const char STR_WAV_VOLUME[];
extern const char STR_BG_VOLUME[];
extern const char STR_PERSISTENT_MAH[];

#define LEN_CALIB_FIELDS               PSIZE(TR_BATT_CALIB)

  extern const char STR_SELECT_MODEL[];
  extern const char STR_SELECT_MODE[];
  extern const char STR_CREATE_CATEGORY[];
  extern const char STR_RENAME_CATEGORY[];
  extern const char STR_DELETE_CATEGORY[];
  extern const char STR_CREATE_MODEL[];
  extern const char STR_DUPLICATE_MODEL[];
  extern const char STR_COPY_MODEL[];
  extern const char STR_MOVE_MODEL[];
  extern const char STR_DELETE_MODEL[];
  extern const char STR_EDIT[];
  extern const char STR_INSERT_BEFORE[];
  extern const char STR_INSERT_AFTER[];
  extern const char STR_COPY[];
  extern const char STR_MOVE[];
  extern const char STR_PASTE[];
  extern const char STR_INSERT[];
  extern const char STR_DELETE[];
  extern const char STR_RESET_FLIGHT[];
  extern const char STR_RESET_TIMER1[];
  extern const char STR_RESET_TIMER2[];
  extern const char STR_RESET_TIMER3[];
  extern const char STR_RESET_TELEMETRY[];
  extern const char STR_STATISTICS[];
  extern const char STR_SAVEALLDATA[];
  extern const char STR_ABOUT_US[];
  extern const char STR_USB_JOYSTICK[];
  extern const char STR_USB_MASS_STORAGE[];
  extern const char STR_USB_SERIAL[];
  extern const char STR_SETUP_SCREENS[];
  extern const char STR_MONITOR_SCREENS[];

extern const char STR_RESET_BTN[];
extern const char STR_DEBUG[];
extern const char STR_KEYS_BTN[];
extern const char STR_ANALOGS_BTN[];
extern const char STR_CALIB_BTN[];

#if defined(SDCARD)
  extern const char STR_BACKUP_MODEL[];
  extern const char STR_RESTORE_MODEL[];
  extern const char STR_DELETE_ERROR[];
  extern const char STR_SDCARD_ERROR[];
  extern const char STR_NO_SDCARD[];
  extern const char STR_SDCARD_FULL[];
  extern const char STR_INCOMPATIBLE[];
  extern const char STR_LOGS_PATH[];
  extern const char STR_LOGS_EXT[];
  extern const char STR_MODELS_PATH[];
  extern const char STR_MODELS_EXT[];
  #define STR_UPDATE_LIST STR_DELAYDOWN
#endif

extern const char STR_CAT_NOT_EMPTY[];
extern const char STR_WARNING[];
extern const char STR_STORAGE_WARNING[];
extern const char STR_EEPROM_CONVERTING[];
extern const char STR_THROTTLEWARN[];
extern const char STR_ALARMSWARN[];
extern const char STR_SWITCHWARN[];
extern const char STR_FAILSAFEWARN[];
extern const char STR_NIGHTLY_WARNING[];
extern const char STR_NIGHTLY_NOTSAFE[];
extern const char STR_WRONG_SDCARDVERSION[];
extern const char STR_WRONG_PCBREV[];
extern const char STR_EMERGENCY_MODE[];
extern const char STR_PCBREV_ERROR[];
extern const char STR_NO_FAILSAFE[];
extern const char STR_KEYSTUCK[];

extern const char STR_SPEAKER_VOLUME[];
extern const char STR_LCD[];
extern const char STR_BRIGHTNESS[];
extern const char STR_CPU_TEMP[];
extern const char STR_CPU_CURRENT[];
extern const char STR_CPU_MAH[];
extern const char STR_COPROC[];
extern const char STR_COPROC_TEMP[];
extern const char STR_CAPAWARNING[];
extern const char STR_TEMPWARNING[];
extern const char STR_FUNC[];
extern const char STR_V1[];
extern const char STR_V2[];
extern const char STR_DURATION[];
extern const char STR_DELAY[];
extern const char STR_SD_CARD[];
extern const char STR_SDHC_CARD[];
extern const char STR_NO_SOUNDS_ON_SD[];
extern const char STR_NO_MODELS_ON_SD[];
extern const char STR_NO_BITMAPS_ON_SD[];
extern const char STR_NO_SCRIPTS_ON_SD[];
extern const char STR_SCRIPT_SYNTAX_ERROR[];
extern const char STR_SCRIPT_PANIC[];
extern const char STR_SCRIPT_KILLED[];
extern const char STR_SCRIPT_ERROR[];
extern const char STR_PLAY_FILE[];
extern const char STR_ASSIGN_BITMAP[];
#if defined(PCBHORUS)
extern const char STR_ASSIGN_SPLASH[];
#endif
extern const char STR_EXECUTE_FILE[];
extern const char STR_DELETE_FILE[];
extern const char STR_COPY_FILE[];
extern const char STR_RENAME_FILE[];
extern const char STR_REMOVED[];
extern const char STR_SD_INFO[];
extern const char STR_SD_FORMAT[];
extern const char STR_NA[];
extern const char STR_HARDWARE[];
extern const char STR_FORMATTING[];
extern const char STR_TEMP_CALIB[];
extern const char STR_TIME[];
extern const char STR_MAXBAUDRATE[];
extern const char STR_BAUDRATE[];
extern const char STR_CRSF_ARMING_MODE[];
extern const char STR_SD_INFO_TITLE[];
extern const char STR_SD_TYPE[];
extern const char STR_SD_SPEED[];
extern const char STR_SD_SECTORS[];
extern const char STR_SD_SIZE[];
extern const char STR_TYPE[];
extern const char STR_GLOBAL_VARS[];
extern const char STR_GVARS[];
extern const char STR_GLOBAL_VAR[];
extern const char STR_OWN[];
extern const char STR_ROTARY_ENCODER[];
extern const char STR_DATE[];
extern const char STR_CHANNELS_MONITOR[];
extern const char STR_MIXERS_MONITOR[];
extern const char STR_PATH_TOO_LONG[];
extern const char STR_VIEW_TEXT[];
extern const char STR_FLASH_BOOTLOADER[];
extern const char STR_FLASH_EXTERNAL_DEVICE[];
extern const char STR_FLASH_INTERNAL_MODULE[];
extern const char STR_FLASH_EXTERNAL_MODULE[];
extern const char STR_FIRMWARE_UPDATE_ERROR[];
extern const char STR_WRITING[];
extern const char STR_CONFIRM_FORMAT[];
extern const char STR_EEBACKUP[];
extern const char STR_FACTORYRESET[];
extern const char STR_CONFIRMRESET[];
extern const char STR_TOO_MANY_LUA_SCRIPTS[];
extern const char STR_BLCOLOR[];

  struct LanguagePack {
    const char * id;
    const char * name;
    void (*playNumber)(getvalue_t number, uint8_t unit, uint8_t flags, uint8_t id);
    void (*playDuration)(int seconds, uint8_t flags, uint8_t id);
  };

  extern const LanguagePack * currentLanguagePack;
  extern uint8_t currentLanguagePackIdx;

  extern const LanguagePack czLanguagePack;
  extern const LanguagePack deLanguagePack;
  extern const LanguagePack enLanguagePack;
  extern const LanguagePack esLanguagePack;
  extern const LanguagePack frLanguagePack;
  extern const LanguagePack huLanguagePack;
  extern const LanguagePack itLanguagePack;
  extern const LanguagePack nlLanguagePack;
  extern const LanguagePack plLanguagePack;
  extern const LanguagePack ptLanguagePack;
  extern const LanguagePack ruLanguagePack;
  extern const LanguagePack seLanguagePack;
  extern const LanguagePack skLanguagePack;
  extern const LanguagePack * const languagePacks[];
  #if defined(LANGUAGE_PACKS_DEFINITION)
  const LanguagePack * const languagePacks[] = {
    // alphabetical order
    &czLanguagePack,
    &deLanguagePack,
    &enLanguagePack,
    &esLanguagePack,
    &frLanguagePack,
    &huLanguagePack,
    &itLanguagePack,
    &nlLanguagePack,
    &plLanguagePack,
    &ptLanguagePack,
    &ruLanguagePack,
    &seLanguagePack,
    &skLanguagePack,
    NULL
  };
  #endif
#if defined(SIMU)
  #define LANGUAGE_PACK_DECLARE(lng, name) const LanguagePack lng ## LanguagePack = { #lng, name, lng ## _ ## playNumber, lng ## _ ## playDuration }
#else
  #define LANGUAGE_PACK_DECLARE(lng, name) extern const LanguagePack lng ## LanguagePack = { #lng, name, lng ## _ ## playNumber, lng ## _ ## playDuration }
#endif
  #define LANGUAGE_PACK_DECLARE_DEFAULT(lng, name) LANGUAGE_PACK_DECLARE(lng, name); const LanguagePack * currentLanguagePack = & lng ## LanguagePack; uint8_t currentLanguagePackIdx
  inline PLAY_FUNCTION(playNumber, getvalue_t number, uint8_t unit, uint8_t flags) { currentLanguagePack->playNumber(number, unit, flags, id); }
  inline PLAY_FUNCTION(playDuration, int seconds, uint8_t flags) { currentLanguagePack->playDuration(seconds, flags, id); }

  extern const char STR_MODELNAME[];
  extern const char STR_PHASENAME[];
  extern const char STR_MIXNAME[];
  extern const char STR_INPUTNAME[];
  extern const char STR_EXPONAME[];

#if defined(COLORLCD)
  #define TR_PHASES_HEADERS { TR_PHASES_HEADERS_NAME, TR_PHASES_HEADERS_SW, TR_PHASES_HEADERS_RUD_TRIM, TR_PHASES_HEADERS_ELE_TRIM, TR_PHASES_HEADERS_THT_TRIM, TR_PHASES_HEADERS_AIL_TRIM, TR_PHASES_HEADERS_CH5_TRIM, TR_PHASES_HEADERS_CH6_TRIM, TR_PHASES_HEADERS_FAD_IN, TR_PHASES_HEADERS_FAD_OUT }
#elif defined(PCBTARANIS)
  #define TR_PHASES_HEADERS { HINT(TR_PHASES_HEADERS_NAME), HINT(TR_PHASES_HEADERS_SW), HINT(TR_PHASES_HEADERS_RUD_TRIM), HINT(TR_PHASES_HEADERS_ELE_TRIM), HINT(TR_PHASES_HEADERS_THT_TRIM), HINT(TR_PHASES_HEADERS_AIL_TRIM), HINT(TR_PHASES_HEADERS_FAD_IN), HINT(TR_PHASES_HEADERS_FAD_OUT) }
#endif

#define TR_LIMITS_HEADERS { HINT(TR_LIMITS_HEADERS_NAME), HINT(TR_LIMITS_HEADERS_SUBTRIM), HINT(TR_LIMITS_HEADERS_MIN), HINT(TR_LIMITS_HEADERS_MAX), HINT(TR_LIMITS_HEADERS_DIRECTION), HINT(TR_LIMITS_HEADERS_CURVE), HINT(TR_LIMITS_HEADERS_PPMCENTER), HINT(TR_LIMITS_HEADERS_SUBTRIMMODE) }
#define TR_LSW_HEADERS    { HINT(TR_LSW_HEADERS_FUNCTION), HINT(TR_LSW_HEADERS_V1), HINT(TR_LSW_HEADERS_V2), HINT(TR_LSW_HEADERS_V2), HINT(TR_LSW_HEADERS_ANDSW), HINT(TR_LSW_HEADERS_DURATION), HINT(TR_LSW_HEADERS_DELAY) }
#define TR_GVAR_HEADERS   { HINT(TR_GVAR_HEADERS_FM0), HINT(TR_GVAR_HEADERS_FM1), HINT(TR_GVAR_HEADERS_FM2), HINT(TR_GVAR_HEADERS_FM3), HINT(TR_GVAR_HEADERS_FM4), HINT(TR_GVAR_HEADERS_FM5), HINT(TR_GVAR_HEADERS_FM6), HINT(TR_GVAR_HEADERS_FM7), HINT(TR_GVAR_HEADERS_FM8) }

#if LCD_W >= 212
  extern const char * const STR_PHASES_HEADERS[];
  extern const char * const STR_LIMITS_HEADERS[];
  extern const char * const STR_LSW_HEADERS[];
  extern const char * const STR_LSW_DESCRIPTIONS[];
  extern const char * const STR_GVAR_HEADERS[];
#endif

  extern const char STR_TRAINER[];
  extern const char STR_MODULE_BIND[];
  extern const char STR_BINDING_1_8_TELEM_ON[];
  extern const char STR_BINDING_1_8_TELEM_OFF[];
  extern const char STR_BINDING_9_16_TELEM_ON[];
  extern const char STR_BINDING_9_16_TELEM_OFF[];
  extern const char STR_CHANNELRANGE[];
  extern const char STR_ANTENNASELECTION[];
  extern const char STR_ANTENNACONFIRM1[];
  extern const char STR_ANTENNACONFIRM2[];
  extern const char STR_R9MFLEXWARN1[];
  extern const char STR_R9MFLEXWARN2[];
  extern const char STR_SET[];
  extern const char STR_PREFLIGHT[];
  extern const char STR_CHECKLIST[];
  extern const char STR_VIEW_NOTES[];
  extern const char STR_MODEL_SELECT[];
  extern const char STR_RESET_SUBMENU[];
  extern const char STR_LOWALARM[];
  extern const char STR_CRITICALALARM[];
  extern const char STR_RSSIALARM_WARN[];
  extern const char STR_NO_RSSIALARM[];
  extern const char STR_DISABLE_ALARM[];
  extern const char STR_TELEMETRY_TYPE[];
  extern const char STR_TELEMETRY_SENSORS[];
  extern const char STR_VALUE[];
  extern const char STR_TOPLCDTIMER[];
  extern const char STR_UNIT[] ;
  extern const char STR_TELEMETRY_NEWSENSOR[];
  extern const char STR_ID[];
  extern const char STR_PRECISION[];
  extern const char STR_RATIO[];
  extern const char STR_FORMULA[];
  extern const char STR_CELLINDEX[];
  extern const char STR_LOGS[];
  extern const char STR_OPTIONS[];
  extern const char STR_ALTSENSOR[];
  extern const char STR_CELLSENSOR[];
  extern const char STR_GPSSENSOR[];
  extern const char STR_CURRENTSENSOR[];
  extern const char STR_AUTOOFFSET[];
  extern const char STR_ONLYPOSITIVE[];
  extern const char STR_FILTER[];
  extern const char STR_TELEMETRYFULL[];
  extern const char STR_SERVOS_OK[];
  extern const char STR_SERVOS_KO[];
  extern const char STR_INVERTED_SERIAL[];
  extern const char STR_IGNORE_INSTANCE[];
  extern const char STR_DISCOVER_SENSORS[];
  extern const char STR_STOP_DISCOVER_SENSORS[];
  extern const char STR_DELETE_ALL_SENSORS[];
  extern const char STR_CONFIRMDELETE[];
  extern const char STR_SELECT_WIDGET[];
  extern const char STR_REMOVE_WIDGET[];
  extern const char STR_WIDGET_SETTINGS[];
  extern const char STR_REMOVE_SCREEN[];
  extern const char STR_SETUP_WIDGETS[];
  extern const char STR_USER_INTERFACE[];
  extern const char STR_THEME[];
  extern const char STR_SETUP[];
  extern const char STR_MAINVIEWX[];
  extern const char STR_LAYOUT[];
  extern const char STR_ADDMAINVIEW[];
  extern const char STR_BACKGROUND_COLOR[];
  extern const char STR_MAIN_COLOR[];
  extern const char STR_TEXT_VIEWER[];
  extern const char STR_MULTI_RFPOWER[];

  extern const char STR_BYTES[];
  extern const char STR_ANTENNAPROBLEM[];
  extern const char STR_MODULE[];
  extern const char STR_ENABLE_POPUP[];
  extern const char STR_DISABLE_POPUP[];
  extern const char STR_POPUP[];
  extern const char STR_MIN[];
  extern const char STR_MAX[];
  extern const char STR_CURVE_PRESET[];
  extern const char STR_PRESET[];
  extern const char STR_MIRROR[];
  extern const char STR_CLEAR[];
  extern const char STR_RESET[];
  extern const char STR_COUNT[];
  extern const char STR_PT[];
  extern const char STR_PTS[];
  extern const char STR_SMOOTH[];
  extern const char STR_COPY_STICKS_TO_OFS[];
  extern const char STR_COPY_TRIMS_TO_OFS[];
  extern const char STR_INCDEC[];
  extern const char STR_GLOBALVAR[];
  extern const char STR_MIXSOURCE[];
  extern const char STR_CONSTANT[];
  extern const char STR_TOP_BAR[];
  extern const char STR_ALTITUDE[];
  extern const char STR_SCALE[];
  extern const char STR_VIEW_CHANNELS[];
  extern const char STR_POTWARNING[];
  extern const char STR_AUX_SERIAL_MODE[];
  extern const char STR_THROTTLE_LABEL[];
  extern const char STR_SCRIPT[];
  extern const char STR_INPUTS[];
  extern const char STR_OUTPUTS[];
  extern const char STR_MENU_INPUTS[];
  extern const char STR_MENU_LUA[];
  extern const char STR_MENU_STICKS[];
  extern const char STR_MENU_POTS[];
  extern const char STR_MENU_MAX[];
  extern const char STR_MENU_HELI[];
  extern const char STR_MENU_TRIMS[];
  extern const char STR_MENU_SWITCHES[];
  extern const char STR_MENU_LOGICAL_SWITCHES[];
  extern const char STR_MENU_TRAINER[];
  extern const char STR_MENU_CHANNELS[];
  extern const char STR_MENU_GVARS[];
  extern const char STR_MENU_TELEMETRY[];
  extern const char STR_MENU_DISPLAY[];
  extern const char STR_MENU_OTHER[];
  extern const char STR_MENU_INVERT[];
  extern const char STR_JITTER_FILTER[];

#if MENUS_LOCK == 1
  extern const char STR_UNLOCKED[];
  extern const char STR_MODS_FORBIDDEN[];
#endif

#if defined(PCBTARANIS) || defined(DSM2) || defined(PCBI6X)
  extern const char STR_MODULE_RANGE[];
#endif


  extern const char STR_ABOUTUS[];
  extern const char STR_ABOUT_OPENTX_1[];
  extern const char STR_ABOUT_OPENTX_2[];
  extern const char STR_ABOUT_OPENTX_3[];
  extern const char STR_ABOUT_OPENTX_4[];
  extern const char STR_ABOUT_OPENTX_5[];

  extern const char STR_ABOUT_BERTRAND_1[];
  extern const char STR_ABOUT_BERTRAND_2[];
  extern const char STR_ABOUT_BERTRAND_3[];

  extern const char STR_ABOUT_MIKE_1[];
  extern const char STR_ABOUT_MIKE_2[];
  extern const char STR_ABOUT_MIKE_3[];
  extern const char STR_ABOUT_MIKE_4[];

  extern const char STR_ABOUT_ROMOLO_1[];
  extern const char STR_ABOUT_ROMOLO_2[];
  extern const char STR_ABOUT_ROMOLO_3[];

  extern const char STR_ABOUT_ANDRE_1[];
  extern const char STR_ABOUT_ANDRE_2[];
  extern const char STR_ABOUT_ANDRE_3[];

  extern const char STR_ABOUT_ROB_1[];
  extern const char STR_ABOUT_ROB_2[];

  extern const char STR_ABOUT_MARTIN_1[];
  extern const char STR_ABOUT_MARTIN_2[];

  extern const char STR_ABOUT_KJELL_1[];
  extern const char STR_ABOUT_KJELL_2[];
  extern const char STR_ABOUT_KJELL_3[];
  extern const char STR_ABOUT_KJELL_4[];

  extern const char STR_ABOUT_HARDWARE_1[];
  extern const char STR_ABOUT_HARDWARE_2[];
  extern const char STR_ABOUT_HARDWARE_3[];

  extern const char STR_ABOUT_PARENTS_1[];
  extern const char STR_ABOUT_PARENTS_2[];
  extern const char STR_ABOUT_PARENTS_3[];
  extern const char STR_ABOUT_PARENTS_4[];

#define CHR_SHORT  TR_CHR_SHORT
#define CHR_LONG   TR_CHR_LONG
#define CHR_TOGGLE TR_CHR_TOGGLE
#define CHR_HOUR   TR_CHR_HOUR
#define CHR_INPUT  TR_CHR_INPUT

#endif // _TRANSLATIONS_H_
