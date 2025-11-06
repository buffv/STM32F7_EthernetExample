/*
 * tcp_echo_server.h
 *
 *  Created on: Nov 5, 2025
 *      Author: Banane
 */

#ifndef INC_TCP_ECHO_SERVER_H_
#define INC_TCP_ECHO_SERVER_H_

#ifdef __cplusplus
 extern "C" {
#endif

#include "lwip/tcp.h"

/**
  * @brief  Initializes the TCP Echo Server.
  *         Call this function once after MX_LWIP_Init().
  */
void tcp_server_init(void);

#ifdef __cplusplus
}
#endif

#endif /* SRC_TCP_ECHO_SERVER_H_ */
