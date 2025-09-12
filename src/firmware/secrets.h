// Arquivo: secrets.h
// Contém as DECLARAÇÕES das variáveis de configuração.
// A definição real está no arquivo .ino principal.

#ifndef SECRETS_H
#define SECRETS_H

// A palavra 'extern' diz ao compilador que estas variáveis existem
// em algum outro arquivo do projeto.
extern const char* ssid;
extern const char* password;
extern const char* mqtt_server;
extern const char* mqtt_user;
extern const char* mqtt_pass;
extern const char* mqtt_topic_req_usrs;
extern const char* mqtt_topic_res_usrs;

#endif