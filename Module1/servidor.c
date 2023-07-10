// Bibliotecas do C para manipular dados
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Bibliotecas do sistema para Sockets e TCP
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <netdb.h>

// Definição de valores globais
#define PORT 3010
#define BUFFER 4096
#define FAIL -1
#define ERROR 2

int main()
{

    /////////////////////////////////////// SERVIDOR
    //   Para que o servidor possa funcionar, é
    //   necessário ter: a) dois sockets para
    //   armazenar as informações de conexão do
    //   cliente e do servidor; b) um buffer para
    //   armazenar as informações de comunicação
    //   (cujo tamanho é pre-estabelecido em 4096).

    // Criação do buffer de dados
    char buffer[BUFFER];

    // Declaração dos sockets de armazenamento
    int serverDecriptor, clientDescriptor;
    struct sockaddr_in server, client;

    /////////////////////////////////////// SOCKETS
    //
    //    O socket permite que processos diferentes
    //    se comuniquem. Nesse caso, a criação do
    //    socket IPv4 envolve o uso da função socket().
    //
    //    Essa função recebe como parâmetros informações
    //    para criar uma conexão TCP (STREAM_SOCKET, ou
    //    seja, criar circuitos virtuais) e usar o
    //    protocolo IPv4 (usando o parâmetro AF_INET).
    //    Como retorno, ela devolve um ponteiro para
    //    ser usado como descritor de arquivos.
    //
    //    Caso o socket seja criado com sucesso, é
    //    preciso configurar as informações na struct
    //    do servidor que foi criada acima. Assim,
    //    configuramos as chaves sin_family, sin_port
    //    e sin_zero.

    // Criação do Socket. É passado o domínio
    // IPv4, o tipo de conexão (TCP - SOCK_STREAM)
    // e o protocolo (0).
    serverDecriptor = socket(AF_INET, SOCK_STREAM, 0);
    fprintf(stdout, "Socket criado sob o numero: %d\n", serverDecriptor);

    // Armazenar informações do socket criado com sucesso
    server.sin_family = AF_INET;
    server.sin_port = htons(PORT);
    memset(server.sin_zero, 0x0, 8);

    //////////////////////////// BIND
    //   Caso o socket tenha sido criado com sucesso, é preciso
    //   realizar a vinculação entre o socket e a porta.

    // Verifica se a porta está em uso
    int check = 1;
    int returnSetSocket = setsockopt(serverDecriptor, SOL_SOCKET, SO_REUSEADDR, &check, sizeof(int));
    if (returnSetSocket == FAIL)
    {
        fprintf(stdout, "Erro na porta!\n");
        return ERROR;
    }

    // Vincula a porta ao Socket.
    int returnBind = bind(serverDecriptor, (struct sockaddr *)&server, sizeof(server));
    if (returnBind == FAIL)
    {
        fprintf(stdout, "Erro na vinculação!\n");
        return ERROR;
    }

    //////////////////////////// LISTEN
    //    Caso tudo tenha ocorrido sem erros, o programa do servidor
    //    irá executar a função listen() e irá bloquear o processo
    //    até que um cliente tente fazer conexão.

    // O programa ficará ouvindo a porta. Caso haja conexão,
    // a comunicação se inicia.
    int returnListen = listen(serverDecriptor, 1);
    if (returnListen == FAIL)
    {
        fprintf(stdout, "Erro em Listening!\n");
        return ERROR;
    }

    //////////////////////////// ACCEPT
    //    O processo permanecerá bloqueado até que o cliente faça
    //    a conexão pelo método do handshake, o que é implementado
    //    pela função accept().

    socklen_t client_len = sizeof(client);
    clientDescriptor = accept(serverDecriptor, (struct sockaddr *)&client, &client_len);
    if (clientDescriptor == FAIL)
    {
        fprintf(stdout, "Erro em Accept!\n");
        return ERROR;
    }

    //////////////////////////// COMUNICAÇÃO
    //    Caso a conexão seja estabelecida entre cliente e
    //    servidor, podemos iniciar a comunicação através
    //    de uma mensagem de boas vindas ao servidor.
    //
    //    O modus operandi da comunicação consiste em o
    //    servidor permanecer disponível para receber mensagens
    //    do cliente, responder a mensagem recebida e ficar
    //    disponível para receber outra mensagem.
    //
    //    Esse processo acontece indefinidamente, mas, caso
    //    receba a mensagem "/quit", a conexão será desfeita e
    //    o servidor será encerrado.

    strcpy(buffer, "Seja bem vindo!\n");

    int returnSend = send(clientDescriptor, buffer, strlen(buffer), 0);
    if (returnSend)
    {
        fprintf(stdout, "Cliente conectado com sucesso.\n");
        fprintf(stdout, "Aguardando mensagens!\n");

        do
        {
            // Limpa o buffer
            memset(buffer, 0x0, BUFFER);

            // Recebe a mensagem do cliente
            int messageLen = recv(clientDescriptor, buffer, BUFFER, 0);
            if (messageLen > 0)
            {
                buffer[messageLen - 1] = '\0';
                printf("Mensagem recebida: %s\n", buffer);
            }

            // Responde ao cliente
            if (strcmp(buffer, "/quit") == 0)
            {
                send(clientDescriptor, "/quit", 5, 0);
            }
            else
            {
                // Zerando buffer
                memset(buffer, 0x0, BUFFER);

                // Coletando mensagem para enviar ao Servidor
                fprintf(stdout, "Digite a mensagem ao cliente:\n");
                fgets(buffer, BUFFER, stdin);
                fprintf(stdout, "Aguardando resposta!\n");

                // Enviando mensagem
                send(clientDescriptor, buffer, strlen(buffer), 0);
            }

        } while (strcmp(buffer, "/quit"));
    }
    return 0;
}
