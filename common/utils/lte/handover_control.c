#include "handover_control.h"

const float RSRP_GOOD_MEASUREMENT = 40.0;
long long start_time = 0;

long long get_time_ms()
{
  struct timeval te;
  gettimeofday(&te, NULL); // get current time
  long long milliseconds = te.tv_sec * 1000LL + te.tv_usec / 1000; // calculate milliseconds
  // printf("milliseconds: %lld\n", milliseconds);
  return milliseconds;
}

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
                                               LTE_TimeToTrigger_t ttt)
{
  uint8_t oai_check = 0;//check_trigger_meas_event(module_idP, frameP, eNB_index, ue_cnx_index, meas_index, ofn, ocn, hys, ofs, ocs, a3_offset, ttt);
  printf("DEBUG: Inside wrapper function!");
  return oai_check;
}

float get_fake_measurement(uint8_t eNB_index)
{
  return eNB_fake_rsrp_measurements[eNB_index];
}

void generate_rsrp_measurement_from_file(char *filename)
{
  if (start_time == 0) {
    start_time = get_time_ms();
  }

  long long current_time;
  char line[256];
  uint8_t target_eNB = -1;
  FILE *fptr;

  if ((fptr = fopen(filename, "r")) == NULL) {
    printf("DEBUG: Error! opening %s file\n", filename);
    return 0;
  }

  int enb_id, time_offset;
  int line_count = 0;
  current_time = get_time_ms();
  while (fgets(line, sizeof(line), fptr)) {
    line_count++;

    sscanf(line, "%d,%d", &enb_id, &time_offset);

    if (current_time < start_time + time_offset)
      continue;
      
    target_eNB = enb_id;
  }

  for (uint8_t enb_offset = 0; enb_offset < NB_eNB_INST; enb_offset++) {
    eNB_fake_rsrp_measurements[enb_offset] = (enb_offset == target_eNB) ? RSRP_GOOD_MEASUREMENT : 0.0;
  }
  printf("DEBUG: fake measurments generated! start: %lld current: %lld\n", start_time, current_time);
  fclose(fptr);
  return 0;
}