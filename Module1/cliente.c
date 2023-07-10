/*
    A criação de um cliente seguirá o mesmo protocolo do servidor,
    afinal, os ambos os processos precisam se comunicar sob as
    mesmas premissas (protocolos). Por isso, criaremos um socket
    e iremos tentar estabelecer conexão com o servidor. Caso seja
    possível, iremos iniciar o envio de mensagens. A conexão será
    finalizada quando digitarmos "/quit".
*/

// Bibliotecas do C para manipular dados
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Bibliotecas do sistema para Sockets e TCP
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

// Definição de valores globais
#define SERVER_ADDR "127.0.0.1"
#define TRUE 1
#define PORT 3010
#define BUFFER 4096
#define FAIL -1
#define ERROR 2

int main()
{

    // Criação do buffer de dados
    char buffer[BUFFER];

    // Declaração dos sockets de armazenamento
    int serverDescriptor;
    struct sockaddr_in server;

    int len = sizeof(server);
    int slen;

    serverDescriptor = socket(AF_INET, SOCK_STREAM, 0);
    if (serverDescriptor == FAIL)
    {
        fprintf(stdout, "Erro no porta!\n");
        return ERROR;
    }
    fprintf(stdout, "Socket criado com sucesso!\n");

    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    server.sin_addr.s_addr = inet_addr(SERVER_ADDR);
    memset(server.sin_zero, 0x0, 8);

    // Tenta estabelecer conexão
    int returnConnect = connect(serverDescriptor, (struct sockaddr *)&server, len);
    if (returnConnect == FAIL)
    {
        fprintf(stdout, "Erro na conexão!\n");
        return ERROR;
    }

    // Aguarda resposta do servidor
    slen = recv(serverDescriptor, buffer, BUFFER, 0);
    if (slen > 0)
    {
        buffer[slen - 1] = '\0';
        fprintf(stdout, "Resposta do server: %s\n", buffer);
    }

    while (TRUE)
    {

        // Zerando buffer
        memset(buffer, 0x0, BUFFER);

        // Coletando mensagem para enviar ao Servidor
        fprintf(stdout, "Digite a mensagem:\n");
        fgets(buffer, BUFFER, stdin);
        fprintf(stdout, "Aguardando resposta!\n");

        // Enviando mensagem
        send(serverDescriptor, buffer, strlen(buffer), 0);

        // Zerando buffer para receber resposta
        memset(buffer, 0x0, BUFFER);

        // Recebendo resposta
        slen = recv(serverDescriptor, buffer, BUFFER, 0);
        if (slen > 0)
        {

            if (strcmp(buffer, "/quit") == 0)
            {
                break;
            }
            else
            {
                buffer[slen - 1] = '\0';
                fprintf(stdout, "Resposta do server: %s\n", buffer);
            }
        }
    }

    return 0;
}