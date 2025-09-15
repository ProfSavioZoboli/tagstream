
#ifndef SECRETS_H
#define SECRETS_H

// A palavra 'static' evita erros de "definição múltipla" quando este arquivo
// é incluído em vários lugares.
static const char* ssid = "Smart 4.0 (3)";
static const char* password = "Smart4.0";
static const char* mqtt_server = "189.8.205.50";
static const char* mqtt_user = "profblu";
static const char* mqtt_pass = "ProFBlu@hotdog";

static const char* mqtt_topic_req_usrs = "/indaial/req/usuarios";
static const char* mqtt_topic_res_usrs = "/indaial/res/usuarios";
static const char* mqtt_topic_log_usuarioLogado = "/indaial/log/usuarioLogado";

#endif
