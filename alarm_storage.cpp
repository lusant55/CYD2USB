#include "alarm_storage.h"
#include <Preferences.h>

extern Alarm alarms[MAX_ALARMS];

#define PREF_NAMESPACE "ALARMS_NVS"

void alarmsSave() {
    Preferences prefs;
    prefs.begin(PREF_NAMESPACE, false); // false = RW

    for (int i=0;i<MAX_ALARMS;i++) {
        char key[16];

        snprintf(key,sizeof(key),"y%d",i); prefs.putUShort(key,alarms[i].year);
        snprintf(key,sizeof(key),"m%d",i); prefs.putUChar(key,alarms[i].month);
        snprintf(key,sizeof(key),"d%d",i); prefs.putUChar(key,alarms[i].day);
        snprintf(key,sizeof(key),"h%d",i); prefs.putUChar(key,alarms[i].hour);
        snprintf(key,sizeof(key),"min%d",i); prefs.putUChar(key,alarms[i].minute);

        uint8_t dow = 0;
        for(int j=0;j<7;j++) if(alarms[i].daysOfWeek[j]) dow |= (1<<j);
        snprintf(key,sizeof(key),"dow%d",i); prefs.putUChar(key,dow);

        snprintf(key,sizeof(key),"act%d",i); prefs.putBool(key,alarms[i].active);
    }
    prefs.end();
}

void alarmsLoad() {
    Preferences prefs;
    prefs.begin(PREF_NAMESPACE, true); // true = read-only

    for (int i=0;i<MAX_ALARMS;i++) {
        char key[16];
        alarms[i].year = prefs.getUShort((String("y")+i).c_str(),0);
        alarms[i].month = prefs.getUChar((String("m")+i).c_str(),0);
        alarms[i].day = prefs.getUChar((String("d")+i).c_str(),0);
        alarms[i].hour = prefs.getUChar((String("h")+i).c_str(),0);
        alarms[i].minute = prefs.getUChar((String("min")+i).c_str(),0);

        uint8_t dow = prefs.getUChar((String("dow")+i).c_str(),0);
        for(int j=0;j<7;j++) alarms[i].daysOfWeek[j] = (dow & (1<<j))!=0;

        alarms[i].active = prefs.getBool((String("act")+i).c_str(),false);
    }
    prefs.end();
}