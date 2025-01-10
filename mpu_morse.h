/*
 * mpu_morse.h
 *
 * Computer Systems 2024 course, University of Oulu
 * Author: Eelis Turjanmaa
 *
 * Based on:
 * - MPU9250_H_ by Teemu Leppanen / UBIComp / University of Oulu, 10.10.2016
 *  (Adopted for SensorTag from https://github.com/kriswiner/MPU-9250 by Kris Winer)
 *- Computer Systems 2024 course material
 */

#ifndef MPU_MORSE_H_
#define MPU_MORSE_H_

//Data structures implemented in accordance with course examples
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

void sampleMpu(struct mpu_sample_t *new_sample, uint32_t new_tstamp);
void addMpuSample(struct mpu_sample_t *samples, struct mpu_sample_t new_sample, uint8_t list_size);
struct mpu_sample_t averageSample(struct mpu_sample_t *samples, uint8_t list_size);
void formatSamplelist(struct mpu_sample_t *samples, uint8_t struct_size);
void gyroDamp(struct mpu_sample_t *samples, uint8_t struct_size);
void sprintSample(struct mpu_sample_t sample);
uint8_t readBtrVoltage();
void mpuMorse(struct mpu_sample_t *new_sample, struct mpu_sample_t *avg_sample, struct mpu_sample_t *samples, uint32_t *timestamp, uint8_t list_size, uint8_t *key, uint16_t *key_count, uint8_t *state, uint8_t *signal, char *morse);

#endif /* MPU_MORSE_H_ */
