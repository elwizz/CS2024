/*
 * mpu_morse.c
 *
 * Computer Systems 2024 course, University of Oulu
 * Author: Eelis Turjanmaa
 *
 * Based on:
 * - MPU9250_H_ by Teemu Leppanen / UBIComp / University of Oulu, 10.10.2016
 *  (Adopted for SensorTag from https://github.com/kriswiner/MPU-9250 by Kris Winer)
 *- Computer Systems 2024 course material
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <inttypes.h>

#include <xdc/runtime/System.h>
#include "sensors/mpu9250.h"
#include <driverlib/aon_batmon.h>

//Data structures for MPU samples implemented according to course examples
struct accelerometer_t {
    float ax, ay, az;

} accelerometer_t;
struct gyroscope_t {
    float gx, gy, gz;
};
struct mpu_status_t {
    uint8_t status_code, battery_level;
};
struct mpu_sample_t {
    uint32_t timestamp;
    struct accelerometer_t accel;
    struct gyroscope_t gyro;
    struct mpu_status_t status;
};

// Function prototypes
void sampleMpu(struct mpu_sample_t *new_sample, uint32_t new_tstamp);
void addMpuSample(struct mpu_sample_t *samples, struct mpu_sample_t new_sample, uint8_t list_size);
struct mpu_sample_t averageSample(struct mpu_sample_t *samples, uint8_t list_size);
void formatSamplelist(struct mpu_sample_t *samples, uint8_t struct_size);
void gyroDamp(struct mpu_sample_t *samples, uint8_t struct_size);
void sprintSample(struct mpu_sample_t sample);
uint8_t readBtrVoltage();
void mpuMorse(struct mpu_sample_t *new_sample, struct mpu_sample_t *avg_sample, struct mpu_sample_t *samples, uint32_t *timestamp, uint8_t list_size, uint8_t *key, uint16_t *key_count, uint8_t *state, uint8_t *signal, char *morse);

//Function for forming a single MPU sample
void sampleMpu(struct mpu_sample_t *new_sample, uint32_t new_tstamp){
    uint32_t ticks; // If needed somewehere later on...
    mpu9250_get_data(&new_sample->accel.ax, &new_sample->accel.ay, &new_sample->accel.az, &new_sample->gyro.gx, &new_sample->gyro.gy, &new_sample->gyro.gz, &ticks);
    new_sample->timestamp = new_tstamp;
    new_sample->status.battery_level = readBtrVoltage();
    //new_sample->status.status_code = //Mist� t�m� otetaan?
}

//Function for adding a sample to a list of samples
void addMpuSample(struct mpu_sample_t *samples, struct mpu_sample_t new_sample, uint8_t list_size){
    uint8_t i = 0;
    for(i = 0; i < (list_size-1); i++){
        (samples + i)->timestamp = (samples + i + 1)->timestamp;
        (samples + i)->accel = (samples + i + 1)->accel;
        (samples + i)->gyro = (samples + i + 1)->gyro;
        (samples + i)->status = (samples + i + 1)->status;
         }
    (samples + list_size-1)->timestamp = new_sample.timestamp;
    (samples + list_size-1)->accel = new_sample.accel;
    (samples + list_size-1)->gyro = new_sample.gyro;
    (samples + list_size-1)->status = new_sample.status;
}

//Function for averaging a sample
struct mpu_sample_t averageSample(struct mpu_sample_t *samples, uint8_t list_size){
    uint8_t i = 0;
    float ax_sum = 0;
    float ay_sum = 0;
    float az_sum = 0;
    float gx_sum = 0;
    float gy_sum = 0;
    float gz_sum = 0;

    struct mpu_sample_t average_sample;

    for(i = 0; i < (list_size); i++){
        ax_sum = ax_sum + (samples + i)->accel.ax;
        ay_sum = ay_sum + (samples + i)->accel.ay;
        az_sum = az_sum + (samples + i)->accel.az;
        gx_sum = gx_sum + (samples + i)->gyro.gx;
        gy_sum = gy_sum + (samples + i)->gyro.gy;
        gz_sum = gz_sum + (samples + i)->gyro.gz;
    }

    average_sample.timestamp = (samples + (list_size - 1))->timestamp;
    average_sample.accel.ax = ax_sum/list_size;
    average_sample.accel.ay = ay_sum/list_size;
    average_sample.accel.az = az_sum/list_size;
    average_sample.gyro.gx = gx_sum/list_size;
    average_sample.gyro.gy = gy_sum/list_size;
    average_sample.gyro.gz = gz_sum/list_size;
    average_sample.status = (samples + (list_size - 1))->status;

    return average_sample;
}

//Function for formatting a sample
void formatSamplelist(struct mpu_sample_t *samples, uint8_t struct_size){
    uint8_t i;

    for(i = 0; i < struct_size; i++){
        (samples + i)->timestamp = 0;
        (samples + i)->accel.ax = 0;
        (samples + i)->accel.ay = 0;
        (samples + i)->accel.az = 0;
        (samples + i)->gyro.gx = 0;
        (samples + i)->gyro.gy = 0;
        (samples + i)->gyro.gz = 0;
        (samples + i)->status.status_code = 0;
        (samples + i)->status.battery_level = 0;
        }
    }

//Function for "dampening" gyroscope measurements after impact
void gyroDamp(struct mpu_sample_t *samples, uint8_t struct_size){
    uint8_t j;
    for(j = 0; j < struct_size; j++){
        (samples + j)->gyro.gx = 0;
        (samples + j)->gyro.gy = 0;
        (samples + j)->gyro.gz = 0;
        }
}

// A function to print a sample - meant for testing
void sprintSample(struct mpu_sample_t sample){
     char sample_str[30];
     sprintf(sample_str,"%u,%.2lf,%.2lf,%.2lf,%.2lf,%.2lf,%.2lf\r", sample.timestamp, sample.accel.ax, sample.accel.ay, sample.accel.az, sample.gyro.gx, sample.gyro.gy, sample.gyro.gz);
     System_printf("%s", sample_str);
     System_flush();
 }

// A function for reading the battery voltage - implemented according to a course example
uint8_t readBtrVoltage(){
     uint32_t battery_reg;
     uint32_t battery_int;
     uint32_t battery_frac;
     char battery_str[30];
     float battery_voltage;
     uint8_t battery_level;

     battery_reg = HWREG(AON_BATMON_BASE + AON_BATMON_O_BAT);
     battery_int = ((battery_reg & 0x7FF) >> 8);
     battery_frac = (battery_reg & 0xFF);

     sprintf(battery_str,"%u.%u\r", battery_int, battery_frac);
     battery_voltage = atof(battery_str);
     battery_level = floor(100*battery_voltage/4);

     return battery_level;
}
//"Master" function, interprets gyroscope and accelerometer data as morse "keying" action
//Designed to be called in the main program by a Task containing necessary arrays and variables for containing
//information on the keying and setting program states according to these.
//Function receives these arrays and variables as arguments, returns void, but manipulates caller Task arrays and
//variables through pointers
void mpuMorse(struct mpu_sample_t *new_sample, struct mpu_sample_t *avg_sample, struct mpu_sample_t *samples, uint32_t *timestamp, uint8_t list_size, uint8_t *key, uint16_t *key_count, uint8_t *state, uint8_t *signal, char *morse){
    // For every measurement round add a new sample with a time stamp
     sampleMpu(&new_sample[0], *timestamp);

    //During the first measurement round, fill an n-size data structure with a copy of the first sample.
    //During later rounds use First-In-First-Out to update the list.
     if(*timestamp == 0){
         uint8_t j;
         for (j = 0; j < list_size; j++){
             addMpuSample(&samples[0], *new_sample, list_size);
         }
     } else {
         addMpuSample(&samples[0], *new_sample, list_size);
     }

    // Use the array to calculate an averaged sample
     *avg_sample = averageSample(&samples[0], list_size);

    //The function has three states for the "morse key":
    // 1) If the Sensortag is laying at rest, the key is at rest (State 0)
    // 2) If the left side of a correctly oriented Sensortag has been lifted quickly enough to a sufficient level,
    // the "key" is pushed
    // down (State 1)
    // 3) If the right side of a correctly oriented Sensortag has been lifted quickly enough to a sufficient level,
    //     // the "space key" is pushed
    // The sequence 0 -> 1 -> 0  will produce a dot or a dash symbol, depending on the duration of the keying.
    // The sequence 0 -> 2 -> 0 will produce a space symbol, regardless of the duration of the keying.
    // The thresholds make use of both averaged gyroscope and averaged accelerometer measurements and instantaneous
    // accelerometer measurements.
    // The gyroDamp function has also been used, although it's necessity is questionable at this point.

    //Incorrect orientation of the Sensortag prevents keying
     if(avg_sample->accel.ax < 0.2 && avg_sample->accel.ax > -0.2){

         //Action when the "key" is "at rest"
         if(*key == 0){
             //The "key" is lifted
             if((avg_sample->gyro.gx) < -30 && new_sample->accel.ay > 0.3){
                 *key = 1;
                 *signal = 2;
                 gyroDamp(&samples[0], list_size);
             } else if((avg_sample->gyro.gx) > 30 && new_sample->accel.ay < -0.3){
                 *key = 2;
                 *signal = 3;
                 gyroDamp(&samples[0], list_size);
             }
         }
         //Action when the "key" is "pushed"
         else if(*key == 1){
             // If the "key" is closed the duration of "keying" is incremented by + 1
             *key_count = *key_count + 1;
             if(new_sample->accel.ay < 0.2){
                 if(*key_count < 3){
                     strcpy(morse, ".\r\n\0");
                     *state = 1;
                 } else {
                     strcpy(morse, "-\r\n\0");
                     *state = 1;
                 }
                 *key = 0;
                 *signal = 1;
                 *key_count = 0;
                 gyroDamp(&samples[0], list_size);
             }
         }
         //Action when the "space key" is pushed
         else if(*key == 2){
             if(new_sample->accel.ay > -0.1){
                 *key = 0;
                 *signal = 1;
                 strcpy(morse, " \r\n\0");
                 *state = 1;
                 gyroDamp(&samples[0], list_size);
              }
         }
         // Timestamp incremented by +1 at the end of the measurement round
         *timestamp = *timestamp + 1;
    }
 }
