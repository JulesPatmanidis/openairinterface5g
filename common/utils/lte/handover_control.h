#ifndef HANDOVER_CONTROL_H
#define HANDOVER_CONTROL_H

#include <stdio.h>
#include <sys/time.h>
#include <unistd.h>

#include "rrc_defs.h"
#include "rrc_extern.h"

const float RSRP_GOOD_MEASUREMENT;

uint8_t eNB_fake_rsrp_measurements[10];
extern long long start_time;

long long get_time_ms();

static uint8_t check_trigger_meas_event_custom(module_id_t module_idP,
                                               frame_t frameP,
                                               uint8_t eNB_index,
                                               uint8_t ue_cnx_index,
                                               uint8_t meas_index,
                                               LTE_Q_OffsetRange_t ofn,
                                               LTE_Q_OffsetRange_t ocn,
                                               LTE_Hysteresis_t hys,
                                               LTE_Q_OffsetRange_t ofs,
                                               LTE_Q_OffsetRange_t ocs,
                                               long a3_offset,
                                               LTE_TimeToTrigger_t ttt);

void generate_rsrp_measurement_from_file(char * filename);

float get_fake_measurement(uint8_t eNB_index);

#endif /* HANDOVER_CONTROL_H*/