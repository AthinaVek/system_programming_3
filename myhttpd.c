#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>
#include <sys/stat.h>
#include "QueueInterface.h"

Queue queue;
pthread_cond_t cond;
pthread_mutex_t mutex;
char rDir[128];
int sum=0, pages=0, bytes=0, numThreads;
pthread_t *threads;
time_t startTime;
        

void *threadHandler(){
    char *fileName, buf[512], timee[256], headerBuf[2048], bbb[10];
    char line1[128], line2[128], line3[128], line4[128], line5[128], line6[128];
    size_t readBytes;
    ItemType data;
    FILE *fp;
    int pgBytes=0, pg=0;
    time_t curTime;
    struct tm *timeInfo;
    struct stat fileStat;

    signal(SIGPIPE, SIG_IGN);
    while (1){
        pthread_mutex_lock(&mutex);
        while (Empty(&queue)){   //perimenw na mpei kati sthn oura
            pthread_cond_wait(&cond, &mutex);
        }
        Remove(&queue, &data);
        pthread_mutex_unlock(&mutex);

        fileName = data.msg;
        if (access(fileName, F_OK) == 0){  //an to arxeio uparxei
            if (access(fileName, R_OK) == 0){ //an exw permissions
                if ((fp = fopen(fileName, "r"))){
                    printf("fileName: %s\n", fileName);

                    strcpy(line1, "HTTP/1.1 200 OK\r\n");   //ftiaxnw tis grammes tou http header
                    time(&curTime);
                    timeInfo = gmtime(&curTime);
                    strcpy(timee, asctime(timeInfo));
                    timee[strlen(timee)-1] = '\0';
                    sprintf(line2, "Date: %s GMT\r\n", timee);
                    strcpy(line3, "Server: myhttpd/1.0.0 (Ubuntu64)\r\n");
                    stat(fileName, &fileStat);
                    sprintf(line4, "Content-Length: %ld\r\n", fileStat.st_size);
                    strcpy(line5, "Content-Type: text/html\r\n");
                    strcpy(line6, "Connection: Closed\r\n");
                    
                    memset(headerBuf, '\0', sizeof(headerBuf));

                    strcpy(headerBuf, line1);
                    strcat(headerBuf, line2);
                    strcat(headerBuf, line3);
                    strcat(headerBuf, line4);
                    strcat(headerBuf, line5);
                    strcat(headerBuf, line6);
                    strcat(headerBuf, "\r\n");
                    send(data.sock, headerBuf, strlen(headerBuf), 0);
                    headerBuf[0] = '\0';
                    sleep(1);

                    memset(buf, '\0', sizeof(buf)); // stelnw to arxeio
                    while ((readBytes = fread(buf, sizeof(char), 512, fp)) > 0){
                        if (send(data.sock, buf, readBytes, 0) < 0)
                            break;

                        pgBytes += readBytes;
                        memset(buf, '\0', sizeof(buf));
                    }
                    fclose(fp);

                    pthread_mutex_lock(&mutex);
                    bytes += pgBytes;
                    pages++;
                    pthread_mutex_unlock(&mutex);
                }
            }
            else{    //an den exoume permission
                strcpy(line1, "HTTP/1.1 403 Forbidden\r\n");   //ftiaxnw tis grammes tou http header
                time(&curTime);
                timeInfo = gmtime(&curTime);
                strcpy(timee, asctime(timeInfo));
                timee[strlen(timee)-1] = '\0';
                sprintf(line2, "Date: %s GMT\r\n", timee);
                strcpy(line3, "Server: myhttpd/1.0.0 (Ubuntu64)\r\n");
                sprintf(line4, "Content-Length: 124\r\n");
                strcpy(line5, "Content-Type: text/html\r\n");
                strcpy(line6, "Connection: Closed\r\n");
                //sz = strlen(line1)+strlen(line2)+strlen(line3)+strlen(line4)+strlen(line5);

                send(data.sock, line1, strlen(line1), 0);
                send(data.sock, line2, strlen(line2), 0);
                send(data.sock, line3, strlen(line3), 0);
                send(data.sock, line4, strlen(line4), 0);
                send(data.sock, line5, strlen(line5), 0);
                send(data.sock, line6, strlen(line6), 0);
                send(data.sock, "\r\n", 2, 0);
                send(data.sock, "<html>Trying to access this file but don't think I can make it.</html>\r\n", 72, 0);
            }
        }
        else{   //an den uparxei to arxeio
            strcpy(line1, "HTTP/1.1 404 Not Found\r\n");   //ftiaxnw tis grammes tou http header
            time(&curTime);
            timeInfo = gmtime(&curTime);
            strcpy(timee, asctime(timeInfo));
            timee[strlen(timee)-1] = '\0';
            sprintf(line2, "Date: %s GMT\r\n", timee);
            strcpy(line3, "Server: myhttpd/1.0.0 (Ubuntu64)\r\n");
            sprintf(line4, "Content-Length: 124\r\n");
            strcpy(line5, "Content-Type: text/html\r\n");
            strcpy(line6, "Connection: Closed\r\n");

            send(data.sock, line1, strlen(line1), 0);
            send(data.sock, line2, strlen(line2), 0);
            send(data.sock, line3, strlen(line3), 0);
            send(data.sock, line4, strlen(line4), 0);
            send(data.sock, line5, strlen(line5), 0);
            send(data.sock, line6, strlen(line6), 0);
            send(data.sock, "\r\n", 2, 0);
            send(data.sock, "<html>Sorry dude, couldn't find this file.</html>\r\n", 51, 0);
        }
//        printf("thread: %lud, sockCli id: %d, msg: %s\n", pthread_self(), data.sock, fileName);
    
        close(data.sock);
    }
    pthread_exit(NULL);
}


void *httpHandler(void *arg){
    int *portPtr, portCon;
    int sockCli, sockSer, readSize;
    char cMsg[2048], temp[2048], *word;
    ItemType item;
    struct sockaddr_in server;
    
    portPtr=(int*)arg;
    portCon=*portPtr;
    // sockSer = sockConnect(portCon);
    
    sockSer = socket(AF_INET, SOCK_STREAM, 0);
    if (sockSer == -1){
        printf("ERROR creating socket\n");
    } else{
        //perigrafoume th dieuthunsh kai to port sto socket
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port = htons(portCon);  //host to network byte order

        //bind
        setsockopt(sockSer, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));
        if (bind(sockSer, (struct sockaddr*)&server, sizeof(server)) < 0){
            printf("ERROR bind\n");
        } else{
            //listen
            listen(sockSer, SOMAXCONN);

            while (1){
                if ((sockCli = accept(sockSer, NULL, NULL))){  //perimenei sundeseis apo crawler
                    printf("New connection: %d\n", sockCli);

                    //msg apo client
                    readSize = recv(sockCli, cMsg, sizeof(cMsg), 0); 
                    if (readSize > 0){
                        cMsg[readSize] = '\0';
                        strcpy(temp, cMsg);

                        if ((strcmp(word = strtok(temp, " "), "GET")) == 0){   //eksetazw to request
                            //stelnw to msg tou client sta threads
                            word = strtok(NULL, " '\n'");

                            item.sock = sockCli;
                            strcpy(item.msg, rDir);
                            strcat(item.msg, "/");
                            strcat(item.msg, word);
                            
                            pthread_mutex_lock(&mutex);
                            Insert(item, &queue);
                            pthread_mutex_unlock(&mutex);

                            pthread_cond_signal(&cond);  //shma sto thread oti tha steilei douleia
                        }
                    }
                    else if (readSize == 0){
                        printf("Client disconnected\n");
                        close(sockCli);
                        //break;
                    }
                }
            }
        }
        close(sockSer);
    }
    pthread_exit(NULL);
}


void *cmndHandler(void *arg){
    int *portPtr, portCon, flag=0, i;
    int sockCli, sockSer, readSize;
    char cMsg[2048], temp[2048], buf[80], statsInfo[1024];
    time_t curTime, wTime;
    struct sockaddr_in server;
    
    portPtr = (int*)arg;
    portCon = *portPtr;
    
    sockSer = socket(AF_INET, SOCK_STREAM, 0);
    if (sockSer == -1){
        printf("ERROR creating socket\n");
    }
    else{

        //perigrafoume th dieuthunsh kai to port sto socket
        server.sin_family = AF_INET;
        server.sin_addr.s_addr = INADDR_ANY;
        server.sin_port = htons(portCon);  //host to network byte order

        //bind
        setsockopt(sockSer, SOL_SOCKET, SO_REUSEADDR, &(int){ 1 }, sizeof(int));
        if (bind(sockSer, (struct sockaddr*)&server, sizeof(server)) < 0){
            printf("ERROR bind\n");
        }
        else{
            //listen
            listen(sockSer, SOMAXCONN);

            //accept
            while ((sockCli = accept(sockSer, NULL, NULL)) && (flag == 0)){  //perimenei pollaples sundeseis
                printf("New connection: %d\n", sockCli);

                while (1){
                    //msg apo client
                    readSize = recv(sockCli, cMsg, sizeof(cMsg), 0);
                    if (readSize > 0){
                        cMsg[readSize] = '\0';
                        strcpy(temp, cMsg);

                        if (strncmp(cMsg, "STATS", 5) == 0){
                            curTime = time(NULL);
                            wTime = difftime(curTime, startTime);
                            sprintf(buf, "%02ld:%02ld", wTime/60, wTime%60);
                            sprintf(statsInfo, "Server up for %s, served %d pages, %d bytes\n", buf, pages, bytes);
                            send(sockCli, statsInfo, strlen(statsInfo), 0);
                        }
                        else if (strncmp(cMsg, "SHUTDOWN", 8) == 0){
                            flag = 1;
                            close(sockCli);
                            close(sockSer);
                            for (i=0; i<numThreads; i++)
                                pthread_kill(threads[i], SIGKILL);
                            break;
                        }
                    }
                    else if (readSize == 0){
                        printf("Client disconnected\n");
                        close(sockCli);
                        break;
                    }
                }
            }
        }
    }
    pthread_exit(NULL);
}


int main(int argc, char** argv) {
    int i;
    int sPort, cPort;
    pthread_t sThread, cThread;
    
    startTime = time(NULL);
    printf("myhttpd server is up at ports: 8080 and 9090\n");
    
    if (argc > 1) {
        for (i=1; i<argc-1; i++){
            if (strcmp(argv[i],"-p") == 0){
                sPort = atoi(argv[i+1]);
                i++;
            }
            else if (strcmp(argv[i],"-c") == 0){
                cPort = atoi(argv[i+1]);
                i++;
            }
            else if (strcmp(argv[i],"-t") == 0){
                numThreads = atoi(argv[i+1]);
                i++;
            }
            else if (strcmp(argv[i],"-d") == 0){
                strcpy(rDir, argv[i+1]);
                i++;
            }
            else{
                printf("ERROR\n");
                return 0;
            }
        }
        
        threads = malloc(numThreads * sizeof(pthread_t));
        
        pthread_cond_init(&cond, NULL);
        pthread_mutex_init(&mutex, NULL);
        InitializeQueue(&queue);

        //ksekinoun ta threads
        for (i=0; i<numThreads; i++){
            pthread_create(&threads[i], NULL, threadHandler, NULL);
        }
       
        //http server thread
        pthread_create(&sThread, NULL, httpHandler, &sPort);

        //commant thread
        pthread_create(&cThread, NULL, cmndHandler, &cPort);
        
        //perimenei na teleiwsoun ta threads
        for (i=0; i<numThreads; i++){
            pthread_join(threads[i], NULL);
        }
        
        pthread_join(sThread, NULL);
        pthread_join(cThread, NULL);

        pthread_cond_destroy(&cond);
        pthread_mutex_destroy(&mutex);
    }
    return (EXIT_SUCCESS);
}

