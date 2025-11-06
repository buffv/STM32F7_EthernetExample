/*
 * tcp_echo_client.c
 *
 *  Created on: Nov 5, 2025
 *      Author: Banane
 */

#include "tcp_echo_client.h"
#include "lwip/opt.h"
#include "lwip/init.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include "lwip/ip_addr.h"
#include <string.h>
#include <stdlib.h>

#define SERVER_IP_ADDR0 192
#define SERVER_IP_ADDR1 168
#define SERVER_IP_ADDR2 1
#define SERVER_IP_ADDR3 225 // Replace with your server's IP address
#define SERVER_PORT     5000   // Echo port

// Structure to hold client connection state
typedef struct {
  struct tcp_pcb *pcb;
  int retries;
} tcp_client_state_t;

static tcp_client_state_t client_state;

/* Function Prototypes for static helpers */
static void tcp_client_connection_close(struct tcp_pcb *tpcb, tcp_client_state_t *es);
static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len);
static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err);
static void tcp_client_error(void *arg, err_t err);

/**
  * @brief  Closes the current TCP connection and cleans up state.
  */
static void tcp_client_connection_close(struct tcp_pcb *tpcb, tcp_client_state_t *es)
{
  tcp_arg(tpcb, NULL);
  tcp_sent(tpcb, NULL);
  tcp_recv(tpcb, NULL);
  tcp_poll(tpcb, NULL, 0);
  tcp_err(tpcb, NULL);
  tcp_close(tpcb);
  es->pcb = NULL;
  // Note: We don't free the global client_state struct itself, just reset its members.
}

/**
  * @brief  Handles data reception callback from LwIP.
  */
static err_t tcp_client_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  if (err == ERR_OK && p != NULL)
  {
    // Acknowledge receipt of data immediately
    tcp_recved(tpcb, p->tot_len);

    // Process received data (e.g., print it, check if it's the echo)

    err_t wr_err = tcp_write(tpcb, p->payload, p->len, TCP_WRITE_FLAG_COPY);
	if (wr_err == ERR_OK)
	{
		tcp_output(tpcb); // sofort senden
	}

    // For this example, we just free the buffer
    pbuf_free(p);
  }
  else if (err == ERR_OK && p == NULL)
  {
    // Server closed the connection (FIN received)
    tcp_client_connection_close(tpcb, (tcp_client_state_t *)arg);
  }
  return ERR_OK;
}

/**
  * @brief  Callback when data has been successfully sent.
  */
static err_t tcp_client_sent(void *arg, struct tcp_pcb *tpcb, u16_t len)
{
  // This is called after LwIP confirms data has left the buffer/interface.
  // You can initiate the next send operation here if needed.
  return ERR_OK;
}


/**
  * @brief  Error callback function.
  */
static void tcp_client_error(void *arg, err_t err)
{
  // Handle fatal errors or unexpected disconnections
  if (arg != NULL)
  {
    // The PCB is already deallocated by LwIP before this call
    client_state.pcb = NULL;
  }
}

/**
  * @brief  Callback when a connection request completes.
  */
static err_t tcp_client_connected(void *arg, struct tcp_pcb *tpcb, err_t err)
{
  tcp_client_state_t *es = (tcp_client_state_t *)arg;

  if (err == ERR_OK)
  {
    // Connection established successfully!

    // Register the receive and sent callbacks
    tcp_recv(tpcb, tcp_client_recv);
    tcp_sent(tpcb, tcp_client_sent);
    // You can also set a poll timer if desired: tcp_poll(tpcb, tcp_client_poll, 4);

    // --- Send initial data ---
    const char *message = "Hello STM32 LwIP Client!\n";
    tcp_write(tpcb, message, strlen(message), TCP_WRITE_FLAG_COPY);
    tcp_output(tpcb); // Flush the output buffer
    // --- End Send initial data ---
  }
  else
  {
    // Connection failed. Clean up.
    tcp_client_connection_close(tpcb, es);
  }
  return err;
}


/**
  * @brief  Initializes the TCP Client functionality and attempts connection.
  */
void tcp_client_init(void)
{
  printf("tcp client init.\n");
  ip_addr_t server_ip;

  // Set the target server IP address
  IP4_ADDR(&server_ip, SERVER_IP_ADDR0, SERVER_IP_ADDR1, SERVER_IP_ADDR2, SERVER_IP_ADDR3);

  // Initialize the client state struct
  client_state.pcb = NULL;
  client_state.retries = 0;

  // 1. Create a new TCP control block (pcb)
  client_state.pcb = tcp_new();

  if (client_state.pcb != NULL)
  {
    // Pass the state structure to all future callbacks as 'arg'
    tcp_arg(client_state.pcb, &client_state);

    // Set the error callback in case the connection fails unexpectedly
    tcp_err(client_state.pcb, tcp_client_error);

    // 2. Attempt to connect to the remote server
    // The tcp_client_connected function will be called when complete (success or fail)
    printf("Connecting to server...\n");
    tcp_connect(client_state.pcb, &server_ip, SERVER_PORT, tcp_client_connected);
    printf("Connect initiated.\n");
  }
}

