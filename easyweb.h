/******************************************************************
 *****                                                        *****
 *****  Name: easyweb.h                                       *****
 *****  Ver.: 1.0                                             *****
 *****  Date: 07/05/2001                                      *****
 *****  Auth: Andreas Dannenberg                              *****
 *****        HTWK Leipzig                                    *****
 *****        university of applied sciences                  *****
 *****        Germany                                         *****
 *****  Func: header-file for easyweb.c                       *****
 *****                                                        *****
 *****  Modified by Haotian Wu                                *****
 *****  The Previous important comment is preserved           *****
 ******************************************************************/

#ifndef __EASYWEB_H
#define __EASYWEB_H

const unsigned char GetResponse[] =              // 1st thing our server sends to a client
{
  "HTTP/1.0 200 OK\r\n"                          // protocol ver 1.0, code 200, reason OK
  "Content-Type: text/html\r\n"                  // type of data we want to send
  "\r\n"                                         // indicate end of HTTP-header
};

void InitOsc(void);                              // prototypes
void InitPorts(void);
void HTTPServer(void);
void easyWEB_init(void);
void easyWEB_s_loop(void);
void InsertDynamicValues(void);

unsigned char *PWebSide;                         // pointer to webside
unsigned int HTTPBytesToSend;                    // bytes left to send

unsigned char HTTPStatus;                        // status byte 
#define HTTP_SEND_PAGE               0x01        // help flag

int rx_buff_flag;                                // a flag to check which GET request is sent
#define DEFAULT_PAGE                 11
#define REFRESH_PAGE                 12
#define VMAX                         15
#define SETTING_PAGE                 13
#define MONITOR_PAGE                 14

int request_check;                                // a flag to check whether the HTTP request is known or not
#define KNOWN                        21
#define UNKNOWN                      20

int RX_buff_flag;                                   //The value of rx_buff_flag will be passed to this flag
                                                    // if an HTTP request is known
int incoming_check;            // This is an important flag to check whether there is an incoming HTTP request
                               //Otherwise the server will always try to send web page before the HTTP request
                               // is examined
int send_flag;                 // A flag for continuous TCP transmission
#define CONTINUE                     1
#define FINISHED                     0

#endif

