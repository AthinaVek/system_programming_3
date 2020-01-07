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
int sum=0, pages=0, bytes=0, numThreads;
pthread_t *threads;
time_t startTime;
char sDir[128];

typedef struct serInfo{
    int sPort;
    char ip[20];
}serInfo;

typedef struct linkList{
    char link[128];
    struct linkList *next;
}linkList;

linkList *linksHead=NULL, *links=NULL;
char filesTable[100][200];

void *threadHandler(void *arg){    //stelnei request, diavazei thn apanthsh kai ta grafei
    struct sockaddr_in server;  //plhrofories server pou prokeitai na sundethei o client
    serInfo *siPtr;
    char line4[600], buf2[128], sMpar[10];
    char path[256], cMsg[512], dr[128], ch, *szBytes, *link, *endLink, buff[1024], page[128], site[128], *ptr;
    char line1[600], line2[600], line3[600], line5[600], line6[600], line[600], headerBuf[1024];
    ssize_t readSize, pgBytes=0;
    ItemType data;
    FILE *fp;
    int pg, i, flag, sockCli, n, k;
    linkList  *new=NULL;
    ItemType item;
    struct stat statStruct;
    int totalBytes, v;

    siPtr=(serInfo*)arg;
    
    while (1){
        pthread_mutex_lock(&mutex);
        while (Empty(&queue)){   //perimenei shma oti mphke kati sthn oura
            pthread_cond_wait(&cond, &mutex);  
        }
        Remove(&queue, &data);
        pthread_mutex_unlock(&mutex);

        flag = 0;
        pg = 0;
        
        sockCli = socket(AF_INET, SOCK_STREAM, 0);
        if (sockCli == -1){
            printf("ERROR creating socket\n");
        }
        else{
            //perigrafoume th dieuthunsh kai to port sto socket
            server.sin_family = AF_INET;
            server.sin_addr.s_addr = inet_addr(siPtr->ip);
            server.sin_port = htons(siPtr->sPort);  //host to network byte order

            if (connect(sockCli, (struct sockaddr *)&server, sizeof(server)) == 0){
                //printf("Connected with server\n");
                memset(headerBuf, '\0', sizeof(headerBuf));

                sprintf(line1, "GET %s HTTP/1.1\r\n", data.msg);    //ftiaxnw tis grammes tou http request
                strcpy(headerBuf, line1);
                strcpy(line2, "User-Agent: Mozilla/4.0 (compatible; MSIE5.01; Windows NT)\r\n");
                strcat(headerBuf, line2);
                strcpy(line3, "Host: www.tutorialspoint.com\r\n");
                strcat(headerBuf, line3);
                strcpy(line4, "Accept-Language: en-us\r\n");
                strcat(headerBuf, line4);
                strcpy(line5, "Accept-Encoding: gzip, deflate\r\n");
                strcat(headerBuf, line5);
                strcpy(line6, "Connection: Keep-Alive\r\n");
                strcat(headerBuf, line6);
                strcat(headerBuf, "\r\n");
                send(sockCli, headerBuf, strlen(headerBuf), 0);       

                memset(headerBuf, '\0', sizeof(headerBuf));
                if ((readSize = recv(sockCli, headerBuf, sizeof(headerBuf), 0)) > 0){
                    headerBuf[readSize] = '\0';
                    i = 0;
                    k = 0;
                    while((ch = headerBuf[i]) != '\n'){
                        line1[k++] = ch;
                        i++;
                    }
                    line1[k] = '\0';
                    if ((strncmp(line1, "HTTP/1.1 200 OK", 15)) == 0){
                        memset(path, '\0', sizeof(path));

                        strcpy(path, sDir);
                        strcat(path, "/");

                        ptr = strchr(data.msg, '/');
                        n = ptr - data.msg;

                        strncpy(site, data.msg, n);
                        site[n] = '\0';

                        ptr++;
                        strcpy(page, ptr);

                        strcat(path, site);
                        realpath(path, dr);
                        if ((stat(dr, &statStruct)) != 0) {  //to path den yparxei. dhmiourgise to
                            mkdir(dr, 0777);
                        }

                        strcat(dr, "/");
                        strcat(dr, page);

                        if ((access(dr, F_OK)) == -1 ){ // to arxeio den yparxei
                            if((fp = fopen(dr, "w"))){ // dhmiourghse to
                                //printf("ThreadID: %ld, SocketID: %d, request FileName = %s from server\n", pthread_self(), sockCli, dr);
                                flag = 1;
                            }
                        }
                    }
                    
                    k = 0;
                    i++;
                    while((ch = headerBuf[i]) != '\n'){
                        line2[k++] = ch;
                        i++;
                    }
                    line2[k] = '\0';

                    k = 0;
                    i++;
                    while((ch = headerBuf[i]) != '\n'){
                        line3[k++] = ch;
                        i++;
                    }
                    line3[k] = '\0';

                    k = 0;
                    i++;
                    int r = 0;
                    while((ch = headerBuf[i]) != '\n'){
                        if(k>=15 && ch != '\r')
                            buf2[r++] = ch;
                            
                        line4[k++] = ch;
                        i++;
                    }
                    line4[k] = '\0';
                    buf2[r] = '\0';

                    totalBytes = atoi(buf2);
                    // printf("total: %d\n", totalBytes);
                    printf("Get filename: %s\n", page);
                    
                    if (flag){    //diavazw to arxeio
                        memset(cMsg, '\0', sizeof(cMsg));
                        while (((readSize = recv(sockCli, cMsg, 512, 0)) > 0)){
                            fwrite(cMsg, sizeof(char), readSize, fp);

                            pgBytes += readSize;
                            if (pgBytes == totalBytes){
                                break;
                            }
                            memset(cMsg, '\0', sizeof(cMsg));
                        }
                        fclose(fp);
                    }

                    if (flag){
                        pthread_mutex_lock(&mutex);
                        bytes += pgBytes;
                        pages++;
                        pthread_mutex_unlock(&mutex);
                    }

                    if ((fp = fopen(dr, "r")) && flag){   //anoigw gia na parw ta links
                        while ((fgets(line, sizeof(line), fp))){   
                            line[strlen(line)-1] = '\0';
                            if (strncmp(line, "<a href=", 8) == 0){
                                link = strstr(line, "site");
                                endLink = strchr(line, '>');
                                if (endLink){
                                    v = endLink-link;
                                    
                                    if (v < 1024) {
                                        strncpy(buff, link, v);
                                        buff[v] = '\0';

                                        pthread_mutex_lock(&mutex);
                                        links = linksHead;
                                        while (links){   //psaxnw na dw an uparxei to link hdh sth lista
                                            if (strcmp(links->link, buff) == 0){
                                                break;
                                            }
                                            links = links->next;
                                        }
                                        pthread_mutex_unlock(&mutex);

                                        if (links == NULL){   //an den uparxei to vazw
                                            pthread_mutex_lock(&mutex);
                                            new = malloc(sizeof(linkList));
                                            strcpy(new->link, buff);
                                            new->next = NULL;
                                            
                                            if (linksHead == NULL){
                                                linksHead = new;
                                            }
                                            else{
                                                new->next = linksHead;
                                                linksHead = new;
                                            }

                                            strcpy(item.msg, new->link);
                                            Insert(item, &queue);
                                            pthread_mutex_unlock(&mutex);

                                            pthread_cond_signal(&cond);
                                        }
                                    }
                                }
                            }
                        }
                        fclose(fp);
                    }
                }
            }
            else{
                printf("Can't connect to myhttpd server.\n");
                break;
            }
        }
        close(sockCli);
    }
    pthread_exit(NULL);
}


void *cmndHandler(void *arg){
    int *portPtr, portCon, flag=0, i;
    int sockCli, sockSer, readSize;
    char cMsg[2048], temp[2048], buf[80], statsInfo[1024];
    time_t curTime, wTime;
    struct sockaddr_in server;
    
    portPtr=(int*)arg;
    portCon=*portPtr;
    
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
            listen(sockSer, 5);

            printf("Crawler server is running at port 9095\n");
            //accept
            while ((sockCli = accept(sockSer, NULL, NULL)) && (flag == 0)){
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
                        else if ((strncmp(cMsg, "SEARCH", 6)) == 0){
                            printf("SEARCH\n");
                        }
                        else if ((strncmp(cMsg, "SHUTDOWN", 8)) == 0){
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
    int i, cPort;
    char sUrl[128];
    pthread_t comThread;
    serInfo si;
    ItemType item;
    
    startTime = time(NULL);
    
    if (argc > 1) {
/*
        for (i=1; i<argc-1; i++){
            if (strcmp(argv[i],"-h") == 0){
                strcpy(si.ip, argv[i+1]);
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
            else if (strcmp(argv[i],"-p") == 0){
                si.sPort = atoi(argv[i+1]);
                i++;
            }
            else if (strcmp(argv[i],"-d") == 0){
                strcpy(sDir, argv[i+1]);
                i+=2;
                strcpy(sUrl, argv[i]);
            }
            else{
                printf("ERROR\n");
                return 0;
            }
        }
*/
         if (strcmp(argv[1],"-h") == 0){
                strcpy(si.ip, argv[2]);
            }
         
         if (strcmp(argv[3],"-p") == 0){
            si.sPort = atoi(argv[4]);
        }
         
         if (strcmp(argv[5],"-c") == 0){
                cPort = atoi(argv[6]);
            }
         
        if (strcmp(argv[7],"-t") == 0){
            numThreads = atoi(argv[8]);
        }
        
         
        if (strcmp(argv[9],"-d") == 0){
            strcpy(sDir, argv[10]);
        }

        strcpy(sUrl, argv[11]);
        
        threads = malloc(numThreads * sizeof(pthread_t));   //desmeuw mnhmh gia ta pool threads ids
        
        pthread_cond_init(&cond, NULL);   
        pthread_mutex_init(&mutex, NULL);   
        InitializeQueue(&queue);
        
        strcpy(item.msg, sUrl);
        Insert(item, &queue);
        

        //ksekinoun ta threads
        for (i=0; i<numThreads; i++){
            pthread_create(&threads[i], NULL, threadHandler, &si);
        }
       
        //commant thread
        pthread_create(&comThread, NULL, cmndHandler, &cPort);
        
        //perimenei na teleiwsoun ta threads
        for (i=0; i<numThreads; i++){
            pthread_join(threads[i], NULL);
        }
        
        pthread_join(comThread, NULL);

        pthread_cond_destroy(&cond);
        pthread_mutex_destroy(&mutex);
    }
    return (EXIT_SUCCESS);
}

