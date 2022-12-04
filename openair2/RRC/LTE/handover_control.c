#include "rrc_defs.h"

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
  uint8_t oai_check = check_trigger_meas_event(module_idP, frameP, eNB_index, ue_cnx_index, meas_index, ofn, ocn, hys, ofs, ocs, a3_offset, ttt);
  printf("DEBUG: Inside wrapper function!");
  return oai_check;
}