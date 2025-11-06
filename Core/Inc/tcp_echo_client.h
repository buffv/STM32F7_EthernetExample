/*
 * tcp_echo_client.h
 *
 *  Created on: Nov 5, 2025
 *      Author: Banane
 */

#ifndef INC_TCP_ECHO_CLIENT_H_
#define INC_TCP_ECHO_CLIENT_H_


#ifdef __cplusplus
extern "C" {
#endif

#include "lwip/tcp.h"

/**
  * @brief  Initializes the TCP Echo Client functionality.
  *         Call this function once after MX_LWIP_Init().
  */
void tcp_client_init(void);

#ifdef __cplusplus
}
#endif


#endif /* INC_TCP_ECHO_CLIENT_H_ */
