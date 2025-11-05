#ifndef SECRETS_H
#define SECRETS_H

static const char* ssid = "Smart 4.0 (3)";
static const char* password = "Smart4.0";
static const char* mqtt_server = "189.8.205.50";
static const char* mqtt_user = "profblu";
static const char* mqtt_pass = "ProFBlu@hotdog";
static const char* mqtt_topic_req_usrs = "/indaial/req/usuarios";
static const char* mqtt_topic_res_usrs = "/indaial/res/usuarios";
static const char* mqtt_topic_log_usuarioLogado = "/indaial/log/usuarioLogado";
static const char* mqtt_topic_req_eqps = "/indaial/req/equipamentos";
static const char* mqtt_topic_res_eqps = "/indaial/res/equipamentos";
static const char* mqtt_topic_sync_eqps = "/indaial/sync/equipamentos";
static const char* mqtt_topic_check_usrs = "/indaial/check/usuarios";
static const char* mqtt_topic_ack_eqps = "/indaial/ack/equipamentos";
static const char* mqtt_topic_ack_usrs = "/indaial/ack/usuarios";
static const char* mqtt_topic_log_movimentacao = "/indaial/log/movimentacao";

#endif