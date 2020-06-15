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
#include <signal.h>
#include <fcntl.h>
#include <string.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include "mqtt_knx.h"

#define MAX_TOPIC_LEN 64


static int run = 1;

void handle_signal(int s)
{
        run = 0;
}

const char *triple_trailer_from_topic(const char* topic);
void connect_callback(struct mosquitto *mosq, void *obj, int result);
void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message);

int mqttsub (EIBConnection *con, const char *broker_host, const int broker_port, const char *topic)
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
  ctx->eib_con = NULL;

  /* Buffering stdout is almost never what we want */
  setvbuf(stdout, NULL, _IOLBF, 0);

  mqttbridge_ensure_init(ctx);

  ctx->eib_con = con;

  signal(SIGINT, handle_signal);
  signal(SIGTERM, handle_signal);

  mosquitto_connect_callback_set(ctx->mosq, connect_callback);
  mosquitto_message_callback_set(ctx->mosq, message_callback);

  int rc = mosquitto_connect(ctx->mosq, broker_host, broker_port, 60);
  mosquitto_subscribe(ctx->mosq, NULL, topic, 0);
  printf("entering loop.\n");
  while (run) {
      usleep(500);
      rc = mosquitto_loop(ctx->mosq, -1, 1);
      if(run && rc){
          printf("connection error!\n");
          sleep(3);
          mosquitto_reconnect(ctx->mosq);
      }
  }
  mosquitto_destroy(ctx->mosq);

  EIBClose (ctx->eib_con);
  return 0;
}

void connect_callback(struct mosquitto *mosq, void *obj, int result)
{
    printf("connect callback, rc=%d\n", result);
}

void message_callback(struct mosquitto *mosq, void *obj, const struct mosquitto_message *message)
{
    mqttbridge_ctx_t *ctx = (mqttbridge_ctx_t *)obj;
    bool match = 0;
    printf("got message '%.*s' for topic '%s'\n", message->payloadlen, (char*) message->payload, message->topic);

    mosquitto_topic_matches_sub(ctx->topic, message->topic, &match);
    if (match) {
        const char *eib_addr = triple_trailer_from_topic(message->topic);
        if (!eib_addr) {
            printf("ignoring invalid topic [%s].\n", message->topic);
            return;
        }
        eibaddr_t dest = parse_eib_addr_triple(eib_addr);
        if (!dest) {
            printf("ignoring invalid destination address [%s].\n", eib_addr);
            return;
        }
        printf("KNX injection for addr [%s] requested. Payload: \"%s\".\n", eib_addr, (char*)message->payload);

        uint8_t lbuf[3] = { 0x0, 0x80 };
        lbuf[1] |= readHex((char*)message->payload) & 0x3f;
        EIBReset(ctx->eib_con);
        if (EIBOpenT_Group(ctx->eib_con, dest, 1) == -1) {
            die ("Connect failed");
        }

        int len = EIBSendAPDU(ctx->eib_con, 2, lbuf);
        if (len == -1) {
            die ("Request failed");
        }
        printf("KNX Message sent: %x %x %x -> %s\n", lbuf[0], lbuf[1], lbuf[2], eib_addr);
    }
}

// return the last 3 /-separated element-string from topic
// "foo/bar/1/2/3" --> "1/2/3"
const char *triple_trailer_from_topic(const char* topic) {
    int idx;
    int slashes = 0;
    for (idx=0; topic[idx]; idx++) {
        if (topic[idx] == '/') slashes++;
    }
    if (slashes < 2) {
        return NULL;
    } else if (slashes == 2) {
        return topic;
    }
    for (idx=0; slashes > 2; idx++) {
        if (topic[idx] == '/') slashes--;
    }
    return topic+idx;
}

// vim:expandtab
