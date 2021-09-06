/*
 * Licensed to the OpenAirInterface (OAI) Software Alliance under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.
 * The OpenAirInterface Software Alliance licenses this file to You under
 * the OAI Public License, Version 1.1  (the "License"); you may not use this file
 * except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.openairinterface.org/?page_id=698
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *-------------------------------------------------------------------------------
 * For more information about the OpenAirInterface (OAI) Software Alliance:
 *      contact@openairinterface.org
 */

#ifndef _SHARED_BUFFERS_H_
#define _SHARED_BUFFERS_H_

#include <pthread.h>
#include <stdint.h>

typedef struct {
  unsigned char dl[10][14*1200*4];
  unsigned char ul[10][14*1200*4];
  uint16_t dl_busy[10];
  uint16_t ul_busy[10];

  pthread_mutex_t m[10];
  pthread_cond_t  c[10];

  unsigned char prach[10][849*4];
  unsigned char prach_busy[10];

  /* statistics/error counting */
  int ul_overflow;
  int dl_underflow;
} shared_buffers;

void init_buffers(shared_buffers *s);
void lock_buffers(shared_buffers *s, int subframe);
void unlock_buffers(shared_buffers *s, int subframe);
void wait_buffers(shared_buffers *s, int subframe);
void signal_buffers(shared_buffers *s, int subframe);

#endif /* _SHARED_BUFFERS_H_ */
