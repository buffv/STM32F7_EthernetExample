/*
 * tcp_echo_server.c
 *
 *  Created on: Nov 5, 2025
 *      Author: BUV
 */

#include "tcp_echo_server.h"
#include "lwip/opt.h"
#include "lwip/init.h"
#include "lwip/mem.h"
#include "lwip/pbuf.h"
#include <string.h>
#include <stdlib.h> // Required for malloc/free

#define TCP_PORT 7  // Echo port
#define TCP_SERVER_BUF_SIZE 256

// Structure to hold connection state information
typedef struct {
  struct tcp_pcb *pcb;
  uint8_t buffer[TCP_SERVER_BUF_SIZE];
  int buflen;
} tcp_server_connection_t;

// Global pointer to the current active connection state (simplification for single connection)
static tcp_server_connection_t *tcp_connection_state;

/* Function Prototypes for static helpers */
static void tcp_server_error(void *arg, err_t err);
static err_t tcp_server_poll(void *arg, struct tcp_pcb *pcb);
static void tcp_server_send(struct tcp_pcb *tpcb, tcp_server_connection_t *es);
static void tcp_server_connection_close(struct tcp_pcb *tpcb, tcp_server_connection_t *es);
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err);
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err);


/**
  * @brief  Closes the current TCP connection and frees memory.
  * @param  tpcb: pointer to the PCB to close
  * @param  es: pointer to the connection state structure
  */
static void tcp_server_connection_close(struct tcp_pcb *tpcb, tcp_server_connection_t *es)
{
  tcp_arg(tpcb, NULL);
  tcp_sent(tpcb, NULL);
  tcp_recv(tpcb, NULL);
  tcp_poll(tpcb, NULL, 0);
  tcp_err(tpcb, NULL);
  tcp_close(tpcb);
  if (es != NULL) {
    free(es); // Free the state structure allocated with malloc
    tcp_connection_state = NULL;
  }
}

/**
  * @brief  Handles data reception callback from LwIP.
  *         Copies data, processes it (echoes it back), and frees the pbuf.
  */
static err_t tcp_server_recv(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err)
{
  tcp_server_connection_t *es = (tcp_server_connection_t *)arg;

  if (err == ERR_OK && p != NULL)
  {
    // Inform LwIP that we have successfully taken the data from the pbuf
    tcp_recved(tpcb, p->tot_len);

    // Copy data from the pbuf into our application buffer
    if (p->tot_len <= TCP_SERVER_BUF_SIZE)
    {
      pbuf_copy_partial(p, es->buffer, p->tot_len, 0);
      es->buflen = p->tot_len;

      // Echo the data back to the client
      tcp_server_send(tpcb, es);
    }

    // Free the pbuf after copying the data
    pbuf_free(p);
  }
  else if (err == ERR_OK && p == NULL)
  {
    // Client closed the connection (FIN received)
    tcp_server_connection_close(tpcb, es);
  }

  // If we return anything other than ERR_OK here, LwIP will free the PCB
  return ERR_OK;
}

/**
  * @brief  Helper function to enqueue and transmit data.
  */
static void tcp_server_send(struct tcp_pcb *tpcb, tcp_server_connection_t *es)
{
  // Enqueue the data into LwIP's send buffer using a copy flag
  err_t err = tcp_write(tpcb, es->buffer, es->buflen, TCP_WRITE_FLAG_COPY);

  if (err == ERR_OK)
  {
    // Data is enqueued, now trigger the actual transmission immediately
    tcp_output(tpcb);
  }
  // Error handling (e.g., if send buffer is full, handle it here or in a sent callback)
}

/**
  * @brief  Poll callback function (called periodically by LwIP timer).
  */
static err_t tcp_server_poll(void *arg, struct tcp_pcb *pcb)
{
  // Can be used for keep-alives or timeouts if needed
  return ERR_OK;
}

/**
  * @brief  Error callback function.
  */
static void tcp_server_error(void *arg, err_t err)
{
  // Handle fatal errors or unexpected disconnections here
  if (arg != NULL)
  {
    tcp_server_connection_t *es = (tcp_server_connection_t *)arg;
    free(es);
    tcp_connection_state = NULL;
  }
}

/**
  * @brief  Accepts a new connection and registers callbacks.
  */
static err_t tcp_server_accept(void *arg, struct tcp_pcb *newpcb, err_t err)
{
  // If we already have a connection, reject this new one (simple single-client server model)
  if (tcp_connection_state != NULL) {
    tcp_server_connection_close(newpcb, NULL); // Closes the new incoming PCB
    return ERR_ABRT;
  }

  // Allocate memory for the new connection state
  tcp_connection_state = (tcp_server_connection_t *)malloc(sizeof(tcp_server_connection_t));
  if (tcp_connection_state == NULL) return ERR_MEM;

  tcp_connection_state->pcb = newpcb;
  tcp_connection_state->buflen = 0;

  // Pass the state structure pointer to all future callbacks for this PCB
  tcp_arg(newpcb, tcp_connection_state);

  // Set the receive, error, and poll callback functions
  tcp_recv(newpcb, tcp_server_recv);
  tcp_err(newpcb, tcp_server_error);
  tcp_poll(newpcb, tcp_server_poll, 2); // Poll every 2 LwIP slow ticks (approx 1 second)

  return ERR_OK;
}

/**
  * @brief  Initializes the TCP Echo Server.
  *         Called from main() after LwIP initialization.
  */
void tcp_server_init(void)
{
  struct tcp_pcb *tpcb;

  // 1. Create a new TCP control block (pcb) in the listening state
  tpcb = tcp_new();

  if (tpcb != NULL)
  {
    err_t err;

    // 2. Bind the pcb to a local IP address (IP_ADDR_ANY for all interfaces) and port
    err = tcp_bind(tpcb, IP_ADDR_ANY, TCP_PORT);

    if (err == ERR_OK)
    {
      // 3. Start listening for incoming connections
      tpcb = tcp_listen(tpcb);

      // 4. Initialize the accept callback function that handles new clients
      tcp_accept(tpcb, tcp_server_accept);
    }
    else
    {
      // Deallocate the pcb if binding fails
      tcp_pcb_remove(tpcb);
    }
  }
}


