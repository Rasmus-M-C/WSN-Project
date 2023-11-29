
#ifndef POWERCONSUMPTION_H
#define POWERCONSUMPTION_H

extern const float RX; // mA
extern const float TX ; // mA
extern const float Radiooff; // mA Not sure if this is correct
extern const float CPU_NORMAL; // mA
extern const float CPU_SLEEP; // mA
extern const float CPU_DEEP_SLEEP; // mA Not sure if this is correct

/*---------------------------------------------------------------------------*/
struct IntDec {
    unsigned long integer;
    unsigned long decimal;
};

struct PowerConsumptionStates {
  float CPU_NORMAL_usage;
  float CPU_SLEEP_usage;
  float CPU_DEEP_SLEEP_usage;
  float TX_usage;
  float RX_usage;
  float Radiooff_usage;
  float Total_usage;
  unsigned long CPU_NORMAL_time;
  unsigned long CPU_SLEEP_time;
  unsigned long CPU_DEEP_SLEEP_time;
  unsigned long TX_time;
  unsigned long RX_time;
  unsigned long Radiooff_time;
  unsigned long Total_time;
};

struct IntDec Get_Float_Parts(float value);

struct PowerConsumptionStates TotalPowerConsumption();

unsigned long to_seconds(uint64_t time);



#endif  // MYFUNCTIONS_H