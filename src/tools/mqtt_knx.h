#include <mosquitto.h>

typedef struct {
    const char *broker_host;
    int broker_port;
    int mosq_connected;
    EIBConnection *eib_con;
    struct mosquitto *mosq;
    const char *topic;
} mqttbridge_ctx_t;


eibaddr_t parse_eib_addr_triple(const char *addr);

int mqttpub (EIBConnection *con, const char *broker_host, const int broker_port, const char *topic);
int mqttsub (EIBConnection *con, const char *broker_host, const int broker_port, const char *topic);

void mqttbridge_ensure_init(mqttbridge_ctx_t *ctx);
void mqttbridge_ensure_connect(mqttbridge_ctx_t *ctx);

int publish(mqttbridge_ctx_t *ctx, uint8_t *payload, const char *topic);
