#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>

#define MAX 100
#define BUFFER 2048
#define TRUE 1
#define FALSE 0

static _Atomic unsigned int cliCount = 0; // Variável estática atômica que armazena o número de clientes conectados.
static int uid = 1; // Variável estática que armazena o identificador único para cada cliente.

typedef struct {
	struct sockaddr_in address; // Endereço do cliente
	int sockfd; // Arquivo socket associado
	int uid; // ID do cliente
	int admin; // Administrado
	int mute; // Mutado ou Nao
	char name[32]; // Nome
} client_t;

client_t *clients[MAX];
pthread_mutex_t clientsMutex = PTHREAD_MUTEX_INITIALIZER;

// Função auxiliar para limpar e formatar o buffer de saída padrão
void format_stdout() {
	printf("\r%s", "> ");
	fflush(stdout);
}

// Função auxiliar para remover a nova linha de uma string
void remove_line(char *arr, int length) {
	for (int i = 0; i < length; i++) { 
		if (arr[i] == '\n') {
			arr[i] = '\0';
			break;
		}
	}
}

// Imprime o endereço IPv4 do cliente
void print_client(struct sockaddr_in addr) { 
	printf("%d.%d.%d.%d",
		   addr.sin_addr.s_addr & 0xff,
		   (addr.sin_addr.s_addr & 0xff00) >> 8,
		   (addr.sin_addr.s_addr & 0xff0000) >> 16,
		   (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

// Função para adicionar pessoas ao chat
void queue_add(client_t *cl) {
    pthread_mutex_lock(&clientsMutex);

    if (!clients[0]) {
        clients[0] = cl;
        clients[0]->admin = TRUE;
        clients[0]->mute = FALSE; // Set mute to FALSE
    }
    else {
        for (int i = 1; i < MAX; i++) {
            if (!clients[i]) {
                clients[i] = cl;
                clients[i]->mute = FALSE; // Set mute to FALSE
                break;
            }
        }
    }

    pthread_mutex_unlock(&clientsMutex);
}

// Função para remover pessoas ao chat
void queue_remove(int uid) {
	pthread_mutex_lock(&clientsMutex); // Adquire o mutex para garantir acesso exclusivo à lista de clientes

	for (int i = 0; i < MAX; ++i) {
		if (clients[i] && clients[i]->uid == uid) { // Verifica se não esta vaziou e se o ID é igual
			clients[i] = NULL; // Define o slot como vazio (NULL) para remover o cliente
			break; // Sai do loop, pois o cliente foi removido com sucesso
		}
	}

	pthread_mutex_unlock(&clientsMutex); // Libera o mutex para permitir que outras threads acessem a lista de clientes
} 

void send_message(char *s, int uid, int mute) {
	pthread_mutex_lock(&clientsMutex); // Adquire o mutex para garantir acesso exclusivo à lista de clientes

	// Envia a mensagem para os clientes conectados, menos para o que enviou a mensagem
	for (int i = 0; i < MAX; i++) {
		if (clients[i] && mute == FALSE && clients[i]->uid != uid && write(clients[i]->sockfd, s, strlen(s)) < 0) {
			perror("ERROR: write to descriptor failed");
			break;
		}
	}

	pthread_mutex_unlock(&clientsMutex); // Libera o mutex para permitir que outras threads acessem a lista de clientes
}

int find_index(char *nick) {
	for (int i = 0; i < MAX; i++) {
		if (clients[i] && strcmp(clients[i]->name, nick) == 0) {
			return i;
		}
	}

	return -1;
}

void *handle_client(void *arg) {
	char buffOut[BUFFER]; // Buffer para o envio de dados
	char name[32]; // Nome do cliente
	int leaveFlag = 0; // Varial para o controle de saída

	cliCount++; // Incrementa o contador de clientes
	client_t *cli = (client_t *)arg; // Converte o argumento em um ponteiro para client_t

	// Recebe o nome do cliente
	if (recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) < 2 || strlen(name) >= 32 - 1) {
		printf("Didn't enter the name.\n");
		leaveFlag = 1;
	}
	else {
		strcpy(cli->name, name);
		sprintf(buffOut, "%s has joined\n", cli->name);
		printf("%s", buffOut);
		send_message(buffOut, cli->uid, cli->mute); // Envia uma mensagem informando que o cliente entrou para todos os outros clientes
	}

	bzero(buffOut, BUFFER); // Limpa o buffer de saída

	// Enquanto um cliente não sair
	while (TRUE) {
		if (leaveFlag) {
			break;
		}

		int receive = recv(cli->sockfd, buffOut, BUFFER, 0); 

		if (receive > 0) {
			
			if (strlen(buffOut) > 0) {
				if (strstr(buffOut, "/kick") && cli->admin == TRUE) {
					send_message(buffOut, cli->uid, cli->mute); 
				}
				else if (!strstr(buffOut, "/kick")) {
					send_message(buffOut, cli->uid, cli->mute); 
				}

				remove_line(buffOut, strlen(buffOut)); 

				printf("%s -> NAME: %s ADMIN: %d UID: %d\n", buffOut, cli->name, cli->admin, cli->uid); 
			}
			
			if (strstr(buffOut, "/ping") != NULL) { 
				strcpy(buffOut, "Servidor: PONG\n"); 
				sleep(.25); 
				send_message(buffOut, -1, cli->mute); 
			}

			else if (strstr(buffOut, "/nickname") != NULL) {
				char *nick = strtok(buffOut, " ");
				nick = strtok(NULL, " ");

				if (nick != NULL) {
        			strcpy(cli->name, nick);
    			}
			}

			else if (cli->admin == TRUE && strstr(buffOut, "/mute")) {
				char *nick = strtok(buffOut, " ");
				nick = strtok(NULL, " ");
				nick = strtok(NULL, " ");

				int index = find_index(nick);
				
				if (index != -1) {
					clients[index]->mute = TRUE;
					
					strcpy(buffOut, "Servidor: ");
					strcat(buffOut, clients[index]->name);
					strcat(buffOut, " is now muted\n");
					sleep(.25);
					send_message(buffOut, -1, FALSE); 
				}
			}

			else if (cli->admin == TRUE && strstr(buffOut, "/unmute")) {
				char *nick = strtok(buffOut, " ");
				nick = strtok(NULL, " ");
				nick = strtok(NULL, " ");

				int index = find_index(nick);

				if (index != -1) {
					clients[index]->mute = FALSE;
				
					strcpy(buffOut, "Servidor: ");
					strcat(buffOut, clients[index]->name);
					strcat(buffOut, " is now unmuted\n");
					sleep(.25);
					send_message(buffOut, -1, FALSE);
				}				 
			}
			else if (cli->admin == TRUE && strstr(buffOut, "/whois")) {
				char *nick = strtok(buffOut, " ");
				nick = strtok(NULL, " ");
				nick = strtok(NULL, " ");

				int index = find_index(nick);

				if (index != -1) {
					char s[MAX];

					sprintf(s, "Servidor: The IPv4 of %s is %d.%d.%d.%d\n", clients[index]->name
																	      , clients[index]->address.sin_addr.s_addr & 0xff
																	      , (clients[index]->address.sin_addr.s_addr & 0xff00) >> 8
																	      , (clients[index]->address.sin_addr.s_addr & 0xff0000) >> 16
																	      , (clients[index]->address.sin_addr.s_addr & 0xff000000) >> 24);

					write(cli->sockfd, s, strlen(s));
					sleep(.25);
				}
			}
		}

		else if (receive == 0 || strcmp(buffOut, "/quit") == 0) {
			sprintf(buffOut, "%s has left\n", cli->name); 
			printf("%s", buffOut);
			send_message(buffOut, cli->uid, FALSE); 
			leaveFlag = 1; 
		}

		else {
			printf("ERROR: -1\n");
			leaveFlag = 1;
		}

		bzero(buffOut, BUFFER); // Limpa o buffer de saída
	}

	close(cli->sockfd); // Fecha o socket do cliente
	queue_remove(cli->uid); // Remove o cliente da lista de clientes conectados
	free(cli); // Libera a memória alocada para a estrutura do cliente
	cliCount--; // Decrementa o contador de clientes
	pthread_detach(pthread_self()); // Desanexa a thread atual

	return NULL;
}

int main(int argc, char **argv) {
	if (argc != 2) { // Verifica o número de argumentos de linha de comando
		printf("Usage: %s <port>\n", argv[0]);
		return EXIT_FAILURE;
	}

	char *ip = "127.0.0.1";
	int port = atoi(argv[1]);
	int option = 1;
	int listenfd = 0, connfd = 0;
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	pthread_t tid;

	listenfd = socket(AF_INET, SOCK_STREAM, 0); // Cria um socket para ouvir as conexões de entrada
	serv_addr.sin_family = AF_INET; // Configura a família de endereços do socket como IPv4
	serv_addr.sin_addr.s_addr = inet_addr(ip); // Configura o endereço IP do servidor
	serv_addr.sin_port = htons(port); // Configura a porta do servidor

	// Ignora o sinal SIGPIPE para evitar o término abrupto do programa em caso de erro de escrita no socket
	signal(SIGPIPE, SIG_IGN);

	// Define a opção SO_REUSEADDR para permitir o reuso do endereço do socket
	if (setsockopt(listenfd, SOL_SOCKET, (SO_REUSEADDR), (char *)&option, sizeof(option)) < 0) {
		perror("ERROR: setsockopt failed");
		return EXIT_FAILURE;
	}

	// Associa o socket a um endereço IP e número de porta
	if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
		perror("ERROR: Socket binding failed");
		return EXIT_FAILURE;
	}

	// Aguarda conexões de entrada no socket
	if (listen(listenfd, 10) < 0) {
		perror("ERROR: Socket listening failed");
		return EXIT_FAILURE;
	}

	printf("=== WELCOME TO THE CHATROOM ===\n");

	while (TRUE) {
		socklen_t clilen = sizeof(cli_addr);

		// Aceita uma nova conexão de cliente
		connfd = accept(listenfd, (struct sockaddr *)&cli_addr, &clilen);

		// Verifica se o número máximo de clientes foi atingido
		if ((cliCount + 1) == MAX) {
			printf("Max clients reached. Rejected: ");
			print_client(cli_addr);
			printf(":%d\n", cli_addr.sin_port);
			close(connfd);
			continue;
		}

		// Aloca memória para a estrutura do cliente
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = cli_addr;
		cli->sockfd = connfd;
		cli->uid = uid++;
		cli->admin = FALSE;
		cli->mute = FALSE;

		queue_add(cli); // Adiciona o cliente à lista de clientes conectados

		pthread_create(&tid, NULL, &handle_client, (void *)cli); // Cria uma nova thread para lidar com o cliente

		sleep(1); // Reduz a utilização da CPU
	}

	return EXIT_SUCCESS;
}