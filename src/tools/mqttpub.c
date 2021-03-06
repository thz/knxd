/*
    mqtt-bridge - read from KNX send to mqtt
    Copyright (C) 2020 Tobias Hintze <th-git@thzn.de>

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#include "common.h"
#include "path.h"
#include <time.h>
#include <fcntl.h>
#include <string.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "mqtt_knx.h"

#define MAX_TOPIC_LEN 64

int
mqttpub (EIBConnection *con, const char *broker_host, const int broker_port, const char *topic)
{
  uint8_t buf[255];
  int len;
  eibaddr_t dest;
  eibaddr_t src;

  if (strlen(topic) > MAX_TOPIC_LEN) {
      die("topic too long");
  }

  mqttbridge_ctx_t *ctx = malloc(sizeof(mqttbridge_ctx_t));
  if (!ctx) {
      die("failed to allocate for mqttbrige context");
  }
  ctx->broker_host = broker_host;
  ctx->broker_port = broker_port;
  ctx->mosq_connected = 0;
  ctx->topic = topic;
  ctx->mosq = NULL;

  /* Buffering stdout is almost never what we want */
  setvbuf(stdout, NULL, _IOLBF, 0);

  if (EIBOpen_GroupSocket (con, 0) == -1)
      die ("Connect failed");

  while (1)
  {
      len = EIBGetGroup_Src (con, sizeof (buf), buf, &src, &dest);
      if (len == -1)
          die ("Read failed");
      if (len < 2)
          die ("Invalid Packet");
      if (buf[0] & 0x3 || (buf[1] & 0xC0) == 0xC0)
      {
          printf ("Unknown APDU from ");
          printIndividual (src);
          printf (" to ");
          printGroup (dest);
          printf (": ");
          printHex (len, buf);
          printf ("\n");
      }
      else
      {
          switch (buf[1] & 0xC0)
          {
              case 0x00:
                  printf ("Read");
                  break;
              case 0x40:
                  printf ("Response");
                  break;
              case 0x80:
                  printf ("Write");
                  break;
          }
          printf (" from ");
          printIndividual (src);
          printf (" to ");
          printGroup (dest);
          if (buf[1] & 0xC0)
          {
              printf (": ");
              if (len == 2)
                  printf ("%02X", buf[1] & 0x3F);
              else
                  printHex (len - 2, buf + 2);
          }
          printf ("\n");

          if ((buf[1] & 0xC0) == 0x80) { // write
              uint8_t mqttBuf[255], tmpBuf[32], subTopic[MAX_TOPIC_LEN+32];
              char cutShort = 0;

              snprintf(mqttBuf, sizeof(mqttBuf)-1, "Write %d/%d/%d --> ", (dest >> 11) & 0x1f, (dest >> 8) & 0x07, (dest) & 0xff);

              if (len == 2) {
                  sprintf(tmpBuf, "%02X", buf[1] & 0x3F);
                  strcat(mqttBuf, tmpBuf);
              } else {
                  if (len-2 > 8) {
                      cutShort = len-2;
                      len=10;
                  }
                  for (int i=0; i<len-2; i++) {
                      sprintf(tmpBuf, "%02X ", buf[2+i]);
                      strcat(mqttBuf, tmpBuf);
                  }
                  if (cutShort) {
                      sprintf(tmpBuf, "... (original len==%d)", cutShort);
                      strcat(mqttBuf, tmpBuf);
                  }
              }

              publish(ctx, mqttBuf, topic);

              snprintf(subTopic, sizeof(subTopic)-1, "%s/%d/%d/%d", topic,
                      (dest >> 11) & 0x1f, (dest >> 8) & 0x07, (dest) & 0xff);
              publish(ctx, mqttBuf, subTopic);
          }
      }
  }

  EIBClose (con);
  return 0;
}

// vim:expandtab
