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
#include "mqtt_knx.h"
#include "path.h"
#include <time.h>
#include <fcntl.h>
#include <string.h>
#ifdef HAVE_SYS_TIME_H
#include <sys/time.h>
#endif

#include <mosquitto.h>

#define MAX_TOPIC_LEN 64


void mqttbridge_ensure_init(mqttbridge_ctx_t *ctx);
void mqttbridge_ensure_connect(mqttbridge_ctx_t *ctx);

int publish(mqttbridge_ctx_t *ctx, uint8_t *payload, const char *topic) {
    mqttbridge_ensure_connect(ctx);
    printf("Publishing to topic %s: %s\n", topic, payload);
    int rc = mosquitto_publish(ctx->mosq, NULL, topic, strlen(payload), payload, 2, false);
    if (rc != MOSQ_ERR_SUCCESS) {
        die("failed to publish. rc==%d.", rc);
    }
}

void mqttbridge_ensure_connect(mqttbridge_ctx_t *ctx) {
    int rc;
    mqttbridge_ensure_init(ctx);
    if (!ctx->mosq_connected) {
        rc = mosquitto_connect(ctx->mosq, ctx->broker_host, ctx->broker_port, 15);
        if (rc != MOSQ_ERR_SUCCESS) {
            die("failed to connect. rc==%d.", rc);
        }
        ctx->mosq_connected=1;
        rc = mosquitto_loop_start(ctx->mosq);
        if (rc != MOSQ_ERR_SUCCESS) {
            die("failed to start mosquitto loop. rc==%d.", rc);
        }
    }
}

void mqttbridge_ensure_init(mqttbridge_ctx_t *ctx) {
    if (ctx->mosq==NULL) {
        mosquitto_lib_init();
        ctx->mosq = mosquitto_new(NULL, true, ctx);
        if(!ctx->mosq){
            if (errno) {
                die("failed to initialize mosquitto: %s", strerror(errno));
            }
            die("failed to initialize mosquitto.");
        }
    }
}


eibaddr_t parse_eib_addr_triple(const char *addr) {
  unsigned int a, b, c, res;
  res = sscanf (addr, "%u/%u/%u", &a, &b, &c);
  if (res == 3 && a <= 0x1F && b <= 0x07 && c <= 0xFF)
    return (a << 11) | (b << 8) | c;
  return 0;
}
// vim:expandtab
