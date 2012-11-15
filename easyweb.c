/******************************************************************
 *****                                                        *****
 *****  Name: easyweb.c                                       *****
 *****  Ver.: 1.0                                             *****
 *****  Date: 07/05/2001                                      *****
 *****  Auth: Andreas Dannenberg                              *****
 *****        HTWK Leipzig                                    *****
 *****        university of applied sciences                  *****
 *****        Germany                                         *****
 *****  Func: implements a dynamic HTTP-server by using       *****
 *****        the easyWEB-API                                 *****
 *****                                                        *****
 *****  Modified by Haotian Wu                                *****
 *****  The Previous important comment is preserved           *****
 ******************************************************************/

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "LPC17xx.h"
#include "type.h"
#include "control/control.h"
#include "comm/comm.h"
#include "comm/commands.h"
#define extern            // Keil: Line added for modular project management

#include "easyweb.h"
#include "EMAC.h"         // Keil: *.c -> *.h    // ethernet packet driver
#include "tcpip.h"        // Keil: *.c -> *.h    // easyWEB TCP/IP stack
#include "webpage.h"                             // webside for our HTTP server (HTML)

volatile uint32_t temp;
void _delay(uint32_t del);
extern long Vo, Vi, Io, Ii, Iop,Vref, Imax, Vmax;  //Core parameter for control and monitor
extern int mode; //Core parameter for bi-direction control
int exam_rx_buffer(void);





void easyWEB_init(void)
{ 
	TCPLowLevelInit();
	HTTPStatus = 0;                   // clear HTTP-server's flag register
	TCPLocalPort = TCP_PORT_HTTP;     // set port we want to listen to
	incoming_check=0;
	send_flag=CONTINUE;
}
void easyWEB_s_loop(void){   //This is a single loop for an HTTP request and response
    if (!(SocketStatus & SOCK_ACTIVE))
    { 
      TCPPassiveOpen();   // listen for incoming TCP-connection
    }
    DoNetworkStuff();     // handle network and easyWEB-stack
                          // events
    HTTPServer();
  }

// This function implements a very simple dynamic HTTP-server.
// It waits until connected, then sends a HTTP-header and the
// HTML-code stored in memory. Before sending, it replaces
// some special strings with dynamic values.
// NOTE: For strings crossing page boundaries, replacing will
// not work. In this case, simply add some extra lines
// (e.g. CR and LFs) to the HTML-code.

void _delay(uint32_t del)
{
	uint32_t i;
	for(i=0;i<del;i++)
		temp = i;
}

void HTTPServer(void)
{
	if (SocketStatus & SOCK_CONNECTED)             // check if somebody has connected to our TCP
	{
		if (SocketStatus & SOCK_DATA_AVAILABLE)      // check if remote TCP sent data
		{
			incoming_check=1;
			rx_buff_flag=exam_rx_buffer();           //check the content of RX buffer
			if (request_check==KNOWN){
				RX_buff_flag=rx_buff_flag;          //pass valid request for further process
			}
			TCPReleaseRxBuffer();                   // Clean RX buffer
		}
	if(incoming_check==1 || send_flag==CONTINUE){
		if (SocketStatus & SOCK_TX_BUF_RELEASED)     // check if buffer is free for TX
		{
			print("incoming data \r\n");
			if (!(HTTPStatus & HTTP_SEND_PAGE))        // init byte-counter and pointer to webside
			{ 	                                         // if called the 1st time
				print("Select web page\r\n");
				if((RX_buff_flag==REFRESH_PAGE)){     //Following if and else if are used for sending different web pages
					HTTPBytesToSend = sizeof(WebSide_par) - 1;
    				PWebSide = (unsigned char *)WebSide_par;
				}else if ((RX_buff_flag==DEFAULT_PAGE)){
					HTTPBytesToSend = sizeof(WebSide_home) - 1;
					PWebSide = (unsigned char *)WebSide_home;
				}else if ((RX_buff_flag==SETTING_PAGE)){
					HTTPBytesToSend = sizeof(WebSide_Setting) - 1;
					PWebSide = (unsigned char *)WebSide_Setting;
				}else if ((RX_buff_flag==MONITOR_PAGE)){
					HTTPBytesToSend = sizeof(WebSide_monitor) - 1;
					PWebSide = (unsigned char *)WebSide_monitor;
				}else{
					HTTPBytesToSend = sizeof(WebSide_home) - 1;   // get HTML length, ignore trailing zero
					PWebSide = (unsigned char *)WebSide_home;     // pointer to HTML-code
				}
			}
			if (HTTPBytesToSend > MAX_TCP_TX_DATA_SIZE)     // transmit a segment of MAX_SIZE
			{
				send_flag=CONTINUE;
				if (!(HTTPStatus & HTTP_SEND_PAGE))           // 1st time, include HTTP-header
				{
					print("send overzised data\r\n");
					memcpy(TCP_TX_BUF, GetResponse, sizeof(GetResponse) - 1);
					memcpy(TCP_TX_BUF + sizeof(GetResponse) - 1, PWebSide, MAX_TCP_TX_DATA_SIZE - sizeof(GetResponse) + 1);
					HTTPBytesToSend -= MAX_TCP_TX_DATA_SIZE - sizeof(GetResponse) + 1;
					PWebSide += MAX_TCP_TX_DATA_SIZE - sizeof(GetResponse) + 1;
				}else{
					memcpy(TCP_TX_BUF, PWebSide, MAX_TCP_TX_DATA_SIZE);
					HTTPBytesToSend -= MAX_TCP_TX_DATA_SIZE;
					PWebSide += MAX_TCP_TX_DATA_SIZE;
				}
				TCPTxDataCount = MAX_TCP_TX_DATA_SIZE;   // bytes to xfer
				InsertDynamicValues();                   // exchange some strings...
				TCPTransmitTxBuffer();                   // xfer buffer
			}else if (HTTPBytesToSend)                  // transmit leftover bytes
			{
				send_flag=FINISHED;
				print("send in-sized data\r\n");
				memcpy(TCP_TX_BUF, PWebSide, HTTPBytesToSend);
				TCPTxDataCount = HTTPBytesToSend;        // bytes to xfer
				InsertDynamicValues();                   // exchange some strings...
				TCPTransmitTxBuffer();                   // send last segment
				TCPClose();                              // and close connection
				HTTPBytesToSend = 0;                     // all data sent
			}
			HTTPStatus |= HTTP_SEND_PAGE;              // ok, 1st loop executed
    		}
		incoming_check=0;
	}
	}else{
		HTTPStatus &= ~HTTP_SEND_PAGE;               // reset help-flag if not connected
    }
}


void InsertDynamicValues(void)
{
  print("InsertDynamicValues\r\n");
  unsigned char *Key;
           char NewKey[7];
  unsigned int i;
  
  if (TCPTxDataCount < 4) return;                     // there can't be any special string
  
  Key = TCP_TX_BUF;
  float temp;
  for (i = 0; i < (TCPTxDataCount - 3); i++)  //match special pattern in html code, replace that with actual data
  {
    if (*Key == 'A')
     if (*(Key + 1) == 'D')
       if (*(Key + 3) == 'O')
         switch (*(Key + 2))
         {
           case '1' :
           {
        	 int integer1, decimal1;
        	 temp=(Vi*Vdd*28)/(4095*3);
        	 integer1=(int)temp;
        	 temp-=integer1;
        	 temp*=1000;
        	 decimal1=(int)temp;
        	 sprintf(NewKey, "%3d.%03d ", integer1, decimal1); //This is a way to display floating point number
             memcpy(Key, NewKey, 7);
             break;
           }
           case '2' :
           {
        	   int integer2, decimal2;
          	 temp=(Vo*Vdd*28)/(4095*3);
          	 integer2=(int)temp;
          	 temp-=integer2;
          	 temp*=1000;
          	 decimal2=(int)temp;
        	   sprintf(NewKey, "%3d.%03d ", integer2, decimal2);
             memcpy(Key, NewKey, 7);
             break;
           }
           case '3' :
           {
        	   int integer3, decimal3;
          	 temp=(Ii*Vdd*2)/(4095*0.151);
          	 integer3=(int)temp;
          	 temp-=integer3;
          	 temp*=1000;
          	 decimal3=(int)temp;
        	   sprintf(NewKey, "%3d.%03d ", integer3, decimal3);
            memcpy(Key, NewKey, 7);
             break;
           }
           case '4' :
           {
        	 int integer4, decimal4;
          	 temp=(Io*Vdd*2)/(4095*0.151);
          	 integer4=(int)temp;
          	 temp-=integer4;
          	 temp*=1000;
          	 decimal4=(int)temp;
        	   sprintf(NewKey, "%3d.%03d ", integer4, decimal4);
             memcpy(Key, NewKey, 7);
             break;
           }
           case '6' :
           {
        	   char* DEFAULT="Default";
        	   char* REVERSE="Reverse";
           	   if (mode==1){
           	   memcpy(Key, DEFAULT, 7);
           	   }
           	   if (mode==0){
           		   memcpy(Key, REVERSE, 7);
           	   }
           	   break;
           }
           case '7' :
           {
        	   int integer7, decimal7;
        	   temp=(Vmax * Vdd * 28) / (4095 * 3);
        	   integer7=(int)temp;
        	   temp-=integer7;
        	   temp*=1000;
        	   decimal7=(int)temp;
        	   sprintf(NewKey, "%3d.%03d ", integer7, decimal7);
        	   memcpy(Key, NewKey, 7);
        	   break;
           }
           case '8' :
           {
        	   int integer8, decimal8;
        	   temp=(Imax * Vdd * 2) / (4095 * 0.151);
        	   integer8=(int)temp;
        	   temp-=integer8;
        	   temp*=1000;
        	   decimal8=(int)temp;
        	   sprintf(NewKey, "%3d.%03d ", integer8, decimal8);
        	   memcpy(Key, NewKey, 7);
        	   break;
           }
           case '9' :
           {
        	   int integer9, decimal9;
        	   temp=(Vref * Vdd * 28) / (4095 * 3);
        	   integer9=(int)temp;
        	   temp-=integer9;
        	   temp*=1000;
        	   decimal9=(int)temp;
        	   sprintf(NewKey, "%3d.%03d ", integer9, decimal9);
        	   memcpy(Key, NewKey, 7);
        	   break;
           }
         }
    Key++;
  }
}
/**
 * int exam_rx_buffer(void)
 * Check all HTTP GET and POST request by examine specific string
 * If the string is matched, a flag will be returned to tell HTTP server
 * to response with correct web page. All unknown GET and POST request will make
 * web server response the last responsed web page
 *
 * For POST request, this function will also change the value of core parameters such as
 * vmax, vref, etc.In this way, remote control of DC/DC converter is implemented
 */
int exam_rx_buffer(void){

	unsigned char* get_default="GET / HTTP/1.1";
	unsigned char* get_refresh="GET /par.html";
	unsigned char* get_index="GET /index.html";
	unsigned char* get_setting="GET /setting.html";
	unsigned char* get_monitor="GET /monitor.html";
	unsigned char* post_mode="mode=";
	unsigned char* post_imax="imax=";
	unsigned char* post_vref="vref=";

	if (strstr(TCP_RX_BUF,get_default)!=NULL){
		request_check=KNOWN;
		return DEFAULT_PAGE;
	}
	else if (strstr(TCP_RX_BUF,get_refresh)!=NULL){
		request_check=KNOWN;
		return REFRESH_PAGE;
	}else if (strstr(TCP_RX_BUF,get_index)!=NULL){
		request_check=KNOWN;
		return DEFAULT_PAGE;
	}else if (strstr(TCP_RX_BUF,get_setting)!=NULL){
		request_check=KNOWN;
		return SETTING_PAGE;
	}else if (strstr(TCP_RX_BUF,get_monitor)!=NULL){
		request_check=KNOWN;
		return MONITOR_PAGE ;
	}
	////////////////////////////////////////////////////////////////////////////// Control part
	else if (strstr(TCP_RX_BUF,post_imax)!=NULL){
		unsigned char* start_point=strstr(TCP_RX_BUF,post_imax);
		int param_value=atoff(start_point+5);
		if((param_value >= 0) && (param_value <= 6)){
			Imax = (int)(param_value * 4095 * 0.151) / (Vdd * 2);   //Control the parameter in the same way that Jorge did
		}
		request_check=KNOWN;
		return DEFAULT_PAGE;
	}
	else if (strstr(TCP_RX_BUF,post_vref)!=NULL){
		unsigned char* start_point=strstr(TCP_RX_BUF,post_vref);
		int param_value=atoff(start_point+5);
		param_value=(param_value * 4095 * 3) /(Vdd * 28);
		if((param_value >= 0) && (param_value <= Vmax)){
			Vref = (int)param_value;
		}
		request_check=KNOWN;
		return DEFAULT_PAGE;
	}
	else if (strstr(TCP_RX_BUF,post_mode)!=NULL){
		unsigned char* start_point=strstr(TCP_RX_BUF,post_mode);
		int param_value=atoff(start_point+5);
		if((param_value == 0) || (param_value == 1)){
			mode=(int)param_value;
		}
		request_check=KNOWN;
		return DEFAULT_PAGE;
	}
	/////////////////////////////////////////////////////////////////////////////////
	else{
		request_check=UNKNOWN;
		return DEFAULT_PAGE;
	}
}
