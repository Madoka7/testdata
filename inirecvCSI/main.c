/*
 * =====================================================================================
 *       Filename:  main.c
 *
 *    Description:  Here is an example for receiving CSI matrix 
 *                  Basic CSi procesing fucntion is also implemented and called
 *                  Check csi_fun.c for detail of the processing function
 *        Version:  1.0
 *
 *         Author:  Yaxiong Xie
 *         Email :  <xieyaxiongfly@gmail.com>
 *   Organization:  WANDS group @ Nanyang Technological University
 *   
 *   Copyright (c)  WANDS group @ Nanyang Technological University
 * =====================================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <termios.h>
#include <pthread.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>

/*start:    xq ssdut edited*/
#include <arpa/inet.h>
#include <sys/socket.h>
#include <linux/socket.h>
#include <netinet/in.h>

#define _READ_IP_PORT_FROM_
#define SERV_PORT 8000
int serv_port = -1;

#define SERV_IP "172.20.20.1"
char serv_ip[16];
/*end:      xq ssdut edited*/


#include "csi_fun.h"

#define BUFSIZE 4096

int quit;
unsigned char buf_addr[BUFSIZE];
unsigned char data_buf[1500];

COMPLEX csi_matrix[3][3][114];
csi_struct*   csi_status;

void sig_handler(int signo)
{
    if (signo == SIGINT)
        quit = 1;
}

int main(int argc, char* argv[])
{
    FILE*       fp;
    int         fd;
    int         i;
    int         total_msg_cnt,cnt;
    int         log_flag;
    unsigned char endian_flag;
    u_int16_t   buf_len;
    
	
	/*xq: add a socket here*/
	int sockfd;
	struct sockaddr_in servaddr;
	
	/*creat a socket, and sequently test socket existance*/
    if((sockfd = socket(AF_INET, SOCK_STREAM,0))<0){
		printf("socket error\n");
	}
	/*initialize the servaddr struct*/
	memset(&servaddr,0,sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(SERV_PORT);
	//servaddr.sin_addr.s_addr = INADDR_ANY;
	
	
	
	
	/*socket ends here*/
	
	
	log_flag = 1;
    csi_status = (csi_struct*)malloc(sizeof(csi_struct));
    /* check usage */
    if (1 == argc){
    
	
	/*modified by xq, use argv[0] to set server ip address*/
	/*thats why the following code has been commented*/
	// 	/* If you want to log the CSI for off-line processing,
    //     * you need to specify the name of the output file
    //     */
	// 	
      log_flag  = 0;
      printf("/**************************************/\n");
      printf("/*   Usage: recv_csi [server ipaddress]    */\n");
      printf("/**************************************/\n");
		
	/*do nothing here*/
		
		
    }
    if (2 == argc){
		/*modify xq, set server address here.*/
			//fp = fopen(argv[1],"w");
			//if (!fp){
			//    printf("Fail to open <output_file>, are you root?\n");
			//    fclose(fp);
			//    return 0;
			//}
			//
			if(is_big_endian())
			    endian_flag = 0xff;
			else
			    endian_flag = 0x0;
			
			//fwrite(&endian_flag,1,1,fp);   
		
		/*then execaute pton and connect to server*/

		
		strcpy(serv_ip, argv[1]);
		if(inet_pton(AF_INET, serv_ip, &servaddr.sin_addr)<=0)
		{
			printf("inet_pton error");
		}

		if(connect(sockfd, (struct sockaddr*)&servaddr, sizeof(servaddr))<0)
		{
			printf("connect error to:\n");
			printf("%s\n",serv_ip);
			return 0;
		}

		write(sockfd, &endian_flag, sizeof(endian_flag));
	
	printf("connect succeed");
    }
    if (argc > 2){
        printf(" Too many input arguments !\n");
        return 0;
    }

    fd = open_csi_device();
    if (fd < 0){
        perror("Failed to open the device...");
        return errno;
    }
    
    printf("#Receiving data! Press Ctrl+C to quit!\n");

    quit = 0;
    total_msg_cnt = 0;
    
    while(1){
        if (1 == quit){
            return 0;
            //fclose(fp);
            close_csi_device(fd);
        }

        /* keep listening to the kernel and waiting for the csi report */
        cnt = read_csi_buf(buf_addr,fd,BUFSIZE);

        if (cnt){
            total_msg_cnt += 1;

            /* fill the status struct with information about the rx packet */
            record_status(buf_addr, cnt, csi_status);

            /* 
             * fill the payload buffer with the payload
             * fill the CSI matrix with the extracted CSI value
             */
            record_csi_payload(buf_addr, csi_status, data_buf, csi_matrix); 
            
            /* Till now, we store the packet status in the struct csi_status 
             * store the packet payload in the data buffer
             * store the csi matrix in the csi buffer
             * with all those data, we can build our own processing function! 
             */
            //porcess_csi(data_buf, csi_status, csi_matrix);   
            
            printf("Recv %dth msg with rate: 0x%02x | payload len: %d\n",total_msg_cnt,csi_status->rate,csi_status->payload_len);
            
            /* log the received data for off-line processing */
			
			/*modified by xq: we need to send the CSI data via socket*/
            if (log_flag){
                buf_len = csi_status->buf_len;
				/*the following two write are related to socket*/
                write(sockfd, &buf_len, sizeof(u_int16_t));
				write(sockfd, buf_addr,buf_len);
				
				/*the following two fwrite are from Xie Yaxiong*/
				//fwrite(&buf_len,1,2,fp);
                //fwrite(buf_addr,1,buf_len,fp);
            }
        }
    }
    //fclose(fp);
    close_csi_device(fd);
    free(csi_status);
    return 0;
}
